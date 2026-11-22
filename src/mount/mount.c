#include "mount.h"
#include "mount_ui.h"
#include "../defs.h"
#include "../globals.h"
#include "../Debug/sectr_err.h"
#include "../disk_data/disk_data.h"
#include "../sectr_helper/sectr_helper.h"
#include "../security/permission.h"
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>

#define MOUNT_DIR_MODE 0755
#define MOUNT_FLAGS (MS_NOSUID | MS_NODEV)
#define MOUNT_DATA_SIZE 64
#define MAX_ID_DIGITS 10
#define LOG_MSG_SIZE 512

static bool has_partitions(const Vec *vec, const drive_mt *drive) {
    const drive_mt *drives = vec->data;

    for (uint32_t i = 0; i < vec->element_count; ++i) {
        if (strcmp(drives[i].parent, drive->name) == 0) return true;
    }
    return false;
}

static bool check_mountable(const Vec *vec, const drive_mt *drive) {
    static const char *const unmountable[][2] = {
        {"swap", "is swap space"},
        {"crypto_LUKS", "is encrypted (LUKS), unlock it first"},
        {"BitLocker", "is encrypted (BitLocker)"},
        {"LVM2_member", "is an LVM volume, mount its logical volume instead"},
        {"linux_raid_member", "is part of a RAID, mount the RAID device instead"},
    };

    if (drive->mountpoint[0]) {
        ERR(ERRC_DeviceBusy, NULL, "%s is already mounted at %s", drive->name, drive->mountpoint);
        return false;
    }

    if (drive->fstype[0] == '\0') {
        if (drive->parent[0] == '\0' && has_partitions(vec, drive)) ERR(ERRC_InvalidDevice, NULL, "%s is a whole disk, mount one of its partitions instead", drive->name);
        else ERR(ERRC_InvalidDevice, NULL, "No filesystem found on %s", drive->name);
        return false;
    }

    for (size_t i = 0; i < SIZEOF_ARRAY(unmountable); ++i) {
        if (strcmp(drive->fstype, unmountable[i][0]) != 0) continue;
        ERR(ERRC_InvalidDevice, NULL, "Cannot mount %s, it %s", drive->name, unmountable[i][1]);
        return false;
    }
    return true;
}

static bool prepare_mountpoint(const char *mountpoint, bool *created) {
    *created = false;

    if (mountpoint[0] != '/') {
        ERR(ERRC_InvalidInput, NULL, "The mount point must be an absolute path like /mnt/usb, got '%s'", mountpoint);
        return false;
    }

    struct stat st;
    if (stat(mountpoint, &st) != 0) {
        if (errno != ENOENT) {
            ERR(ERRC_IOError, NULL, "Cannot use %s as mount point: %s", mountpoint, strerror(errno));
            return false;
        }
        if (mkdir(mountpoint, MOUNT_DIR_MODE) != 0) {
            ERR(ERRC_IOError, NULL, "Failed to create the mount point %s: %s", mountpoint, strerror(errno));
            return false;
        }
        *created = true;
        return true;
    }

    if (!S_ISDIR(st.st_mode)) {
        ERR(ERRC_InvalidInput, NULL, "%s is not a folder", mountpoint);
        return false;
    }

    DIR *dir = opendir(mountpoint);
    if (!dir) {
        ERR(ERRC_IOError, NULL, "Cannot open %s: %s", mountpoint, strerror(errno));
        return false;
    }

    bool empty = true;
    const struct dirent *entry;
    while (empty && (entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) empty = false;
    }
    closedir(dir);

    if (!empty) {
        ERR(ERRC_InvalidInput, NULL, "%s is not empty, mounting there would hide its files. Use an empty folder", mountpoint);
        return false;
    }
    return true;
}

static bool is_id(const char *text) {
    if (!text || text[0] == '\0' || strlen(text) > MAX_ID_DIGITS) return false;
    for (const char *c = text; *c; ++c) {
        if (*c < '0' || *c > '9') return false;
    }
    return true;
}

