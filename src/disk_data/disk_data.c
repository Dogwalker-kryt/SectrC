#define _DEFAULT_SOURCE
#include "disk_data.h"
#include "../read_sys_fs/read_sys_fs.h"
#include "../Debug/sectr_err.h"
#include <mntent.h>

char* scsi_device_type_name(const char *disk_name, __uint8_t type) {
    switch (type) {
        case 0x00: return (char*)"disk";
        case 0x01: return (char*)"tape";
        case 0x05: return (char*)"cd/dvd";
        case 0x07: return (char*)"magneto-optical";
        case 0x08: return (char*)"medium changer";
        case 0x0c: return (char*)"RAID";
        case 0x0d: return (char*)"enclosure";
        case 0x14: return (char*)"zoned block device";
        case 0x7f: return (char*)"no LUN";
        default:  if (strstr(disk_name, "nvme") != NULL) { return (char*)"nvme"; } else { return (char*)"unkown"; };
    }
}

static bool get_x(const char *disk_name, char *buffer, __uint64_t buffer_size, enum GET_X_INFO code) {
    if (unlikely(!disk_name || !buffer || buffer_size == 0)) {
        ERR(ERRC_InvalidInput, "Invalid mount lookup arguments", "disk_name, buffer, and buffer_size are required");
        return false;
    }

    buffer[0] = '\0';
    FILE *mtab = setmntent("/etc/mtab", "r");
    if (unlikely(!mtab)) {
        ERR(ERRC_FileNotFound, "Failed to open mount table", "/etc/mtab");
        return false;
    }

    struct mntent *entry;
    bool found = false;
    while ((entry = getmntent(mtab)) != NULL) {
        if (strcmp(entry->mnt_fsname, disk_name) != 0) continue;

        const char *value = NULL;
        switch (code) {
            case FS_TYPE:
                value = entry->mnt_type;
                break;
            case MOUNT_POINT:
                value = entry->mnt_dir;
                break;
            default:
                ERR(ERRC_Unknown, "Unknown code in get_x", "%d", code);
                endmntent(mtab);
                return false;
        }

        __uint64_t value_length = (__uint64_t)strlen(value);
        if (value_length >= buffer_size) {
            ERR(ERRC_OutOfRange, "Mount information buffer is too small", "Required %lu bytes, have %lu", (unsigned long)(value_length + 1), (unsigned long)buffer_size);
            endmntent(mtab);
            return false;
        }

        memcpy(buffer, value, value_length + 1);
        found = true;
        break;
    }

    endmntent(mtab);
    if (unlikely(!found)) ERR(ERRC_DeviceNotFound, "No mount entry found for device", "%s", disk_name);
    return found;
}

bool get_mount_point(const char *disk_name, char *buffer, __uint64_t buffer_size) {
    return get_x(disk_name, buffer, buffer_size, MOUNT_POINT);
}

bool get_FS(const char *disk_name, char *buffer, __uint64_t buffer_size) {
    return get_x(disk_name, buffer, buffer_size, FS_TYPE);
}

Vec *get_physical_disk_info(const char *disk_name, Vec *vec) {
    if (!disk_name || !vec || vec->element_size != sizeof(drive_mt)) {
        ERR(ERRC_InvalidInput, "Invalid physical-disk lookup arguments", "disk_name and an initialized drive vector are required");
        return NULL;
    }

    const char *sys_block_p = "/sys/block/";
    
    DIR *dir = opendir(sys_block_p);

    if (dir == NULL) {
        ERR(ERRC_FileNotFound, "Failed to open /sys/block/", "");
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *dev = entry->d_name;

        if (dev[0] == '.' || strstr(dev, "loop") != NULL || strstr(dev, "ram") != NULL) continue;

        char device_path[PATH_MAX];
        snprintf(device_path, sizeof(device_path), "/dev/%s", dev);
        if (strcmp(disk_name, device_path) != 0) continue;

        drive_mt disk = {0};
        snprintf(disk.name, sizeof(disk.name), "/dev/%.58s", dev);
        char path_buffer[512];

        snprintf(path_buffer, sizeof(path_buffer), "%s%s/size", sys_block_p, dev);
        disk.size = read_sysfs_uint64(path_buffer) * 512;

        CLEAR_BUFFER(path_buffer);

        snprintf(path_buffer, sizeof(path_buffer), "%s%s/device/type", sys_block_p, dev);
        snprintf(disk.type, sizeof(disk.type), "%s", scsi_device_type_name(dev, (__uint8_t)read_sysfs_uint64(path_buffer)));
        
        CLEAR_BUFFER(path_buffer);

        snprintf(path_buffer, sizeof(path_buffer), "%s%s/stat", sys_block_p, dev);
        snprintf(disk.status, sizeof(disk.status), "%s", access(path_buffer, R_OK) == 0 ? "running" : "unknown");

        if (!vec_add_element(vec, &disk)) {
            closedir(dir);
            ERR(ERRC_AllocationFault, "Failed to add drive to vector", "%s", device_path);
            return NULL;
        }
    }

    closedir(dir);
    return vec;
}