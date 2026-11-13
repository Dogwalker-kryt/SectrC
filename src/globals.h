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

char *build_path_beside_exe(const char *suffix);

/* Returns $XDG_STATE_HOME/sectrc/<file> (default ~/.local/state/sectrc/<file>), creating the folder. */
char *build_state_path(const char *file);

__always_inline void set_globals(void) {
    globals.bypass_security_code = false;
    globals.no_log = false;
    globals.no_color = false;
    globals.stand_alone = false;

    char *log = build_state_path("log.txt");
    if (!log) log = build_path_beside_exe("/log.txt");
    char *cfg = build_path_beside_exe("/config.json");

    globals.log_path.path = log;
    globals.config_path.path = cfg;
} 

void set_bsc_true();

void set_nolog_true();

void set_nocolor_true();

void set_standalone_true();
