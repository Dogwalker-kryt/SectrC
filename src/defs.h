#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint-gcc.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>
#include "color.h"

typedef __UINTPTR_TYPE__ __uintptr_t;

#define DEV_SUFFIX "_dev"
#define NEW_TERMINAL_SCREEN "\033[?1049h"
#define LEAVE_TERMINAL_SCREEN "\033[?1049l"

typedef struct drive_metadata_t {
    char *name;
    char *model;
    char *type;
    char *mountpoint;
    char *vendor;
    char *fstype;
    
    __uint64_t size;
    __uint16_t serial;
    __uint16_t uuid;

} drive_metadata_t;