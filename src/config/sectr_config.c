#include "sectr_config.h"

config_f config_file_open() {
    config_f config_file = fopen(/*config_path*/, "r");
    if (!config_file) {
        ERR(ERRC_IOError, "Failed to open %s", config_path);
    }
    
    return config_file;
}