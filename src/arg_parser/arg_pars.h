#pragma once

#include "../defs.h"
#include "../Debug/sectr_err.h"

typedef void  (*void_fp)(int argc, char **argv);

typedef struct args_t {
    char *long_arg;
    char *short_arg;

    char *describtion;

    bool require_one_more_arg;

    void_fp func;

} args_t;

void parse_args(int argc, char **argv, const args_t *arg_arr);