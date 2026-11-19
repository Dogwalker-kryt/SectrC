#pragma once

#include "../defs.h"
#include "../ui/ui.h"
#include "../disk_data/disk_data.h"

void print_progress(uint64_t done, uint64_t total);

void print_erase_list(const Vec *vec, const drive_mt *target);