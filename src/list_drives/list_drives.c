#include "list_drives.h"
#include "../read_sys_fs/read_sys_fs.h"
#include "../Debug/sectr_err.h"

#define _POSIX_C_SOURCE 200809L

static bool open_mtab(FILE **out) {
    *out = fopen("/etc/mtab", "r");
    return *out != NULL;
}

static bool get_x(const char *disk_name, char *buffer, enum GET_X_INFO code) {
    FILE *mtab = NULL;
    bool success_open = open_mtab(&mtab);

    if (unlikely(!success_open)) {
        ERR(ERRC_FileNotFound, "Failed to open /etc/mtab", "");
        return false;
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

        if (unlikely(dev == NULL || mount == NULL || fstype == NULL)) continue;
        
        if (unlikely(strstr(dev, disk_name) == NULL)) continue;
    
        switch (code) {
            case FS_TYPE:
                strcpy(buffer, fstype);
                goto cleanup;

            case MOUNT_POINT:
                strcpy(buffer, mount);
                goto cleanup;
        default:
            ERR(ERRC_Unknown, "Unknown code in get_x", "");
            break;
        }

        cleanup:
        buffer[strcspn(buffer, "\n")] = '\0';
        break;
    }

    free(line);
    fclose(mtab);
    return true;
}

bool get_mount_point(const char *disk_name, char *buffer) {
    return get_x(disk_name, buffer, MOUNT_POINT) != false;
}

bool get_FS(const char *disk_name, char *buffer) {
    return get_x(disk_name, buffer, FS_TYPE) != false;
}