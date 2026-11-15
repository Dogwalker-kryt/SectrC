#pragma once

#include "../defs.h"
#include "../Debug/sectr_err.h"

#define CONFIG_LINE_MAX 64U

#define config_f FILE*

static const char *valid_config_keys[] = {
    "no_color", "no_log", ""
};

typedef struct config_t {
    config_f config_file;
    
    char **keys;
    char **values;
    uint8_t elements;

} config_t;

static config_t config;

config_f config_file_open();