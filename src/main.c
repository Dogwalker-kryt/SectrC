#include "sectr_helper/sectr_helper.h"
#include "Debug/sectr_err.h"
#include "arg_parser/arg_pars.h"
#include "read_sys_fs/read_sys_fs.h"
#include "disk_data/disk_data.h"
#include "list_drives/list_drives.h"
#include "spinner/spinner.h"
#include "fingerprint/fingerprint.h"
#include "overwriter/overwriter.h"
#include "clone/clone.h"
#include "config/sectr_config.h"
#include "ui/ui.h"
#include <errno.h>
#include "io_stats/io_stats.h"
#include "disk_health/health.h"
#include "mount/mount.h"

#ifndef SECTRC_VERSION
#define SECTRC_VERSION "development"
#endif

typedef struct help_context_t {
    const args_t *options;
    size_t option_count;
    const char *program_name;
} help_context_t;

static help_context_t help_context;

static void print_example(const char *args, const char *description) {
    printf("    %s%s%s %s", CYAN, ui_program_name, RST, args);
    printf("%*s%s%s%s\n", (int32_t)(22 - strlen(args)), "", GRAY, description, RST);
}

static bool handle_help(const char *value, void *context) {
    (void)value;
    const help_context_t *help = (const help_context_t *)context;

    ui_banner();
    printf("\n  %sLinux disk inspection tool%s  %s·  %s%s\n", BOLD, RST, GRAY, SECTRC_VERSION, RST);

    ui_section("USAGE");
    printf("    %s%s%s %s[options]%s\n", CYAN, help->program_name, RST, GRAY, RST);

    ui_section("OPTIONS");
    for (size_t i = 0; i < help->option_count; ++i) {
        const args_t *arg = &help->options[i];

        char short_name[16], value_name[32] = "";
        snprintf(short_name, sizeof(short_name), "%s,", arg->short_arg);
        if (arg->value_mode == ARG_REQ_REQUIRED_VALUE) snprintf(value_name, sizeof(value_name), "<%s>", arg->value_name ? arg->value_name : "value");
        else if (arg->value_mode == ARG_REQ_OPTIONAL_VALUE) snprintf(value_name, sizeof(value_name), "[%s]", arg->value_name ? arg->value_name : "value");

        printf("    %s%s%-4s%s %s%s%-14s%s %s%-12s%s %s\n", BOLD, CYAN, short_name, RST, BOLD, CYAN, arg->long_arg, RST, GRAY, value_name, RST, arg->description ? arg->description : "");
    }

    ui_section("EXAMPLES");
    print_example("--list", "all disks and partitions");
    print_example("-l nvme0n1", "details for one disk and its partitions");
    print_example("-fp /dev/sda", "fingerprint a drive");
    print_example("-ow sdb", "erase a disk (asks first, needs sudo)");
    print_example("-cp sda:sdb", "clone sda onto sdb (asks first, needs sudo)");
    print_example("-i nvme0n1", "I/O stats of a disk since boot");
    print_example("-hl nvme0n1", "SMART health of a disk (needs sudo)");
    print_example("-vl 20", "last 20 log lines");

    
    printf("\n  %sColor: %s\n  Stand-alone: %s", GRAY, globals.no_color ? "false" : "true", globals.stand_alone ? "true" : "false");

    char log_path[PATH_MAX];
    char config_path[PATH_MAX];
    ui_tilde_path(globals.log_path.path ? globals.log_path.path : "-", log_path, sizeof(log_path));
    ui_tilde_path(globals.config_path.path ? globals.config_path.path : "-", config_path, sizeof(config_path));
    printf("\n  Logs: %s\n  Config: %s%s\n\n", log_path, config_path, RST);
    return true;
}

static bool print_version(const char *value, void *context) {
    (void)value;
    (void)context;
    printf("%sSectrC%s%s %s%s\n", BOLD, RST, GRAY, SECTRC_VERSION, RST);
    return true;
}

