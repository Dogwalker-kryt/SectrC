#include "sectr_config.h"
#include "../globals.h"
#include "../Debug/sectr_warn.h"
#include <ctype.h>
#include <stddef.h>

config_t config = {
    NULL,
    NULL,
    NULL,
    0
};

const config_option_t config_options[] = {
    {"no_color", offsetof(globals_t, no_color)},
    {"no_log", offsetof(globals_t, no_log)},
};

const size_t config_option_count = sizeof(config_options) / sizeof(config_options[0]);

static const struct {
    const char *text;
    bool value;
} config_bool_values[] = {
    {"true", true},
    {"1", true},
    {"false", false},
    {"0", false},
};

static char *trim_config_text(char *text) {
    while (isspace((unsigned char)*text)) ++text;

    char *end = text + strlen(text);

    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';

    return text;
}

bool config_parse_line(char *line, char **key, char **value) {
    if (unlikely(unlikely(!line) || unlikely(!key) || unlikely(!value))) return false;

    char *entry = trim_config_text(line);
    if (*entry == '\0' || *entry == '#') return false;

    char *equals = strchr(entry, '=');
    if (!equals) return false;

    *equals = '\0';
    char *parsed_key = trim_config_text(entry);
    char *parsed_value = trim_config_text(equals + 1);
    if (*parsed_key == '\0' || *parsed_value == '\0') return false;

    bool valid_key = false;
    for (size_t i = 0; i < config_option_count; ++i) if (strcmp(parsed_key, config_options[i].key) == 0) valid_key = true;
    if (!valid_key) return false;

    *key = parsed_key;
    *value = parsed_value;
    return true;
}

bool config_apply(const char *key, const char *value) {
    bool *setting = NULL;
    bool parsed_value = false;

    for (size_t i = 0; i < config_option_count; ++i) {
        if (strcmp(key, config_options[i].key) == 0) {
            setting = (bool*)((unsigned char*)&globals + config_options[i].global_offset);
            break;
        }
    }
    if (!setting) return false;

    bool valid_value = false;
    for (size_t i = 0; i < sizeof(config_bool_values) / sizeof(config_bool_values[0]); ++i) {
        if (strcmp(value, config_bool_values[i].text) == 0) {
            parsed_value = config_bool_values[i].value;
            valid_value = true;
            break;
        }
    }
    if (!valid_value) return false;

    *setting = parsed_value;
    return true;
}

FILE *config_file_open(char *config_path) {
    FILE *config_file = fopen(config_path, "r");
    if (!config_file) {
        WARN("Couldnt open config file");
        return NULL;
    }
    
    return config_file;
}

void config_file_close() {
    fclose(config.config_file);
    globals_set_ptr_fd(&globals.config_path.fd, NULL);
}

bool config_load(void) {
    config.config_file = config_file_open(globals.config_path.path);
    if (!config.config_file) return false;
    globals_set_ptr_fd(&globals.config_path.fd, config.config_file);

    char *line = NULL;
    size_t capacity = 0;
    bool ok = 0;

    while (getline(&line, &capacity, config.config_file) != -1) {
        char *key, *value;
        if (!config_parse_line(line, &key, &value)) continue;

        if (!config_apply(key, value)) {
            ok = false;
            break;
        }

        ok = true;
    }

    free(line);
    return ok;
}