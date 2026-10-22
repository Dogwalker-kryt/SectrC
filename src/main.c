#include "sectr_helper/sectr_helper.h"
#include "arg_parser/arg_pars.h"

void help(int argc, char **argv);

const args_t arg_arr[] = {
    {"--help", "-h", "shows all avilable/valid flags", false, help},
    {"--version", "-v", "show current version of the sectr", false, version},
    {"--no-color", "-nc", "disables colors "}
};

void help(int argc, char **argv) {
    (void)argc;

    printf("%sUsage: %s%s -fs.<filesystem> <device> [options]\n", BOLD, argv[0], RST);
    puts("  Only one operation may be specified.");
    puts("  -c, --confirm                    confirm the operation");
    puts("  -nl, --no-log                    disable logging");
    puts("  -nc, --no-color                  disable colored output");
    puts("  -sa, --stand-alone               run in standalone mode");
    puts("  -bsc, --bypass-security-code     bypass the security-code check");

    for (size_t i = 0; i < sizeof(arg_arr) / sizeof(arg_arr[0]); ++i) {
        printf("  %s, %s      %s\n", arg_arr[i].long_arg, arg_arr[i].short_arg, arg_arr[i].describtion);
    }
}

void version(int argc, char **argv) {
    (void)argc;
    (void)argv;
    puts("SectrC");
}


int main(int argc, char **argv, char **envp) {
    globals _g;
    set_globals(&_g);

    if (argc < 2) {
        help(argc, argv);
        return EXIT_SUCCESS;
    }

    if (!parse_args(argc, argv, arg_arr, sizeof(arg_arr) / sizeof(arg_arr[0]))) {
        PERR();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
