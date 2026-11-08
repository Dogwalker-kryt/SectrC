#pragma once

#include "../defs.h"
#include "../vector/vector.h"

typedef struct drive_mt {
    char name[64];
    char model[64];
    char type[64];
    char mountpoint[64];
    char vendor[64];
    char fstype[64];
    char status[64];
    
    __uint64_t size;
    __uint16_t serial;
    __uint16_t uuid;

} drive_mt;

char* scsi_device_type_name(const char *disk_name, __uint8_t type);

enum GET_X_INFO {
    FS_TYPE,
    MOUNT_POINT
};

bool get_mount_point(const char *disk_name, char *buffer, __uint64_t buffer_size);

bool get_FS(const char *disk_name, char *buffer, __uint64_t buffer_size);

Vec *get_physical_disk_info(Vec *vec);