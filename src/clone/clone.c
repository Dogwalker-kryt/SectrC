#define _GNU_SOURCE
#include "clone.h"
#include "cl_ui.h"
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
#include <time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <sys/param.h>

#define CLONE_BUFFER_SIZE (uint64_t)(4 * 1024 * 1024)

static int32_t get_disk_size(fd_t fd, uint64_t *size) {
    return ioctl(fd, BLKGETSIZE64, size);
}

static int32_t get_sector_size(fd_t fd, uint32_t *size) {
    return ioctl(fd, BLKSSZGET, size);
}

#define CLONE_READONLY 0
#define CLONE_WRITE 1

static int32_t open_drive(const char *disk_name, uint8_t type_to_open) {
    volatile fd_t disk = 0;

    if (type_to_open == CLONE_READONLY) {
        disk = (fd_t)open(disk_name, O_RDONLY | O_EXCL | O_CLOEXEC);
    } else if (type_to_open == CLONE_WRITE) {
        disk = (fd_t)open(disk_name, O_RDWR | O_EXCL | O_CLOEXEC);
    }

    if (disk >= 0) return disk;

    if (errno == EACCES || errno == EPERM) ERR(ERRC_PermissionDenied, NULL, "Cannot open %s for cloning, run it with sudo", disk_name);
    else if (errno == EBUSY) ERR(ERRC_DeviceBusy, NULL, "%s is in use (mounted, swap, LVM or RAID)", disk_name);
    else ERR(ERRC_IOError, "Failed to open drive for cloning", "%s: %s", disk_name, strerror(errno));
    return -1;
}

#define CLONE_SRC_IDX 0
#define CLONE_DEST_IDX 1
#define CLONE_DISK_COUNT 2

/**
 * @brief Opens the source and destination drives for cloning. uses malloc
 * @return the fds at CLONE_SRC_IDX and CLONE_DEST_IDX (close_disks + free it), or NULL on failure (ERR is set)
 */
static fd_t *open_disks(const char *__src, const char *__dest) {
    fd_t *disks_arr = (fd_t*)malloc(sizeof(fd_t) * CLONE_DISK_COUNT);
    if (!disks_arr) {
        ERR(ERRC_AllocationFault, NULL, "Failed to allocate the disk array for cloning");
        return NULL;
    }

    const fd_t src = open_drive(__src, CLONE_READONLY);
    if (src < 0) {
        free(disks_arr);
        return NULL;
    }

    disks_arr[CLONE_SRC_IDX] = src;

    const fd_t dest = open_drive(__dest, CLONE_WRITE);
    if (dest < 0) {
        close(src);
        free(disks_arr);
        return NULL;
    }

    disks_arr[CLONE_DEST_IDX] = dest;

    return disks_arr;
}

static void close_disks(const fd_t *__arr) {
    if (!__arr) return;

    for (uint8_t i = 0; i < CLONE_DISK_COUNT; i++) {
        close(__arr[i]);
    }
}

static bool check_sizes(const char *src_path, uint64_t src_size, const char *dest_path, uint64_t dest_size) {
    if (dest_size >= src_size) return true;

    char src_str[16], dest_str[16];
    ui_format_size(src_size, src_str, sizeof(src_str));
    ui_format_size(dest_size, dest_str, sizeof(dest_str));
    ERR(ERRC_OutOfRange, "Target drive is too small", "%s has %" PRIu64 " bytes (%s), %s only %" PRIu64 " bytes (%s)", src_path, src_size, src_str, dest_path, dest_size, dest_str);
    return false;
}

static bool get_sizes(const fd_t *disks, const char *src_path, const char *dest_path, uint64_t *src_size, uint64_t *dest_size) {
    if (get_disk_size(disks[CLONE_SRC_IDX], src_size) != 0) {
        ERR(ERRC_IOError, "Failed to get drive size for cloning", "%s: %s", src_path, strerror(errno));
        return false;
    }
    if (get_disk_size(disks[CLONE_DEST_IDX], dest_size) != 0) {
        ERR(ERRC_IOError, "Failed to get drive size for cloning", "%s: %s", dest_path, strerror(errno));
        return false;
    }
    if (*src_size == 0) {
        ERR(ERRC_InvalidDevice, NULL, "%s has a size of 0 (no medium inserted?)", src_path);
        return false;
    }
    return check_sizes(src_path, *src_size, dest_path, *dest_size);
}

