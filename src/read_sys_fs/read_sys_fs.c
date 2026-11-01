#include "read_sys_fs.h"
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

bool read_sysfs_text(const char *path, char *buffer, __uint_least64_t capacity) {
    if (unlikely(path == NULL) || unlikely(buffer == NULL) || unlikely(capacity == 0)) return false;

    buffer[0] = '\0';

    FILE *fp = fopen(path, "r");
    if (fp == NULL) return false;

    __uint_least64_t length = 0;
    __int32_t ch;
    bool ok = true;

    while ((ch = fgetc(fp)) != EOF) {

        if (ch == '\0' || length >= capacity - 1) {
            ok = false;
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


bool read_sysfs_u64(const char *path, __uint64_t *out) {
    if (unlikely(out == NULL)) return false;

    char buffer[128];
    if (!read_sysfs_text(path, buffer, sizeof(buffer))) return false;

    char *start = buffer;
    while (isspace((unsigned char)*start)) ++start;

    if (*start == '-' || *start == '\0') return false;

    errno = 0;
    char *end;
    __uintmax_t value = strtoumax(start, &end, 10);

    while (isspace((unsigned char)*end)) ++end;

    if (errno == ERANGE || end == start || *end != '\0' || value > __UINT64_MAX__) return false;

    *out = (__uint64_t)value;
    return true;
}


__uint64_t read_sysfs_uint64(const char *__restrict__ path) {
    FILE *sys_file = fopen(path, "r");
    if (!sys_file) {
        return 0;
    }

    __uint64_t value = 0;

    if (fscanf(sys_file, "%" SCNu64, &value) != 1) {
        fclose(sys_file);
        return 0;
    }

    fclose(sys_file);
    return value;
}