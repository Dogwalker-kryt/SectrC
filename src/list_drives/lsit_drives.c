#include "list_drives.h"
#include "../sectr_helper/sectr_helper.h"

static const char *device_basename(const char *device_name) {
    const char *basename = strrchr(device_name, '/');
    return basename ? basename + 1 : device_name;
}

bool print_disk_infos(const drive_mt *disks, __uint64_t count, const char *disk_name) {
    __uint64_t printed_count = 0;
    for (__uint64_t i = 0; i < count; i++) {
        bool matches = !disk_name || disk_name[0] == '\0' ||
            strcmp(device_basename(disk_name), device_basename(disks[i].name)) == 0 ||
            (disks[i].parent[0] != '\0' && strcmp(device_basename(disk_name), device_basename(disks[i].parent)) == 0);
        if (!matches) continue;

        printf("Device: %s (%s), %llu bytes\n", disks[i].name, disks[i].type, (unsigned long long)disks[i].size);
        printf("  Parent: %s | Model: %s | Vendor: %s | Bus: %s\n",
                disks[i].parent[0] ? disks[i].parent : "-", disks[i].model, disks[i].vendor, disks[i].bus);
        printf("  Filesystem: %s | Label: %s | UUID: %s | Mountpoint: %s\n",
                disks[i].fstype[0] ? disks[i].fstype : "-", disks[i].label[0] ? disks[i].label : "-",
                disks[i].uuid[0] ? disks[i].uuid : "-", disks[i].mountpoint[0] ? disks[i].mountpoint : "-");
        printf("  Serial: %s | Status: %s\n\n", disks[i].serial[0] ? disks[i].serial : "-", disks[i].status);
        ++printed_count;
    }

    return printed_count > 0;
}

