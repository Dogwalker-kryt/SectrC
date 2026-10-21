#include "sectr_helper/sectr_helper.h"
#include "arg_parser/arg_pars.h"

void_fp help(int argc, char **argv, void *data);

const args_t arg_arr[] = {
    {"--help", "-h", "shows all avilable/valid flags", help},
    {"--version", "-v", "show current version of the sectr"}
};

void_fp help(int argc, char **argv, void *data) {
    (void)argc; (void)data;

    printf( "Usage: %s --flag data\n", argv[0]);

    for (size_t i = 0; i < sizeof(arg_arr); ++i) {
        printf("    %s, %s      %s\n", arg_arr[i].long_arg, arg_arr[i].short_arg, arg_arr[i].describtion);
    }
}


int main(int argc, char **argv, char **envp) {
    globals _g;
    set_globals(&_g);

    ERR(ERRC_IOError, "test");

    PERR();

    return EXIT_SUCCESS;
}
