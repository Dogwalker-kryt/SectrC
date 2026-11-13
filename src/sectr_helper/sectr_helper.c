#include "sectr_helper.h"
#include "../Debug/sectr_err.h"
#include <inttypes.h>

void ul_to_str(char *buff, size_t buff_size, uint64_t d) {
    snprintf(buff, buff_size, "%" PRIu64, d);
}

void clear_logs(void) {
    if (!globals.log_path.fd || !globals.log_path.path) {
        ERR(ERRC_IOError, "Cannot clear logs", "log file is not open");
        return;
    }

    if (fflush(globals.log_path.fd) != 0 || !freopen(globals.log_path.path, "w+", globals.log_path.fd)) {
        ERR(ERRC_IOError, "Failed to clear log file", "%s", globals.log_path.path);
        return;
    }

    puts("Log file cleared");
}

void view_logs(size_t lines_to_count) {
    if (!globals.log_path.fd) {
        ERR(ERRC_IOError, "Cannot view logs", "log file is not open");
        return;
    }
    if (fflush(globals.log_path.fd) != 0 || fseek(globals.log_path.fd, 0, SEEK_SET) != 0) {
        ERR(ERRC_IOError, "Cannot read logs", "failed to seek log file");
        return;
    }

    char *line = NULL;
    size_t capacity = 0;
    uint64_t total_lines = 0;
    
    while (getline(&line, &capacity, globals.log_path.fd) != -1) {
        ++total_lines;
    }

    if (ferror(globals.log_path.fd)) {
        free(line);
        ERR(ERRC_IOError, "Cannot read logs", "failed while counting log lines");
        return;
    }

    uint64_t lines_to_skip = (lines_to_count > 0 && lines_to_count < total_lines) ? total_lines - lines_to_count : 0;
    if (fseek(globals.log_path.fd, 0, SEEK_SET) != 0) {
        free(line);
        ERR(ERRC_IOError, "Cannot read logs", "failed to rewind log file");
        return;
    }

    uint64_t line_number = 0;
    while (getline(&line, &capacity, globals.log_path.fd) != -1) {
        if (line_number++ >= lines_to_skip) {
            fputs(line, stdout);
        }
    }
    free(line);

    if (ferror(globals.log_path.fd)) {
        ERR(ERRC_IOError, "Cannot read logs", "failed while displaying log lines");
    }
}

char *convert_to_GiB(size_t bytes) {
    char buffer[32];
    double gibibytes = (double)bytes / (1024.0 * 1024.0 * 1024.0);
    snprintf(buffer, sizeof(buffer), "%.2f GiB", gibibytes);
    return strdup(buffer);
}

