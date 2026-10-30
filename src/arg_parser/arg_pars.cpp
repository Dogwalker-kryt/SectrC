#include "arg_pars.hpp"

static const args_t *find_arg(const char *token, __uint64_t name_length, const args_t *arg_arr, __uint64_t arg_count) {
    for (__uint64_t i = 0; i < arg_count; ++i) {
        const char *names[] = {arg_arr[i].long_arg, arg_arr[i].short_arg};

        for (__uint64_t name_index = 0; name_index < SIZEOF_ARRAY(names); ++name_index) {

            const char *name = names[name_index];
            if (name && strlen(name) == name_length && strncmp(token, name, name_length) == 0) {
                return &arg_arr[i];
            }

        }
    }
    return NULL;
}

static bool looks_like_option(const char *token) {
    return token && token[0] == '-' && token[1] != '\0';
}

static bool set_parse_error(const char *message, const char *token) {
    ERR(ERRC_InvalidInput, NULL, message, token);
    return false;
}

static bool is_known_option(const char *token, const args_t *const arg_arr, __uint64_t arg_count) {
    if (!token) {
        return false;
    }

    const char *equals = strchr(token, '=');
    __uint64_t name_length = equals ? (__uint64_t)(equals - token) : strlen(token);

    return find_arg(token, name_length, arg_arr, arg_count) != NULL;
}

bool parse_args(int argc, const char *const argv[], const args_t *arg_arr, __uint64_t arg_count) {

    for (int i = 1; i < argc; ++i) {

        const char *token = argv[i];
        if (!token) {
            return set_parse_error("Null option token: '%s'", "<null>");
        }

        const char *equals = strchr(token, '='); 
        __uint64_t name_length = equals ? (__uint64_t)(equals - token) : strlen(token);
        const args_t *arg = find_arg(token, name_length, arg_arr, arg_count);
        if (unlikely(!arg)) {
            return set_parse_error("Unknown option: '%s'", token);
        }

        const char *value = equals ? equals + 1 : NULL;
        if (equals && arg->value_mode == ARG_REQ_NO_VALUE) {
            return set_parse_error("Option does not take a value: '%s'", token);
        }
        
        if (equals && value[0] == '\0') {
            return set_parse_error("Option value cannot be empty: '%s'", token);
        }

        if (arg->value_mode == ARG_REQ_REQUIRED_VALUE && !value) {
            if (i + 1 >= argc || is_known_option(argv[i + 1], arg_arr, arg_count)) {
                return set_parse_error("Missing value for option: '%s'", token);
            }

            value = argv[++i];

        } else if (arg->value_mode == ARG_REQ_OPTIONAL_VALUE && !value && i + 1 < argc && !looks_like_option(argv[i + 1])) {
            value = argv[++i];
        }

        if (arg->operation && !arg->operation(value, arg->context)) {
            return false;
        }
        if (arg->exit_after) {
            return true;
        }
    }

    return true;
}
