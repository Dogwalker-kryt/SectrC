#pragma once

#include "../defs.h"
#include "../globals.h"

/* Same size as the name fields in drive_mt (disk_data.h). */
#define DEVICE_NAME_SIZE 64

void ul_to_str(char *buff, size_t buff_size, uint64_t d);

/* "/dev/nvme0n1", "/dev/nvme0n1/" and "nvme0n1" → "nvme0n1" in buf.
   Returns buf, or NULL (buf = "") if there is no name or it doesn't fit. */
char *device_basename(const char *device_name, char *buf, size_t buf_size);

__always_inline void cpy_root_to_buff(char *buff, const char *dmgr_root) {
    strcpy(buff, dmgr_root);
}

bool clear_logs(void);
bool view_logs(size_t lines_to_count);

char *convert_to_GiB(size_t bytes);

char *llu_to_str(__llu value, char *buffer, size_t size);