static bool abstract_view_logs(const char *value, void *context) {
    (void)context;
    size_t line_count = 0;

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
        
        if (unlikely(errno == ERANGE || end == value || *end != '\0')) {
            ERR(ERRC_InvalidInput, NULL, "Expected a non-negative line count, got '%s'", value);
            return false;
        }
        
        line_count = (size_t)parsed;
    }

    return view_logs(line_count);
}

static bool abstract_clear_logs(const char *value, void *context) {
    (void)value;
    (void)context;
    return clear_logs();
}

static bool list_drives(const char *value, void *context) {
    (void)context;

    Vec vec;
    if (unlikely(!vec_init(&vec, 1, sizeof(drive_mt)))) return false;

    if (unlikely(!get_physical_disk_info(&vec))) {
        vec_free(&vec);
        return false;
    }

    bool found = print_disk_infos((const drive_mt*)vec.data, vec.element_count, value);

    vec_free(&vec);
    if (!found && value) {
        ERR(ERRC_DeviceNotFound, NULL, "No disk or partition matched '%s'", value);
        return false;
    }

    return true;
}

static bool fingerprint(const char *value, void *context) {
    (void)context;

    char hash[FINGERPRINT_SHA256_HEX_SIZE + 1];
    if (!fingerprint_disk(value, hash)) return false;

    printf("\n  %s%s◆%s %sFingerprint%s  %s%s%s\n", BOLD, PURPLE, RST, BOLD, RST, GRAY, value, RST);
    printf("    %sSHA-256%s  %s%s%s%s\n\n", GRAY, RST, BOLD, GREEN, hash, RST);
    return true;
}

static bool overwrite(const char *value, void *context) {
    (void)context;
    return disk_overwrite(value);
}

static bool clone_drive(const char *value, void *context) {
    (void)context;

    const char *colon = strchr(value, ':');
    if (!colon || colon == value || colon[1] == '\0' || strchr(colon + 1, ':')) {
        ERR(ERRC_InvalidInput, NULL, "Expected <source>:<target> like sda:sdb, got '%s'", value);
        return false;
    }

    char src[PATH_MAX];
    snprintf(src, sizeof(src), "%.*s", (int32_t)(colon - value), value);
    return disk_clone(colon + 1, src);
}

static bool io_info(const char *value, void *context) {
    (void)context;
    return io_statistics(value);
}

static bool health_info(const char *value, void *context) {
    (void)context;
    return disk_health(value);
}

static bool mount(const char *value, void *context) {
    (void)context;

    const char *colon = strchr(value, ':');
    if (!colon || colon == value || colon[1] == '\0' || strchr(colon + 1, ':')) {
        ERR(ERRC_InvalidInput, NULL, "Expected <source>:<target> like sda:/mnt/sda, got '%s'", value);
        return false;
    }

    char src[PATH_MAX];
    snprintf(src, sizeof(src), "%.*s", (int32_t)(colon - value), value);
    return disk_mount(src, colon + 1);
}

static bool unmount(const char *value, void *context) {
    (void)context;
    return disk_unmount(value);
}

