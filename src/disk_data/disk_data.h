#pragma once

#include "../defs.h"
#include "../vector/vector.h"

#define drive_mt_member_size 13

typedef struct drive_mt {
    char name[64];
    char model[64];
    char type[64];
    char mountpoint[64];
    char vendor[64];
    char fstype[64];
    char status[64];
    char serial[128];
    char uuid[128];
    char label[128];
    char bus[32];
    char parent[64];
    
    uint64_t size;

} drive_mt;

const char *scsi_device_type_name(const char *disk_name, uint8_t type);

enum GET_X_INFO {
    FS_TYPE,
    MOUNT_POINT
};

bool get_mount_point(const char *disk_name, char *buffer, size_t buffer_size);

bool get_FS(const char *disk_name, char *buffer, size_t buffer_size);

/* Adds physical disks and their partitions to an initialized drive_mt vector. */
Vec *get_physical_disk_info(Vec *vec);