static void owner_options(const char *fstype, char *data, size_t data_size) {
    data[0] = '\0';
    if (strcmp(fstype, "vfat") != 0 && strcmp(fstype, "exfat") != 0 && strcmp(fstype, "ntfs") != 0) return;

    const char *uid = getenv("SUDO_UID");
    const char *gid = getenv("SUDO_GID");
    if (is_id(uid) && is_id(gid)) snprintf(data, data_size, "uid=%s,gid=%s", uid, gid);
}

static bool do_mount(const drive_mt *drive, const char *mountpoint, bool *read_only) {
    const char *fstype = strcmp(drive->fstype, "ntfs") == 0 ? "ntfs3" : drive->fstype;
    char data[MOUNT_DATA_SIZE];
    owner_options(drive->fstype, data, sizeof(data));
    const char *options = data[0] ? data : NULL;

    *read_only = false;
    if (mount(drive->name, mountpoint, fstype, MOUNT_FLAGS, options) == 0) return true;

    if (errno == EROFS || errno == EACCES) {
        *read_only = true;
        if (mount(drive->name, mountpoint, fstype, MOUNT_FLAGS | MS_RDONLY, options) == 0) return true;
    }

    const int32_t error = errno;
    if (error == EBUSY) ERR(ERRC_DeviceBusy, NULL, "Failed to mount %s, it is in use (mounted, swap, LVM or RAID)", drive->name);
    else if (error == ENODEV) ERR(ERRC_InvalidDevice, NULL, "Failed to mount %s, the kernel has no driver for %s", drive->name, fstype);
    else if (error == EINVAL || error == EUCLEAN) ERR(ERRC_CorruptedData, NULL, "Failed to mount %s, the %s filesystem looks damaged (try fsck)", drive->name, drive->fstype);
    else ERR(ERRC_IOError, NULL, "Failed to mount %s at %s: %s", drive->name, mountpoint, strerror(error));
    return false;
}

bool disk_mount(const char *disk_name, const char *mountpoint) {
    if (!is_root()) {
        ERR(ERRC_PermissionDenied, NULL, "This function requires root privileges. Please run with 'sudo'");
        return false;
    }

    bool created = false;

    Vec vec;
    if (!vec_init(&vec, DEFAULT_EXPECTED_ELEMENTS_COUNT, sizeof(drive_mt))) {
        ERR(ERRC_AllocationFault, NULL, "Failed to init vec for mounting");
        return false;
    }

    if (!get_physical_disk_info(&vec)) {
        goto _exit_failure;
    }

    char basename[DEVICE_NAME_SIZE];
    if (!device_basename(disk_name, basename, sizeof(basename))) {
        ERR(ERRC_InvalidInput, NULL, "'%s' is not a valid disk name", disk_name);
        goto _exit_failure;
    }

    const drive_mt *drive = find_drive(&vec, basename);
    if (!drive) {
        ERR(ERRC_DeviceNotFound, NULL, "No disk or partition named '%s'", basename);
        goto _exit_failure;
    }

    if (!check_mountable(&vec, drive)) {
        goto _exit_failure;
    }

    if (!prepare_mountpoint(mountpoint, &created)) {
        goto _exit_failure;
    }

    bool read_only = false;
    if (!do_mount(drive, mountpoint, &read_only)) {
        goto _exit_failure;
    }

    char log_msg[LOG_MSG_SIZE];
    snprintf(log_msg, sizeof(log_msg), "Mounted %s (%s) at %s%s", drive->name, drive->fstype, mountpoint, read_only ? " read-only" : "");
    LOG_INFO(log_msg);

    print_mounted(drive, mountpoint, read_only, created);
    vec_free(&vec);
    return true;

_exit_failure:
    if (created) rmdir(mountpoint);
    vec_free(&vec);
    return false;
}

bool disk_unmount(const char *path) {
    if (!is_root()) {
        ERR(ERRC_PermissionDenied, NULL, "This function requires root privileges. Please run with 'sudo'");
        return false;
    }

    errno = 0;

    if (umount(path) != 0) {        
        ERR(ERRC_ProcessFailure, NULL, "%s", strerror(errno));
        return false;
    }

    return true;
}
