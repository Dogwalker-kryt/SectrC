#pragma once

#include "../defs.h"
#include "../ui/ui.h"
#include "../disk_data/disk_data.h"

void print_progress(uint64_t done, uint64_t total);

void print_erase_list(const Vec *vec, const drive_mt *target);
/* "! erase all data on /dev/sdX (size)? [y/N]", true only on an explicit yes. */
bool ow_confirm_erase(const char *device_path, const char *size_str);

void ow_print_cancelled(void);

void ow_print_done(const char *device_path, const char *size_str, const char *method);
