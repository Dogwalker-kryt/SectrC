#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/types.h>
#include <termios.h>
#include "color.h"

typedef __UINTPTR_TYPE__ __uintptr_t;
typedef __UINT_FAST64_TYPE__ __uint_fast64;

#define DEV_SUFFIX "_dev"
#define NEW_TERMINAL_SCREEN "\033[?1049h"
#define LEAVE_TERMINAL_SCREEN "\033[?1049l"

#define REQUIRE_ARG true
#define NOT_REQUIRE_ARG false

#define EXIT_AFTER true
#define NOT_EXIT_AFTER false

#define KiB 1024

#define PATH_MAX 4096

#define LOG_PATH_SLICE "/data/log.dat"
#define CONFIG_PATH_SLICE "/data/config.conf"

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define likely(expr)   __builtin_expect(!!(expr), 1)

#define SIZEOF_ARRAY(__arr) sizeof(__arr) / sizeof(__arr[0])

typedef bool (*arg_fn)(const char *value, void *context);

#define CLEAR_BUFFER(_buffer) memset(_buffer, '\0', sizeof(_buffer))