#include "arg_pars.h"

void parse_args(int argc, char **argv, const args_t *arg_arr) {
    for (size_t i = 0; i < argc; ++i) {

        for (size_t arr_idx = 0; arr_idx < sizeof(arg_arr); ++arr_idx) {

            char *argv_i = argv[i];
            if (strcmp(argv_i, arg_arr[arr_idx].long_arg) == 0 || strcmp(argv_i, arg_arr[arr_idx].short_arg)) {

                arg_arr[arr_idx].func(argc, argv);
                break;

            }

            const char *msg = "No matching arg flag for '";
            char buff[128];
            strncpy(buff, msg, 128);
            strncpy(buff, argv_i, strlen(msg));

            ERR(ERRC_OutOfRange, "No matching arg flag for '");
        }
    }
}
