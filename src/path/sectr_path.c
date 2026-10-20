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

