#include "list_drives.h"
#include "../sectr_helper/sectr_helper.h"

void print_disk_infos(const drive_mt *disks, __uint64_t count) {
    for (__uint64_t i = 0; i < count; i++) {
        printf("Device: %s (%s), %llu bytes\n", disks[i].name, disks[i].type, (unsigned long long)disks[i].size);
        printf("  Parent: %s | Model: %s | Vendor: %s | Bus: %s\n",
               disks[i].parent[0] ? disks[i].parent : "-", disks[i].model, disks[i].vendor, disks[i].bus);
        printf("  Filesystem: %s | Label: %s | UUID: %s | Mountpoint: %s\n",
               disks[i].fstype[0] ? disks[i].fstype : "-", disks[i].label[0] ? disks[i].label : "-",
               disks[i].uuid[0] ? disks[i].uuid : "-", disks[i].mountpoint[0] ? disks[i].mountpoint : "-");
        printf("  Serial: %s | Status: %s\n\n",
               disks[i].serial[0] ? disks[i].serial : "-", disks[i].status);
    }
}

