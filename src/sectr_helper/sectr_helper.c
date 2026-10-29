#include "sectr_helper.h"
#include "../Debug/sectr_err.h"

void ul_to_str(char *buff, __uint64_t d) {
    snprintf(buff, sizeof(buff), "%lu", d);
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

void view_logs(__uint64_t lines_to_count) {
    if (lines_to_count == 0) lines_to_count == __UINT64_MAX__;

    fseek(globals.log_path.fd, 0, SEEK_SET);

    #define line_size 256UL
    char line[line_size];

    while (fgets(line, line_size, globals.log_path.fd) || lines_to_count >= 0) {

        char *open_bracket = strchr(line, '[');
        if (open_bracket == NULL) {
            char buff[32];
            ul_to_str(buff, lines_to_count);
            ERR(ERRC_CorruptedData, "Log message format at line:%s is invalid", buff);
        }
    }
}