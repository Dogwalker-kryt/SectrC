#include "sectr_path.h"

path_t path_init(const char *path, const char *def) {
    path_t path_obj = { NULL, NULL, -1 };

    if (unlikely(path == NULL)) {
        return path_obj;
    }

    path_obj.path = __builtin_strdup(path);
    path_obj.def = def ? __builtin_strdup(def) : NULL;
    path_obj.fd = -1;

    return path_obj;
}

char *elf_run_path() {
    char *path = (char*)malloc(PATH_MAX);
    if (path == NULL) {
        return NULL;
    }

    __uint64_t len = readlink("/proc/self/exe", path, PATH_MAX - 1);

    if (len == -1) {
        // TODO: ERR() es LOG_ERR()
        free(path);
        return NULL;
    }

    path[len] = '\0';

    return path;
}