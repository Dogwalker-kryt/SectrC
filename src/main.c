#include "sectr_helper/sectr_helper.h"
#include "arg_parser/arg_pars.h"

void help(int argc, char **argv);
void version(int argc, char **argv);

const args_t arg_arr[] = {
    {"--help", "-h", "shows all avilable/valid flags", false, help},
    {"--version", "-v", "show current version of the sectr", false, version}
};

void help(int argc, char **argv) {
    (void)argc;

    printf( "%sUsage: %s%s --flag data\n", BOLD, argv[0], RST);

    for (size_t i = 0; i < sizeof(arg_arr) / sizeof(arg_arr[0]); ++i) {
        printf("    %s%s, %s%s      %s\n", BOLD, arg_arr[i].long_arg, arg_arr[i].short_arg, RST, arg_arr[i].describtion);
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

    bool parse_ret = parse_args(argc, argv, arg_arr, sizeof(arg_arr) / sizeof(arg_arr[0]));
    PERR();

    if (!parse_ret) {
        putc('\n', stderr);
        help(argc, argv);
    }

    return EXIT_SUCCESS;
}
