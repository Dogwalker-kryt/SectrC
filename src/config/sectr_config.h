#pragma once

#include "../defs.h"
#include "../Debug/sectr_err.h"

#define CONFIG_LINE_MAX 64U

typedef struct config_option_t {
    const char *key;
    size_t global_offset;
} config_option_t;

extern const config_option_t config_options[];
extern const size_t config_option_count;

typedef struct config_t {
    FILE *config_file;
    
    char **keys;
    char **values;
    uint8_t elements;

} config_t;

extern config_t config;

FILE *config_file_open(char *config_path);

void config_file_close();

bool config_parse_line(char *line, char **key, char **value);

bool config_apply(const char *key, const char *value);

bool config_load(void);