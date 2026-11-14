#pragma once

#include "../defs.h"
#include "../globals.h"
#include <time.h>

enum LOG_TYPE {
    logt_err,
    logt_warn,
    logt_info,
};

bool open_log_file(void);
void close_log_file(void);

void sectr_log(enum LOG_TYPE log_type, const char *msg);

#define LOG_ERR(msg) if (!globals.no_log) sectr_log(logt_err, msg)

#define LOG_WARN(msg) if (!globals.no_log) sectr_log(logt_warn, msg)
#define LOG_INFO(msg) if (!globals.no_log) sectr_log(logt_info, msg)

