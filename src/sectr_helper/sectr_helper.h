#pragma once

#include "../defs.h"
#include "../globals.h"

__always_inline void cpy_root_to_buff(char *buff, const char *dmgr_root) {
    strcpy(buff, dmgr_root);
}

void set_globals(globals *g);