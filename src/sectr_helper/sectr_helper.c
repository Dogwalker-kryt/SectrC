#include "sectr_helper.h"
#include "../Debug/sectr_err.h"
#include "../ui/ui.h"
#include <inttypes.h>

void ul_to_str(char *buff, size_t buff_size, uint64_t d) {
    snprintf(buff, buff_size, "%" PRIu64, d);
}

bool clear_logs(void) {
    if (!globals.log_path.fd || !globals.log_path.path) {
        ERR(ERRC_IOError, "Cannot clear logs", "log file is not open");
        return false;
    }

    if (fflush(globals.log_path.fd) != 0 || !freopen(globals.log_path.path, "w+", globals.log_path.fd)) {
        ERR(ERRC_IOError, "Failed to clear log file", "%s", globals.log_path.path);
        return false;
    }

    char path[PATH_MAX];
    ui_tilde_path(globals.log_path.path, path, sizeof(path));
    ui_ok("Log cleared  %s%s%s", GRAY, path, RST);
    return true;
}

static void print_log_line(const char *line) {
    const char *tag = strchr(line, '[');
    const char *close = tag ? strchr(tag, ']') : NULL;
    if (!tag || !close) {
        printf("  %s", line);
        return;
    }

    const char *color = strncmp(tag, "[ERROR]", 7) == 0 ? RED : strncmp(tag, "[WARNING]", 9) == 0 ? YELLOW : BLUE;
    printf("  %s%.*s%s%s%s%.*s%s%s", GRAY, (int)(tag - line), line, RST, BOLD, color, (int)(close - tag + 1), tag, RST, close + 1);
}

bool view_logs(size_t lines_to_count) {
    if (!globals.log_path.fd) {
        ERR(ERRC_IOError, "Cannot view logs", "log file is not open");
        return false;
    }
    if (fflush(globals.log_path.fd) != 0 || fseek(globals.log_path.fd, 0, SEEK_SET) != 0) {
        ERR(ERRC_IOError, "Cannot read logs", "failed to seek log file");
        return false;
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
        return false;
    }

    char path[PATH_MAX];
    ui_tilde_path(globals.log_path.path, path, sizeof(path));

    if (total_lines == 0) {
        free(line);
        ui_ok("The log is empty  %s%s%s", GRAY, path, RST);
        return true;
    }

    uint64_t lines_to_skip = (lines_to_count > 0 && lines_to_count < total_lines) ? total_lines - lines_to_count : 0;
    if (fseek(globals.log_path.fd, 0, SEEK_SET) != 0) {
        free(line);
        ERR(ERRC_IOError, "Cannot read logs", "failed to rewind log file");
        return false;
    }

    printf("\n  %s%s▍ Log%s  %s%s  ·  %" PRIu64 " of %" PRIu64 " lines%s\n\n", BOLD, MAGENTA, RST, GRAY, path, total_lines - lines_to_skip, total_lines, RST);

    uint64_t line_number = 0;
    while (getline(&line, &capacity, globals.log_path.fd) != -1) {
        if (line_number++ >= lines_to_skip) print_log_line(line);
    }
    free(line);
    putchar('\n');

    if (ferror(globals.log_path.fd)) {
        ERR(ERRC_IOError, "Cannot read logs", "failed while displaying log lines");
        return false;
    }
    return true;
}

char *convert_to_GiB(size_t bytes) {
    char buffer[32];
    double gibibytes = (double)bytes / (1024.0 * 1024.0 * 1024.0);
    snprintf(buffer, sizeof(buffer), "%.2f GiB", gibibytes);
    return strdup(buffer);
}

