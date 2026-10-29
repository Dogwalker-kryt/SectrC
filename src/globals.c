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
    __uint64_t dir_len = slash ? (__uint64_t)(slash - exe) : 0;

    char *out = (char*)malloc(PATH_MAX);
    if (out == NULL) {
        free(exe);
        ERR(ERRC_AllocationFault, "failed to build path", "malloc() failed");
        return NULL;
    }

    int n = snprintf(out, PATH_MAX, "%.*s%s", (int)dir_len, exe, suffix);
    free(exe);

    if (n < 0 || (__uint64_t)n >= PATH_MAX) {
        free(out);
        ERR(ERRC_OutOfRange, "built path is too long", suffix);
        return NULL;
    }

    return out;
}