static bool check_sector_sizes(const fd_t *disks, const char *src_path, const char *dest_path) {
    uint32_t src_sector = 0, dest_sector = 0;
    if (get_sector_size(disks[CLONE_SRC_IDX], &src_sector) != 0 || get_sector_size(disks[CLONE_DEST_IDX], &dest_sector) != 0) {
        ERR(ERRC_IOError, "Failed to get sector size for cloning", "%s", strerror(errno));
        return false;
    }
    if (src_sector == dest_sector) return true;

    ERR(ERRC_InvalidDevice, "Sector sizes don't match", "%s uses %" PRIu32 " byte sectors, %s uses %" PRIu32 ", the copied partition table would be broken", src_path, src_sector, dest_path, dest_sector);
    return false;
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

static const drive_mt *find_named(const Vec *vec, const char *disk_name, const char *role) {
    char wanted[DEVICE_NAME_SIZE];
    if (!device_basename(disk_name, wanted, sizeof(wanted))) {
        ERR(ERRC_InvalidInput, NULL, "'%s' is not a valid disk name (%s)", disk_name, role);
        return NULL;
    }

    const drive_mt *drive = find_drive(vec, wanted);
    if (!drive) ERR(ERRC_DeviceNotFound, NULL, "No disk or partition named '%s' (%s)", wanted, role);
    return drive;
}

/**
 * @brief Finds both drives in @p vec and checks that they can be cloned: not the same, not overlapping, not mounted.
 * @return true with @p src and @p dest set, false on failure (ERR is set)
 */
static bool find_pair(const Vec *vec, const char *disk_src, const char *disk_dest, const drive_mt **src, const drive_mt **dest) {
    const drive_mt *source = find_named(vec, disk_src, "source");
    if (!source) return false;

    const drive_mt *target = find_named(vec, disk_dest, "target");
    if (!target) return false;

    if (source == target) {
        ERR(ERRC_InvalidInput, NULL, "Source and target are the same device (%s)", source->name);
        return false;
    }

    const bool target_in_source = strcmp(target->parent, source->name) == 0;
    if (target_in_source || strcmp(source->parent, target->name) == 0) {
        ERR(ERRC_InvalidInput, "Source and target overlap", "%s is a partition of %s", target_in_source ? target->name : source->name, target_in_source ? source->name : target->name);
        return false;
    }

    const drive_mt *mounted = find_mounted(vec, source);
    if (mounted) {
        ERR(ERRC_DeviceBusy, "Refusing to clone a mounted device", "%s is mounted at %s, unmount it first so it can't change while it's copied", mounted->name, mounted->mountpoint);
        return false;
    }

    mounted = find_mounted(vec, target);
    if (mounted) {
        ERR(ERRC_DeviceBusy, "Refusing to overwrite a mounted device", "%s is mounted at %s, unmount it first", mounted->name, mounted->mountpoint);
        return false;
    }

    *src = source;
    *dest = target;
    return true;
}



#define CLONE_SYNC_CHUNK (uint64_t)(64 * 1024 * 1024)
#define PROGRESS_INTERVAL 0.25
#define LOG_MSG_SIZE 256

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/**
 * @brief Copies the first @p size bytes of @p src to @p dst and flushes them to the target, with a progress line.
 * @param src    source drive, opened with CLONE_READONLY
 * @param dst    target drive, opened with CLONE_WRITE, at least @p size bytes big
 * @param size   number of bytes to copy (the size of the source drive)
 * @param copied set to the number of bytes copied; on failure, where the copy stopped
 * @return true on success, false on failure with errno set (ENODATA: source ended early, ENOSPC: target full)
 */
static bool clone_copy(fd_t src, fd_t dst, uint64_t size, uint64_t *copied) {
    static char buffer[CLONE_BUFFER_SIZE];
    uint64_t started = 0, flushed = 0;
    const double start = now_seconds();
    double last_print = start;

    *copied = 0;
    cl_print_progress(0, size, 0.0);
    while (*copied < size) {
        const size_t to_read = (size_t)MIN((uint64_t)sizeof(buffer), size - *copied);
        const ssize_t got = pread(src, buffer, to_read, (off_t)*copied);

        if (got < 0 && errno == EINTR) continue;
        if (got <= 0) { if (got == 0) errno = ENODATA; return false; }

        size_t done = 0;
        while (done < (size_t)got) {
            const ssize_t put = pwrite(dst, buffer + done, (size_t)got - done, (off_t)(*copied + done));

            if (put < 0 && errno == EINTR) continue;
            if (put <= 0) { if (put == 0) errno = ENOSPC; return false; }
            done += (size_t)put;
        }
        *copied += (uint64_t)got;

        if (*copied - started < CLONE_SYNC_CHUNK && *copied < size) continue;
        if (started > flushed && sync_file_range(dst, (off_t)flushed, (off_t)(started - flushed), SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WRITE | SYNC_FILE_RANGE_WAIT_AFTER) != 0) return false;
        flushed = started;
        if (sync_file_range(dst, (off_t)started, (off_t)(*copied - started), SYNC_FILE_RANGE_WRITE) != 0) return false;
        started = *copied;

        const double now = now_seconds();
        if (now - last_print < PROGRESS_INTERVAL) continue;
        cl_print_progress(flushed, size, now - start);
        last_print = now;
    }

    if (fsync(dst) != 0) return false;
    cl_print_progress(size, size, now_seconds() - start);
    putchar('\n');
    return true;
}

static const char *copy_error(int32_t error) {
    if (error == ENODATA) return "the source ended early";
    if (error == ENOSPC) return "the target is full";
    return strerror(error);
}



bool disk_clone(const char *disk_dest, const char *disk_src) {
    if (!is_root()) {
        ERR(ERRC_PermissionDenied, NULL, "This function requires root privileges. Please run with 'sudo'");
        return false;
    }

    const fd_t *disks_array = NULL;
    const drive_mt *src = NULL, *dest = NULL;

    uint64_t copied = 0;
    uint64_t src_size = 0;
    uint64_t dest_size = 0;

    Vec disks_vec;
    if (!vec_init(&disks_vec, 8, sizeof(drive_mt))) {
        ERR(ERRC_AllocationFault, NULL, "Failed to init vec for cloning");
        return false;
    }

    if (!get_physical_disk_info(&disks_vec)) {
        goto _exit_failure;
    }

    if (!find_pair(&disks_vec, disk_src, disk_dest, &src, &dest)) {
        goto _exit_failure;
    }

    disks_array = open_disks(src->name, dest->name);
    if (!disks_array) {
        goto _exit_failure;
    }

    if (!get_sizes(disks_array, src->name, dest->name, &src_size, &dest_size)) {
        goto _exit_failure;
    }

    const bool dest_is_disk = dest->parent[0] == '\0';
    if (src->parent[0] == '\0' && dest_is_disk && !check_sector_sizes(disks_array, src->name, dest->name)) {
        goto _exit_failure;
    }

    cl_print_plan(&disks_vec, src, dest, src_size, dest_size);

    char log_msg[LOG_MSG_SIZE];
    if (!globals.bypass_security_code) {
        if (!isatty(STDIN_FILENO)) {
            ERR(ERRC_NoInput, "Cannot ask for confirmation", "stdin is not a terminal, refusing to overwrite %s", dest->name);
            goto _exit_failure;
        }

        if (!cl_confirm_clone(src->name, dest->name, dest_size)) {
            cl_print_cancelled();
            snprintf(log_msg, sizeof(log_msg), "Clone of %s to %s cancelled by user", src->name, dest->name);
            LOG_INFO(log_msg);
            goto _exit_success;
        }
    }

    snprintf(log_msg, sizeof(log_msg), "Clone of %s to %s (%" PRIu64 " bytes) started%s", src->name, dest->name, src_size, globals.bypass_security_code ? " without confirmation" : "");
    LOG_INFO(log_msg);

    putchar('\n');
    const double start = now_seconds();
    if (!clone_copy(disks_array[CLONE_SRC_IDX], disks_array[CLONE_DEST_IDX], src_size, &copied)) {
        const int32_t error = errno;
        putchar('\n');
        ERR(ERRC_IOError, "Clone failed, the target is now incomplete", "%s → %s stopped at byte %" PRIu64 ": %s", src->name, dest->name, copied, copy_error(error));
        goto _exit_failure;
    }
    const double seconds = now_seconds() - start;

    const bool replug_hint = dest_is_disk && ioctl(disks_array[CLONE_DEST_IDX], BLKRRPART) != 0;

    snprintf(log_msg, sizeof(log_msg), "Cloned %s to %s (%" PRIu64 " bytes)", src->name, dest->name, src_size);
    LOG_INFO(log_msg);
    cl_print_done(src->name, dest->name, src_size, dest_size, seconds, replug_hint);
    goto _exit_success;

_exit_failure:
    close_disks(disks_array);
    vec_free(&disks_vec);
    free((void *)disks_array);
    return false;

_exit_success:
    close_disks(disks_array);
    vec_free(&disks_vec);
    free((void *)disks_array);
    return true;
}