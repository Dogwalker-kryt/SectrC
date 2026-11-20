#define _GNU_SOURCE
#include "overwriter.h"
#include "ow_ui.h"
#include "../defs.h"
#include "../globals.h"
#include "../Debug/sectr_err.h"
#include "../disk_data/disk_data.h"
#include "../read_sys_fs/read_sys_fs.h"
#include "../sectr_helper/sectr_helper.h"
#include "../security/prompt.h"
#include "../security/permission.h"
#include "../spinner/spinner.h"
#include "../ui/ui.h"
#include <errno.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <sys/param.h>

#define WRITE_BUFFER_SIZE (1024 * 1024)
#define SYNC_CHUNK_SIZE (64 * 1024 * 1024)
#define LOG_MSG_SIZE 256

typedef enum {
    OM_DONE,
    OM_UNSUPPORTED,
    OM_FAILED,
} om_result_t;


static int32_t open_drive(const char *disk_name) {
    // O_EXCL on a block device fails with EBUSY while it (or a partition of it) is mounted, swap, LVM or RAID
    const int32_t disk = open(disk_name, O_RDWR | O_EXCL | O_CLOEXEC);
    if (disk >= 0) return disk;

    if (errno == EACCES || errno == EPERM) ERR(ERRC_PermissionDenied, NULL, "Cannot open %s for writing, run it with sudo", disk_name);
    else if (errno == EBUSY) ERR(ERRC_DeviceBusy, NULL, "%s is in use (mounted, swap, LVM or RAID)", disk_name);
    else ERR(ERRC_IOError, "Failed to open drive for overwriting", "%s: %s", disk_name, strerror(errno));
    return -1;
}

static void close_drive(int32_t disk) {
    close(disk);
}


typedef struct {
    bool blkdiscard;
    bool blksecurediscard;
} support_t;

static const support_t *check_support(const char *queue_disk_name, support_t *support) {
    char path_buffer[PATH_MAX];

    snprintf(path_buffer, sizeof(path_buffer), "/sys/block/%s/queue/discard_max_bytes", queue_disk_name);
    const uint64_t discard_max = read_sysfs_u64(path_buffer);

    snprintf(path_buffer, sizeof(path_buffer), "/sys/block/%s/queue/discard_granularity", queue_disk_name);
    const uint64_t discard_granularity = read_sysfs_u64(path_buffer);

    support->blkdiscard = discard_granularity > 0 && discard_max > 0;
    support->blksecurediscard = support->blkdiscard;
    return support;
}


static om_result_t om_perform_secure_discard(int32_t fd, uint64_t size) {
    uint64_t range[2] = {0, size};

    spinner_start_pt("Secure discard");
    const int32_t result = ioctl(fd, BLKSECDISCARD, &range);
    const int32_t error = errno;
    spinner_kill_pt();

    if (result == 0) return OM_DONE;
    if (error == EOPNOTSUPP) return OM_UNSUPPORTED;

    ERR(ERRC_IOError, "Secure discard failed", "%s", strerror(error));
    return OM_FAILED;
}

static om_result_t om_perform_discard(int32_t fd, uint64_t size) {
    uint64_t range[2] = {0, size};

    spinner_start_pt("Discard");
    const int32_t result = ioctl(fd, BLKDISCARD, &range);
    const int32_t error = errno;
    spinner_kill_pt();

    if (result == 0) return OM_DONE;
    if (error == EOPNOTSUPP) return OM_UNSUPPORTED;

    ERR(ERRC_IOError, "Discard failed", "%s", strerror(error));
    return OM_FAILED;
}

