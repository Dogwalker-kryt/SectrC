#include "sectr_helper.h"

__always_inline void cpy_root_to_buff(char *buff, const char *dmgr_root) {
    strcpy(buff, dmgr_root);
}

void set_globals(globals *g) {
    g->bypass_security_code = false;
    g->no_color = false;
    g->no_log = false;
    g->stand_alone = false;

    char *buff = (char*)malloc(PATH_MAX);
    char *sectr_root = elf_run_path();

    cpy_root_to_buff(&buff, sectr_root);
    strcpy(buff, LOG_PATH_SLICE);
    g->log_path = path_init(buff, "log_file");

    memset(buff, '\0', sizeof(buff));

    cpy_root_to_buff(&buff, sectr_root);
    strcpy(buff, CONFIG_PATH_SLICE);
    g->config_path = path_init(buff, "config_file");

    free(buff);
}