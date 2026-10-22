#include "arg_pars.h"

bool parse_args(int argc, char **argv, const args_t *arg_arr, size_t arg_count) {
    if (unlikely(argc < 2)) return false;
    
    for (int i = 1; i < argc; ++i) {
        bool matched = false;

        for (size_t arr_idx = 0; arr_idx < arg_count; ++arr_idx) {
            const char *argv_i = argv[i];
            const args_t *arg = &arg_arr[arr_idx];

            if ((arg->long_arg && strcmp(argv_i, arg->long_arg) == 0) ||(arg->short_arg && strcmp(argv_i, arg->short_arg) == 0)) {
                if (arg->func) {
                    arg->func(argc, argv);
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
