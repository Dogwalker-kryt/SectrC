#pragma once

#include "../defs.h"
#include "../disk_data/disk_data.h"

void cl_print_plan(const Vec *vec, const drive_mt *src, const drive_mt *dest, uint64_t src_size, uint64_t dest_size);

bool cl_confirm_clone(const char *src_path, const char *dest_path, uint64_t dest_size);

void cl_print_progress(uint64_t done, uint64_t total, double seconds);

void cl_print_cancelled(void);

void cl_print_done(const char *src_path, const char *dest_path, uint64_t size, uint64_t dest_size, double seconds, bool replug_hint);
