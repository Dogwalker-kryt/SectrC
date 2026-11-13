#pragma once

#include "../defs.h"
#include "../globals.h"

void ul_to_str(char *buff, size_t buff_size, uint64_t d);

__always_inline void cpy_root_to_buff(char *buff, const char *dmgr_root) {
    strcpy(buff, dmgr_root);
}

void clear_logs(void);
void view_logs(uint64_t lines_to_count);

char *convert_to_GiB(uint64_t bytes);