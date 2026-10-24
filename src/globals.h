#pragma once

#include "defs.h"
#include "path/sectr_path.h"

typedef struct globals_t {
    bool bypass_security_code;
    bool no_log;
    bool no_color;
    bool stand_alone;

    path_t log_path;
    path_t config_path;
} globals_t;

extern globals_t globals;