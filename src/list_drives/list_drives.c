#include "list_drives.h"
#include "../read_sys_fs/read_sys_fs.h"
#include "../Debug/sectr_err.h"

#define _POSIX_C_SOURCE 200809L

static bool open_mtab(FILE **out) {
    *out = fopen("/etc/mtab", "r");
    return *out != NULL;
}

void get_mount_point(const char *disk_name, char *buffer) {
    FILE *mtab = NULL;
    bool success_open = open_mtab(&mtab);

    if (!success_open) {
        ERR(ERRC_FileNotFound, "Failed to open /etc/mtab", "");
        return;
    }

    char *line = NULL;
    __uint64_t capacity = 0;
    __int64_t len;

    buffer[0] = '\0';

    while ((len = getline(&line, &capacity, mtab)) != -1) {
        char *saveptr;
        char *dev = strtok_r(line, " ", &saveptr);
        char *mount = strtok_r(NULL, " ", &saveptr);
        char *fstype = strtok_r(NULL, " ", &saveptr);

        if (dev != NULL && mount != NULL && fstype != NULL) {
            if (strstr(dev, disk_name) != NULL) {
                strcpy(buffer, mount);

                // Remove trailing newline
                buffer[strcspn(buffer, "\n")] = '\0';

                break;
            }
        }
    }

    free(line);
    fclose(mtab);
}