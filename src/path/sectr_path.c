#include "sectr_path.h"

path_t path_init(const char *path, const char *def) {
    path_t path_obj = { NULL, NULL, -1 };

    if (__builtin_expect(path == NULL, 0)) {
        return ;
    }

    path_obj.path = strdup(path);
    path_obj.def = def ? strdup(def) : NULL;
    path_obj.fd = -1;

    return path_obj;
}

char *elf_run_path() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path,  sizeof(path) - 1);

    if (len == -1) {
        // TODO: ERR() es LOG_ERR()
        return NULL;
    }

    path[len] = '\0';

    return path;
}