static bool om_perform_zeroout(int32_t fd, uint64_t size) {
    char *write_buffer = calloc(WRITE_BUFFER_SIZE, 1);
    if (!write_buffer) {
        ERR(ERRC_AllocationFault, NULL, "Failed to allocate zero-fill buffer");
        return false;
    }
    uint64_t written = 0, started = 0, flushed = 0;
    uint64_t last_percent = 0;

    print_progress(0, size);
    while (written < size) {
        const size_t to_write = (size_t)MIN((uint64_t)WRITE_BUFFER_SIZE, size - written);
        const ssize_t result = write(fd, write_buffer, to_write);

        if (result < 0 && errno == EINTR) continue;
        if (result <= 0) {
            putchar('\n');
            ERR(ERRC_IOError, "Failed to write zeros to drive", "at byte %" PRIu64 ": %s", written, result < 0 ? strerror(errno) : "no space left");
            free(write_buffer);
            return false;
        }

        written += (uint64_t)result;
        if (written - started < SYNC_CHUNK_SIZE && written < size) continue;

        if (started > flushed && sync_file_range(fd, (off_t)flushed, (off_t)(started - flushed), SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WRITE | SYNC_FILE_RANGE_WAIT_AFTER) != 0) {
            putchar('\n');
            ERR(ERRC_IOError, "Failed to write zeros to drive", "at byte %" PRIu64 ": %s", flushed, strerror(errno));
            free(write_buffer);
            return false;
        }
        flushed = started;

        if (sync_file_range(fd, (off_t)started, (off_t)(written - started), SYNC_FILE_RANGE_WRITE) != 0) {
            putchar('\n');
            ERR(ERRC_IOError, "Failed to write zeros to drive", "at byte %" PRIu64 ": %s", started, strerror(errno));
            free(write_buffer);
            return false;
        }
        started = written;

        const uint64_t percent = flushed * 100 / size;
        if (percent != last_percent) {
            print_progress(flushed, size);
            last_percent = percent;
        }
    }

    if (fsync(fd) != 0) {
        putchar('\n');
        ERR(ERRC_IOError, "Failed to flush zeros to drive", "%s", strerror(errno));
        free(write_buffer);
        return false;
    }
    print_progress(size, size);
    putchar('\n');

    free(write_buffer);
    return true;
}


static const drive_mt *find_drive(const Vec *vec, const char *wanted) {
    const drive_mt *drives = vec->data;
    char name[DEVICE_NAME_SIZE];

    for (uint32_t i = 0; i < vec->element_count; ++i) {
        if (device_basename(drives[i].name, name, sizeof(name)) && strcmp(name, wanted) == 0) return &drives[i];
    }
    return NULL;
}

static const drive_mt *find_mounted(const Vec *vec, const drive_mt *target) {
    const drive_mt *drives = vec->data;

    for (uint32_t i = 0; i < vec->element_count; ++i) {
        const drive_mt *drive = &drives[i];
        const bool belongs = drive == target || strcmp(drive->parent, target->name) == 0;
        if (belongs && drive->mountpoint[0] != '\0') return drive;
    }
    return NULL;
}

static bool erase(int32_t fd, uint64_t size, const char *queue_disk_name, const char **method) {
    support_t support;
    check_support(queue_disk_name, &support);

    if (support.blksecurediscard) {
        printf("  %ssecure discard…%s\n", GRAY, RST);
        const om_result_t result = om_perform_secure_discard(fd, size);
        if (result == OM_FAILED) return false;
        if (result == OM_DONE) { *method = "secure discard"; return true; }
    }

    if (support.blkdiscard) {
        printf("  %sdiscarding…%s\n", GRAY, RST);
        const om_result_t result = om_perform_discard(fd, size);
        if (result == OM_FAILED) return false;
        if (result == OM_DONE) { *method = "discard"; return true; }
    }

    printf("  %swriting zeros, this can take hours%s\n", GRAY, RST);
    if (!om_perform_zeroout(fd, size)) return false;
    *method = "zero fill";
    return true;
}


