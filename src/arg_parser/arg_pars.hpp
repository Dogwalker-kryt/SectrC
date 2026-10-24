#pragma once

#include <functional>
#include "../defs.h"
#include "../Debug/sectr_err.h"

typedef struct args_t {
    const char *long_arg;
    const char *short_arg;

    const char *describtion;

    bool require_one_more_arg = false;

    std::function<void()> operation;
    bool exit_after_ = false;

} args_t;


bool parse_args(int argc, char **argv, const args_t *arg_arr, __uint64_t arg_count);
