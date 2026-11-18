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

void globals_set_ptr_fd(FILE **dest, FILE *src);

char *build_path_beside_exe(const char *suffix);

char *build_state_path(const char *file);

char *build_config_path(const char *file);

__always_inline void set_globals(void) {
    globals.bypass_security_code = false;
    globals.no_log = false;
    globals.no_color = false;
    globals.stand_alone = false;

    char *log = build_state_path("log.txt");
    // if (!log) log = build_path_beside_exe("/log.txt");
    char *cfg = build_config_path("config.conf");

    globals.log_path.path = log;
    globals.config_path.path = cfg;
} 

bool set_bsc_true(const char *value, void *context);

bool set_nolog_true(const char *value, void *context);

bool set_no_color_true(const char *value, void *context);

bool set_standalone_true(const char *value, void *context);

bool set_no_config_true(const char *value, void *context);

bool set_bypass_security_code_true(const char *value, void *context);