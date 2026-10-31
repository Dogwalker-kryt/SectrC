#include "read_sys_fs.h"
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

static inline bool read_sysfs_text(const char *path, char *buffer, __uint64_t capacity) {
    if (path == NULL || buffer == NULL || capacity == 0) return false;

    buffer[0] = '\0';

    FILE *fp = fopen(path, "r");
    if (fp == NULL) return false;

    __uint64_t length = 0;
    int ch;
    bool ok = true;

    while ((ch = fgetc(fp)) != EOF) {

        if (ch == '\0' || length >= capacity - 1) {
            ok = false; /* Embedded NUL or buffer too small */
            break;
        }

        buffer[length++] = (char)ch;
    }

    if (ferror(fp)) ok = false;

    if (fclose(fp) != 0) ok = false;

    if (!ok) {
        buffer[0] = '\0';
        return false;
    }

    buffer[length] = '\0';
    return true;
}


static inline bool read_sysfs_u64( const char *path, __uint64_t *out) {
    if (unlikely(out == NULL)) return false;

    char buffer[128];
    if (!read_sysfs_text(path, buffer, sizeof buffer))
        return false;

    char *start = buffer;
    while (isspace((unsigned char)*start))
        ++start;

    if (*start == '-' || *start == '\0')
        return false;

    errno = 0;
    char *end;
    __uintmax_t value = strtoumax(start, &end, 10);

    while (isspace((unsigned char)*end))
        ++end;

    if (errno == ERANGE || end == start || *end != '\0' ||
        value > __UINT64_MAX__)
        return false;

    *out = (__uint64_t)value;
    return true;
}
