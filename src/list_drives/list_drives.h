#pragma once

#include "../defs.h"

static bool open_mtab(FILE **out);

void get_mount_point(const char *disk_name, char *buffer);
