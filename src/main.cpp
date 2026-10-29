#include "sectr_helper/sectr_helper.h"
#include "Debug/sectr_err.h"
#include "arg_parser/arg_pars.hpp"

void free_paths(void) {
    free_path(&globals.log_path);
    free_path(&globals.config_path);
}


void help(int argc, char **argv, const args_t *arg_arr) {
    (void)argc;

    printf("%sUsage: %s%s -<operation> <device> [options]\n", BOLD, argv[0], RST);
    puts("  Only one operation may be specified.");
    puts("  -c, --confirm                    confirm the operation");
    puts("  -nl, --no-log                    disable logging");
    puts("  -nc, --no-color                  disable colored output");
    puts("  -sa, --stand-alone               run in standalone mode");
    puts("  -bsc, --bypass-security-code     bypass the security-code check");

    for (__uint64_t i = 0; i < SIZEOF_ARRAY(arg_arr); ++i) {
        printf("  %s, %s      %s\n", arg_arr[i].long_arg, arg_arr[i].short_arg, arg_arr[i].describtion);
    }
}


int main(int argc, char **argv, char **envp) {
    // atexit(free_paths);
    atexit(close_log_file);

    set_globals();

    if (!open_log_file()) {
        PERR();
        exit(EXIT_FAILURE);
    }

    const args_t arg_arr[] = {
        {"--help", "-h", "shows all avilable/valid flags", NOT_REQUIRE_ARG, [&argc, &argv, &arg_arr](){help(argc, argv, arg_arr);}, EXIT_AFTER},
        {"--version", "-v", "show current version of the sectr", NOT_REQUIRE_ARG, [](){printf("%sSectrC version%s: %s%s%s\n", BOLD, RST, BOLD, "no", RST);}, EXIT_AFTER},
        {"--no-color", "-nc", "disables colors ", NOT_REQUIRE_ARG, [](){globals.no_color = true;}, NOT_EXIT_AFTER},
        {"--view-logs", "-vl", "view log messages", NOT_REQUIRE_ARG, [](){ view_logs(0); }, EXIT_AFTER},
        {"--clear-logs", "-cl", "clear log file", NOT_REQUIRE_ARG, [](){ clear_logs(); }, EXIT_AFTER},
    };

    if (argc < 2) {
        help(argc, argv, arg_arr);
        exit(EXIT_SUCCESS);
    }

    if (!parse_args(argc, argv, arg_arr, SIZEOF_ARRAY(arg_arr))) {
        PERR();
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
