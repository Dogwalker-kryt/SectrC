#include "disk_data.h"
#include "../Debug/sectr_err.h"
#include <blkid/blkid.h>
#include <libudev.h>
#include <errno.h>
#include <limits.h>
#include <mntent.h>

const char *scsi_device_type_name(const char *disk_name, uint8_t type) {
    switch (type) {
        case 0x00: return "disk";
        case 0x01: return "tape";
        case 0x05: return "cd/dvd";
        case 0x07: return "magneto-optical";
        case 0x08: return "medium changer";
        case 0x0c: return "RAID";
        case 0x0d: return "enclosure";
        case 0x14: return "zoned block device";
        case 0x7f: return "no LUN";
        default: return disk_name && strstr(disk_name, "nvme") != NULL ? "nvme" : "unknown";
    }
}

static void copy_value(char *destination, size_t destination_size, const char *value) {
    if (destination_size == 0) return;
    snprintf(destination, destination_size, "%s", value ? value : "");
}

static bool copy_blkid_value(blkid_probe probe, const char *tag, char *destination, size_t destination_size) {
    const char *value = NULL;
    size_t value_length = 0;
    if (blkid_probe_lookup_value(probe, tag, &value, &value_length) != 0 || !value || value_length == 0) return false;
    snprintf(destination, destination_size, "%.*s", (int)value_length, value);
    return destination[0] != '\0';
}

static void populate_blkid_data(drive_mt *disk) {
    blkid_probe probe = blkid_new_probe_from_filename(disk->name);
    if (!probe) return;
    if (blkid_do_safeprobe(probe) == 0) {
        copy_blkid_value(probe, "TYPE", disk->fstype, sizeof(disk->fstype));
        copy_blkid_value(probe, "UUID", disk->uuid, sizeof(disk->uuid));
        copy_blkid_value(probe, "LABEL", disk->label, sizeof(disk->label));
    }
    blkid_free_probe(probe);
}

static bool get_x(const char *disk_name, char *buffer, size_t buffer_size, enum GET_X_INFO code) {
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

        size_t value_length = strlen(value);
        if (value_length >= buffer_size) {
            ERR(ERRC_OutOfRange, "Mount information buffer is too small", "Required %zu bytes, have %zu", value_length + 1, buffer_size);
            endmntent(mtab);
            return false;
        }

        memcpy(buffer, value, value_length + 1);
        found = true;
        break;
    }

    endmntent(mtab);
    return found;
}

bool get_mount_point(const char *disk_name, char *buffer, size_t buffer_size) {
    return get_x(disk_name, buffer, buffer_size, MOUNT_POINT);
}

bool get_FS(const char *disk_name, char *buffer, size_t buffer_size) {
    if (unlikely(!disk_name || !buffer || buffer_size == 0)) {
        ERR(ERRC_InvalidInput, "Invalid filesystem lookup arguments", "disk_name, buffer, and buffer_size are required");
        return false;
    }
    buffer[0] = '\0';
    blkid_probe probe = blkid_new_probe_from_filename(disk_name);
    if (!probe) return false;
    bool found = blkid_do_safeprobe(probe) == 0 && copy_blkid_value(probe, "TYPE", buffer, buffer_size);
    blkid_free_probe(probe);
    return found;
}

