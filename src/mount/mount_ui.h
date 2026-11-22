#pragma once

#include "../ui/ui.h"
#include "../disk_data/disk_data.h"

void print_mounted(const drive_mt *drive, const char *mountpoint, bool read_only, bool created);