bool disk_overwrite(const char *disk_name) {
    if (!is_root()) {
        ERR(ERRC_PermissionDenied, NULL, "This function requires root privileges. Please run with 'sudo'");
        return false;
    }

    char wanted[DEVICE_NAME_SIZE];
    if (!device_basename(disk_name, wanted, sizeof(wanted))) {
        ERR(ERRC_InvalidInput, NULL, "'%s' is not a valid disk name", disk_name);
        return false;
    }

    Vec vec;
    if (!vec_init(&vec, 8, sizeof(drive_mt))) {
        ERR(ERRC_AllocationFault, NULL, "Failed to init vec for overwriting");
        return false;
    }
    if (!get_physical_disk_info(&vec)) {
        vec_free(&vec);
        return false;
    }

    const drive_mt *target = find_drive(&vec, wanted);
    if (!target) {
        vec_free(&vec);
        ERR(ERRC_DeviceNotFound, NULL, "No disk or partition named '%s'", wanted);
        return false;
    }

    const drive_mt *mounted = find_mounted(&vec, target);
    if (mounted) {
        ERR(ERRC_DeviceBusy, "Refusing to overwrite a mounted device", "%s is mounted at %s, unmount it first", mounted->name, mounted->mountpoint);
        vec_free(&vec);
        return false;
    }

    char device_path[DEVICE_NAME_SIZE], queue_disk_name[DEVICE_NAME_SIZE];
    snprintf(device_path, sizeof(device_path), "%s", target->name);
    if (!device_basename(target->parent[0] ? target->parent : target->name, queue_disk_name, sizeof(queue_disk_name))) {
        ERR(ERRC_CorruptedData, NULL, "Bad device name from udev: '%s'", target->name);
        vec_free(&vec);
        return false;
    }

    const int32_t fd = open_drive(device_path);
    if (fd < 0) {
        vec_free(&vec);
        ERR(ERRC_IOError, NULL, "Failed to open %s, for overwriting", disk_name);
        return false;
    }

    struct stat st;
    uint64_t size = 0;
    if (fstat(fd, &st) != 0 || !S_ISBLK(st.st_mode)) {
        ERR(ERRC_InvalidDevice, NULL, "%s is not a block device", device_path);
        close_drive(fd);
        vec_free(&vec);
        return false;
    }
    if (ioctl(fd, BLKGETSIZE64, &size) != 0 || size == 0) {
        ERR(ERRC_IOError, "Failed to get drive size for overwriting", "%s: %s", device_path, size == 0 ? "size is 0" : strerror(errno));
        close_drive(fd);
        vec_free(&vec);
        return false;
    }

    char size_str[16];
    ui_format_size(size, size_str, sizeof(size_str));

    print_erase_list(&vec, target);
    vec_free(&vec);

    if (!globals.bypass_security_code) {
        if (!isatty(STDIN_FILENO)) {
            ERR(ERRC_NoInput, "Cannot ask for confirmation", "stdin is not a terminal, refusing to overwrite %s", device_path);
            close_drive(fd);
            return false;
        }

        if (!ow_confirm_erase(device_path, size_str)) {
            ow_print_cancelled();

            char log_msg[LOG_MSG_SIZE];
            snprintf(log_msg, sizeof(log_msg), "Overwrite of %s cancelled by user", device_path);
            LOG_INFO(log_msg);
            
            close_drive(fd);
            return true;
        }
    }

    char log_msg[LOG_MSG_SIZE];
    snprintf(log_msg, sizeof(log_msg), "Overwrite of %s (%" PRIu64 " bytes) started%s", device_path, size, globals.bypass_security_code ? " without confirmation" : "");
    LOG_INFO(log_msg);

    putchar('\n');
    const char *method = NULL;
    const bool ok = erase(fd, size, queue_disk_name, &method);
    close_drive(fd);

    if (!ok) return false;

    snprintf(log_msg, sizeof(log_msg), "Overwrote %s (%" PRIu64 " bytes) using %s", device_path, size, method);
    LOG_INFO(log_msg);
    ow_print_done(device_path, size_str, method);
    return true;
}
