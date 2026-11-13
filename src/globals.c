#include "globals.h"
#include "Debug/sectr_err.h"

globals_t globals = {
    false, false, false, false,
    {NULL, NULL, NULL},
    {NULL, NULL, NULL}
};

char *build_path_beside_exe(const char *suffix) {
    char *exe = elf_run_path();
    if (!exe) return NULL;

    char *slash = strrchr(exe, '/');
    size_t dir_len = slash ? (size_t)(slash - exe) : 0;

    char *path_buffer = (char*)malloc(PATH_MAX);
    if (path_buffer == NULL) {
        free(exe);
        ERR(ERRC_AllocationFault, "failed to build path", "malloc() failed");
        return NULL;
    }

    int n = snprintf(path_buffer, PATH_MAX, "%.*s%s", (int)dir_len, exe, suffix);
    free(exe);

    if (n < 0 || (size_t)n >= PATH_MAX) {
        free(path_buffer);
        ERR(ERRC_OutOfRange, "built path is too long", "%s", suffix);
        return NULL;
    }

    return path_buffer;
}

void set_bsc_true() {
    globals.bypass_security_code = true; 
}

void set_nolog_true() {
    globals.no_log = true; 
}

void set_nocolor_true() {
    globals.no_color = true; 
}

void set_standalone_true() {
    globals.stand_alone = true; 
}

const char *sectr_color(const char *code) {
    return globals.no_color ? "" : code;
}
