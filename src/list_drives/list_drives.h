#pragma once

#include "../defs.h"

static bool open_mtab(FILE **out);

enum GET_X_INFO {
    FS_TYPE,
    MOUNT_POINT
};

static bool get_x(const char *disk_name, char *buffer, enum GET_X_INFO code);

bool get_mount_point(const char *disk_name, char *buffer);

bool get_FS(const char *disk_name, char *buffer);