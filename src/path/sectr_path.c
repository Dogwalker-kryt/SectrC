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
        ERR(ERRC_AllocationFault, "Failed to allocate memory for elf_run_path", "malloc() failed");
        return NULL;
    }

    __uint64_t len = readlink("/proc/self/exe", path, PATH_MAX - 1);

    if (len == -1) {
        ERR(ERRC_ProcessFailure, "Failed to read the path of the running executable", "readlink() failed");
        free(path);
        return NULL;
    }

    path[len] = '\0';

    return path;
}


void free_path(path_t *path_obj) {
    if (unlikely(path_obj == NULL)) {
        ERR(ERRC_InvalidInput, "Invalid input to free_path", "path_obj is NULL");
        return;
    }

    if (likely(path_obj->path)) {
        free(path_obj->path);
        path_obj->path = NULL;
    }

    path_obj->def = NULL;
    path_obj->fd = -1;
}

void free_paths(path_t *log_path, path_t *config_path) {
    free_path(log_path);
    free_path(config_path);
}


// void cpy_root_to_buff(char *buff, const char *sectr_root) {
//     if (unlikely(buff == NULL || sectr_root == NULL)) {
//         ERR(ERRC_InvalidInput, "Invalid input to cpy_root_to_buff", "buff or sectr_root is NULL");
//         return;
//     }

//     size_t root_len = strlen(sectr_root);
//     if (root_len >= PATH_MAX) {
//         ERR(ERRC_OutOfRange, "sectr_root path is too long", "Length of sectr_root exceeds PATH_MAX");
//         return;
//     }

//     strncpy(buff, sectr_root, PATH_MAX - 1);
//     buff[PATH_MAX - 1] = '\0';
// }