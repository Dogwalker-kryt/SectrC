#pragma once

#include "../defs.h"

static inline void sectr_warn(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static inline void sectr_warn(const char *fmt, ...) {
    fprintf(stderr, "\n  %s%sWarning:%s ", YELLOW, BOLD, RST);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "%s\n\n", RST);
}

#define WARN(...) sectr_warn(__VA_ARGS__)
