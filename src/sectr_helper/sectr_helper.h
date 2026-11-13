#pragma once

#include "../defs.h"
#include "../globals.h"

void ul_to_str(char *buff, size_t buff_size, uint64_t d);

__always_inline void cpy_root_to_buff(char *buff, const char *dmgr_root) {
    strcpy(buff, dmgr_root);
}

bool clear_logs(void);
bool view_logs(size_t lines_to_count);

char *convert_to_GiB(size_t bytes);