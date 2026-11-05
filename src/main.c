#include "sectr_helper/sectr_helper.h"
#include "Debug/sectr_err.h"
#include "arg_parser/arg_pars.h"
#include "read_sys_fs/read_sys_fs.h"
#include "list_drives/list_drives.h"
#include <errno.h>

#define SECTRC_VERSION "development"

typedef struct help_context_t {
    const args_t *options;
    __uint64_t option_count;
    const char *program_name;
} help_context_t;

static help_context_t help_context;

static bool handle_help(const char *value, void *context) {
    (void)value;
    const help_context_t *help = (const help_context_t *)context;

    printf("%sUsage:%s %s [options]\n\n%sOptions%s:\n", BOLD, RST, help->program_name, BOLD, RST);
    for (__uint64_t i = 0; i < help->option_count; ++i) {

        const args_t *arg = &help->options[i];
        printf("  %-18s %-6s", arg->long_arg, arg->short_arg);

        if (arg->value_mode == ARG_REQ_REQUIRED_VALUE) {
            printf(" <%s>", arg->value_name ? arg->value_name : "value");

        } else if (arg->value_mode == ARG_REQ_OPTIONAL_VALUE) {
            printf(" [%s]", arg->value_name ? arg->value_name : "value");
        }

        printf("\t%s\n", arg->description ? arg->description : "");
    }
    return true;
}

static bool print_version(const char *value, void *context) {
    (void)value;
    (void)context;
    printf("SectrC %s\n", SECTRC_VERSION);
    return true;
}

static bool set_no_color_true(const char *value, void *context) {
    (void)value;
    (void)context;
    globals.no_color = true;
    return true;
}

static bool abstract_view_logs(const char *value, void *context) {
    (void)context;
    __uint64_t line_count = 0;

    if (value) {
        for (const char *digit = value; *digit; ++digit) {
            if (*digit < '0' || *digit > '9') {
                ERR(ERRC_InvalidInput, NULL, "Expected a non-negative line count, got '%s'", value);
                return false;
            }
        }

        char *end = NULL;
        errno = 0;
        unsigned long long parsed = strtoull(value, &end, 10);
        if (errno == ERANGE || end == value || *end != '\0') {
            ERR(ERRC_InvalidInput, NULL, "Expected a non-negative line count, got '%s'", value);
            return false;
        }
        line_count = (__uint64_t)parsed;
    }

    view_logs(line_count);
    return true;
}

static bool abstract_clear_logs(const char *value, void *context) {
    (void)value;
    (void)context;
    clear_logs();
    return true;
}

static bool list_drives(const char *value, void *context) {
    char buffer[256];

    get_mount_point("/dev/sda2", buffer);

    printf("%s\n", buffer);

    return true;
}

static const args_t arg_arr[] = {
    {"--help", "-h", "Show this help message", NULL, ARG_REQ_NO_VALUE, handle_help, &help_context, EXIT_AFTER},
    {"--version", "-v", "Show the current version", NULL, ARG_REQ_NO_VALUE, print_version, NULL, EXIT_AFTER},
    {"--no-color", "-nc", "Disable colored output", NULL, ARG_REQ_NO_VALUE, set_no_color_true, NULL, DONT_EXIT_AFTER},
    {"--view-logs", "-vl", "View log messages (default: all)", "lines", ARG_REQ_OPTIONAL_VALUE, abstract_view_logs, NULL, DONT_EXIT_AFTER},
    {"--clear-logs", "-cl", "Clear the log file", NULL, ARG_REQ_NO_VALUE, abstract_clear_logs, NULL, DONT_EXIT_AFTER},
    {"--list", "-l", "Lists all drives", NULL, ARG_REQ_OPTIONAL_VALUE, list_drives, NULL, EXIT_AFTER},
};


int main(int argc, char **argv) {
    atexit(close_log_file);
    set_globals();

    help_context.options = arg_arr;
    help_context.option_count = SIZEOF_ARRAY(arg_arr);
    help_context.program_name = argc > 0 ? argv[0] : "ox";

    if (!open_log_file()) {
        PERR();
        return EXIT_FAILURE;
    }

    if (argc < 2) {
        handle_help(NULL, &help_context);
        return EXIT_SUCCESS;
    }

    if (!parse_args(argc, argv, arg_arr, SIZEOF_ARRAY(arg_arr))) {
        PERR();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
