#include "arg_pars.hpp"

bool parse_args(int argc, char **argv, const args_t *arg_arr, __uint64_t arg_count) {
    if (unlikely(argc < 2)) return false;
    
    const char *argv_i = NULL;
    const args_t *arg = NULL;
    for (int i = 1; i < argc; ++i) {
        bool matched = false;

        for (__uint64_t arr_idx = 0; arr_idx < arg_count; ++arr_idx) {
            argv_i = argv[i];
            arg = &arg_arr[arr_idx];

            if (likely((arg->long_arg && strcmp(argv_i, arg->long_arg) == 0) || (arg->short_arg && strcmp(argv_i, arg->short_arg) == 0))) {
                if (arg->operation) {
                    arg->operation();
                }

                if (arg->exit_after_ == EXIT_AFTER) {
                    exit(EXIT_SUCCESS);
                } 

                matched = true;
                break;
            }
        }

        if (!matched) {
            ERR(ERRC_OutOfRange, NULL, "No matching arg flag for '%s' found", argv[i]);
            return false;
        }
    }

    return true;
}
