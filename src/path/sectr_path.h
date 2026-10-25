#pragma once

#include <sys/stat.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <fcntl.h>
#include "../defs.h"
#include "../Debug/sectr_err.h"

typedef struct path_t {
    char *path;
    char *def;
    FILE *fd;

} path_t;

path_t path_init(const char *path, const char *def);

#define PATH_INIT_DEFAULT path_init(NULL, NULL)

char *elf_run_path();

void free_path(path_t *path_obj);

