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

static const char *log_message(enum LOG_TYPE log_type) {
    switch (log_type) {
        case logt_err:
            return " [ERROR] ";
        case logt_warn:
            return " [WARNING] ";
        case logt_info:
            return " [INFO] ";
        default:
            return " [VOID] "; 
    }
}

static char *get_time(void) {
    time_t time_now = time(NULL);
    struct tm local;

    if (time_now == (time_t)-1 || localtime_r(&time_now, &local) == NULL) {
        ERR(ERRC_TimeFailure, "Failed to get current time", "time() or localtime_r() failed");
        return NULL;
    }
    static char time_str[26];
    strftime(time_str, sizeof(time_str), "%d-%m-%Y %H:%M:%S", &local);
    return time_str;
}

void log(enum LOG_TYPE log_type, const char *msg, const char *func) {
    char *time_str = get_time();
    const char *log_type_str = log_message(log_type);

    if (time_str == NULL || log_type_str == NULL) {
        return;
    }

    fprintf(globals.log_path.fd, "%s%s%s%s%s%s\n", time_str, log_type_str, msg, " (location: ", func, ")");
}

void clear_logs(void) {
    fseek(globals.log_path.fd, 0, SEEK_END);

    long size = ftell(globals.log_path.fd);
    if (size < 0) goto CLEAR_LOGS_EMPTY;

    if (!freopen(globals.log_path.path, "w", globals.log_path.fd)) {
        ERR(ERRC_IOError, "Failed to clear log file: %s%s%s", BOLD, globals.log_path.path, RST);
        return;
    }
    
    CLEAR_LOGS_EMPTY:
        printf("%sLog file is already empty.%s\n", BOLD, RST);
}

void view_logs(void) {
    fseek(globals.log_path.fd, 0, SEEK_SET);

    const __uint_fast64 buffer_size = 256;
    char *buffer = (char*)malloc(buffer_size);

    while (fgets(buffer, buffer_size, globals.log_path.fd)) {



    }
}

