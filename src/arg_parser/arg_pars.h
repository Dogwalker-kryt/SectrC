#pragma once

#include "../defs.h"
#include "../Debug/sectr_err.h"

typedef enum {
    ARG_REQ_NO_VALUE = 0,
    ARG_REQ_REQUIRED_VALUE = 1,
    ARG_REQ_OPTIONAL_VALUE = 2,
} arg_reqs_t;

#define EXIT_AFTER true
#define DONT_EXIT_AFTER false

typedef struct args_t {
    const char *long_arg;
    const char *short_arg;

    const char *description;

    const char *value_name;
    arg_reqs_t value_mode;

    arg_fn operation;

    void *context;
    bool exit_after;
} args_t;

bool parse_args(int argc, char *const argv[], const args_t *arg_arr, __uint64_t arg_count);
