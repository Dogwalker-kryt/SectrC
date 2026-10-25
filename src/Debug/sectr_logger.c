#include "sectr_logger.h"

bool open_log_file(void) {
    FILE *log_file = fopen(globals.log_path.path, "a");

    if (log_file == NULL) {
        ERR(ERRC_IOError, "Faile to open file: %s%s%s", BOLD, globals.log_path.path, RST);
        return false;
    }

    globals.log_path.fd = log_file;
    return true;
}

void close_log_file(void) {
    fclose(globals.log_path.fd);
}

static const char *log_messgae(enum LOG_TYPE log_type) {
    switch (log_type) {
        case logt_err:
            return "[ERROR] ";
        case logt_warn:
            return "[WARNING] ";
        case logt_info:
            return "[INFO] ";
        default:
            return "[VOID] "; 
    }
}

static char *get_time(void) {
    time_t time_now = time(NULL);
    struct tm local;

    if (time_now == (time_t)-1 || localtime_r(&time_now, &local) == NULL) {
        ERR(ERRC_TimeFailure, "Failed to get current time", "time() or localtime_r() failed");
    }
    static char time_str[26];
    strftime(time_str, sizeof(time_str), "%d-%m-%Y %H:%M:%S", &local);
    return time_str;
}

void log(enum LOG_TYPE log_type, const char *msg, const char *func) {
    char *time_str = get_time();
    const char *log_type_str = log_messgae(log_type);

    fprintf(globals.log_path.fd, "%s%s%s%s%s%s\n", time_str, log_type_str, msg, " (location: ", func, ")");
}