static const args_t arg_arr[] = {
    {"--help", "-h", "Show this help message", NO_VALUE_NAME, ARG_REQ_NO_VALUE, handle_help, &help_context, EXIT_AFTER},
    {"--version", "-v", "Show the current version", NO_VALUE_NAME, ARG_REQ_NO_VALUE, print_version, NO_CONTEXT, EXIT_AFTER},
    {"--no-color", "-nc", "Disable colored output", NO_VALUE_NAME, ARG_REQ_NO_VALUE, set_no_color_true, NO_CONTEXT, DONT_EXIT_AFTER},
    {"--no-log", "-nl", "Disables logging", NO_VALUE_NAME, ARG_REQ_NO_VALUE, set_nolog_true, NO_CONTEXT, DONT_EXIT_AFTER},
    {"--stand-alone", "-sa", "Disables config loading and logging", NO_VALUE_NAME, ARG_REQ_NO_VALUE, set_standalone_true, NO_CONTEXT, DONT_EXIT_AFTER},
    {"--no-config", "-ncfg", "Disables config loading", NO_VALUE_NAME, ARG_REQ_NO_VALUE, set_no_config_true, NO_CONTEXT, DONT_EXIT_AFTER},
    {"--bypass-security", "-y", "bypass 'are you sure' qestions", NO_VALUE_NAME, ARG_REQ_NO_VALUE, set_bypass_security_code_true, NO_CONTEXT, DONT_EXIT_AFTER},
    {"--view-logs", "-vl", "View log messages (default: all)", "lines", ARG_REQ_OPTIONAL_VALUE, abstract_view_logs, NO_CONTEXT, EXIT_AFTER},
    {"--clear-logs", "-cl", "Clear the log file", NO_VALUE_NAME, ARG_REQ_NO_VALUE, abstract_clear_logs, NO_CONTEXT, EXIT_AFTER},
    {"--list", "-l", "List disks, optionally filtered by disk or partition name", "disk-name", ARG_REQ_OPTIONAL_VALUE, list_drives, NO_CONTEXT, EXIT_AFTER},
    {"--fingerprint", "-fp", "Show custom sha256 fingerprint of a drive", "disk-name", ARG_REQ_REQUIRED_VALUE, fingerprint, NO_CONTEXT, EXIT_AFTER},
    {"--overwrite", "-ow", "Erase all data on a disk or partition (asks first)", "disk-name", ARG_REQ_REQUIRED_VALUE, overwrite, NO_CONTEXT, EXIT_AFTER},
    {"--clone", "-cp", "Copy a disk or partition onto another one (asks first)", "src:dest", ARG_REQ_REQUIRED_VALUE, clone_drive, NO_CONTEXT, EXIT_AFTER},
    {"--info", "-i", "Show I/O stats of a disk or partition since boot", "disk-name", ARG_REQ_REQUIRED_VALUE, io_info, NO_CONTEXT, EXIT_AFTER},
    {"--health", "-hl", "Show SMART health of a SATA or NVMe disk", "disk-name", ARG_REQ_REQUIRED_VALUE, health_info, NO_CONTEXT, EXIT_AFTER},
    {"--mount", "-mnt", "Mount a disk", "disk:dest", ARG_REQ_REQUIRED_VALUE, mount, NO_CONTEXT, EXIT_AFTER},
    {"--umount", "-umnt", "Unmount a path", "path", ARG_REQ_REQUIRED_VALUE, unmount, NO_CONTEXT, EXIT_AFTER},
};

int main(int argc, char **argv) {
    atexit(free_globals);
    set_globals();

    if (argc > 1 && (strcmp(argv[1], arg_arr[4].long_arg) == 0 || strcmp(argv[1], arg_arr[4].short_arg) == 0)) {
        set_standalone_true(NULL, NULL);
    } else if (argc > 1 && (strcmp(argv[1], arg_arr[5].long_arg) == 0 || strcmp(argv[1], arg_arr[5].short_arg) == 0)) {
        set_no_config_true(NULL, NULL);
    } else {
        config_load();
    }

    help_context.options = arg_arr;
    help_context.option_count = SIZEOF_ARRAY(arg_arr);

    if (argc > 0 && argv[0][0]) {
        const char *slash = strrchr(argv[0], '/');
        ui_program_name = slash ? slash + 1 : argv[0];
    }

    help_context.program_name = ui_program_name;

    if (!isatty(STDOUT_FILENO) || getenv("NO_COLOR")) globals.no_color = true;

    if (!globals.no_log && !globals.stand_alone) {
        if (unlikely(!open_log_file())) goto _EXIT_FAILIURE;
        atexit(close_log_file);
    }

    if (unlikely(argc < 2)) {
        handle_help(NULL, &help_context);
        goto _EXIT_SUCCESS;
    }

    const uint8_t parse = parse_args(argc, argv, arg_arr, SIZEOF_ARRAY(arg_arr));
    if (unlikely((parse == UINT8_MAX))) {
        goto _EXIT_FAILIURE;

    } else if (parse == 0) {
        goto _EXIT_SUCCESS;
    }

    handle_help(NULL, &help_context);
    goto _EXIT_SUCCESS;

    _EXIT_FAILIURE:
        PERR();
        return EXIT_FAILURE;

    _EXIT_SUCCESS:
    return EXIT_SUCCESS;
}