Vec *get_physical_disk_info(Vec *vec) {
    if (!vec || vec->element_size != sizeof(drive_mt)) {
        ERR(ERRC_InvalidInput, "Invalid physical-disk lookup arguments", "An initialized drive vector is required");
        return NULL;
    }

    struct udev *udev = udev_new();
    if (!udev) {
        ERR(ERRC_AllocationFault, "Failed to initialize libudev", "udev_new() failed");
        return NULL;
    }

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    if (!enumerate) {
        udev_unref(udev);
        ERR(ERRC_AllocationFault, "Failed to initialize block-device enumeration", "udev_enumerate_new() failed");
        return NULL;
    }
    if (udev_enumerate_add_match_subsystem(enumerate, "block") < 0 || udev_enumerate_scan_devices(enumerate) < 0) {
        udev_enumerate_unref(enumerate);
        udev_unref(udev);
        ERR(ERRC_IOError, "Failed to enumerate block devices", "libudev could not scan block devices");
        return NULL;
    }

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;
    udev_list_entry_foreach(entry, devices) {
        struct udev_device *device = udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
        if (!device) continue;
        const char *devtype = udev_device_get_devtype(device);
        const char *devnode = udev_device_get_devnode(device);
        const char *sysname = udev_device_get_sysname(device);
        bool is_disk = devtype && strcmp(devtype, "disk") == 0;
        bool is_partition = devtype && strcmp(devtype, "partition") == 0;
        if ((!is_disk && !is_partition) || !devnode || !sysname || strncmp(sysname, "loop", 4) == 0 || strncmp(sysname, "ram", 3) == 0 || strncmp(sysname, "zram", 4) == 0) {
            udev_device_unref(device);
            continue;
        }

        struct udev_device *parent = is_disk ? device : udev_device_get_parent_with_subsystem_devtype(device, "block", "disk");
        const char *parent_name = parent ? udev_device_get_sysname(parent) : NULL;
        if (parent_name && (strncmp(parent_name, "loop", 4) == 0 || strncmp(parent_name, "ram", 3) == 0 || strncmp(parent_name, "zram", 4) == 0)) {
            udev_device_unref(device);
            continue;
        }

        drive_mt disk = {0};
        copy_value(disk.name, sizeof(disk.name), devnode);
        if (is_partition && parent && udev_device_get_devnode(parent)) copy_value(disk.parent, sizeof(disk.parent), udev_device_get_devnode(parent));
        copy_value(disk.type, sizeof(disk.type), is_partition ? "partition" : "");
        copy_value(disk.model, sizeof(disk.model), udev_device_get_property_value(device, "ID_MODEL"));
        copy_value(disk.vendor, sizeof(disk.vendor), udev_device_get_property_value(device, "ID_VENDOR"));
        copy_value(disk.serial, sizeof(disk.serial), udev_device_get_property_value(device, "ID_SERIAL_SHORT"));
        if (disk.serial[0] == '\0') copy_value(disk.serial, sizeof(disk.serial), udev_device_get_property_value(device, "ID_SERIAL"));
        copy_value(disk.bus, sizeof(disk.bus), udev_device_get_property_value(device, "ID_BUS"));
        if (disk.model[0] == '\0') copy_value(disk.model, sizeof(disk.model), udev_device_get_property_value(device, "ID_MODEL_FROM_DATABASE"));
        if (disk.model[0] == '\0') copy_value(disk.model, sizeof(disk.model), udev_device_get_sysattr_value(device, "device/model"));
        if (disk.vendor[0] == '\0') copy_value(disk.vendor, sizeof(disk.vendor), udev_device_get_property_value(device, "ID_VENDOR_FROM_DATABASE"));
        if (disk.vendor[0] == '\0') copy_value(disk.vendor, sizeof(disk.vendor), udev_device_get_sysattr_value(device, "device/vendor"));
        if (disk.serial[0] == '\0') copy_value(disk.serial, sizeof(disk.serial), udev_device_get_sysattr_value(device, "device/serial"));
        if (disk.bus[0] == '\0' && strncmp(sysname, "nvme", 4) == 0) copy_value(disk.bus, sizeof(disk.bus), "nvme");
        if (parent && parent != device) {
            if (disk.model[0] == '\0') copy_value(disk.model, sizeof(disk.model), udev_device_get_property_value(parent, "ID_MODEL"));
            if (disk.vendor[0] == '\0') copy_value(disk.vendor, sizeof(disk.vendor), udev_device_get_property_value(parent, "ID_VENDOR"));
            if (disk.serial[0] == '\0') copy_value(disk.serial, sizeof(disk.serial), udev_device_get_property_value(parent, "ID_SERIAL_SHORT"));
            if (disk.serial[0] == '\0') copy_value(disk.serial, sizeof(disk.serial), udev_device_get_property_value(parent, "ID_SERIAL"));
            if (disk.bus[0] == '\0') copy_value(disk.bus, sizeof(disk.bus), udev_device_get_property_value(parent, "ID_BUS"));
            if (disk.model[0] == '\0') copy_value(disk.model, sizeof(disk.model), udev_device_get_property_value(parent, "ID_MODEL_FROM_DATABASE"));
            if (disk.model[0] == '\0') copy_value(disk.model, sizeof(disk.model), udev_device_get_sysattr_value(parent, "device/model"));
            if (disk.vendor[0] == '\0') copy_value(disk.vendor, sizeof(disk.vendor), udev_device_get_property_value(parent, "ID_VENDOR_FROM_DATABASE"));
            if (disk.vendor[0] == '\0') copy_value(disk.vendor, sizeof(disk.vendor), udev_device_get_sysattr_value(parent, "device/vendor"));
            if (disk.serial[0] == '\0') copy_value(disk.serial, sizeof(disk.serial), udev_device_get_sysattr_value(parent, "device/serial"));
            if (disk.bus[0] == '\0' && parent_name && strncmp(parent_name, "nvme", 4) == 0) copy_value(disk.bus, sizeof(disk.bus), "nvme");
        }
        copy_value(disk.uuid, sizeof(disk.uuid), udev_device_get_property_value(device, "ID_FS_UUID"));
        copy_value(disk.fstype, sizeof(disk.fstype), udev_device_get_property_value(device, "ID_FS_TYPE"));
        copy_value(disk.label, sizeof(disk.label), udev_device_get_property_value(device, "ID_FS_LABEL"));
        copy_value(disk.status, sizeof(disk.status), udev_device_get_is_initialized(device) ? "available" : "unknown");

        const char *sectors_value = udev_device_get_sysattr_value(device, "size");
        if (sectors_value) {
            char *end = NULL;
            errno = 0;
            unsigned long long sectors = strtoull(sectors_value, &end, 10);
            if (errno == 0 && end != sectors_value && *end == '\0' && sectors <= ULLONG_MAX / 512) disk.size = (uint64_t)sectors * 512;
        }

        const char *type_value = is_disk ? udev_device_get_sysattr_value(device, "device/type") : NULL;
        if (is_disk && type_value) {
            char *end = NULL;
            unsigned long scsi_type = strtoul(type_value, &end, 10);
            if (end != type_value && *end == '\0' && scsi_type <= 0xff) copy_value(disk.type, sizeof(disk.type), scsi_device_type_name(sysname, (uint8_t)scsi_type));
        }
        if (is_disk && disk.type[0] == '\0') copy_value(disk.type, sizeof(disk.type), scsi_device_type_name(sysname, 0xff));

        populate_blkid_data(&disk);
        get_mount_point(disk.name, disk.mountpoint, sizeof(disk.mountpoint));

        if (!vec_add_element(vec, &disk)) {
            udev_device_unref(device);
            udev_enumerate_unref(enumerate);
            udev_unref(udev);
            ERR(ERRC_AllocationFault, "Failed to add drive to vector", "%s", disk.name);
            return NULL;
        }
        udev_device_unref(device);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    return vec;
}