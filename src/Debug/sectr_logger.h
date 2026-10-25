#include "../defs.h"
#include "sectr_err.h"
#include "../globals.h"
#include <time.h>

enum LOG_TYPE {
    logt_err,
    logt_warn,
    logt_info,
};

bool open_log_file(void);
void close_log_file(void);

static const char *log_messgae(enum LOG_TYPE log_type);
static char *get_time(void);

void log(enum LOG_TYPE log_type, const char *msg, const char *func);

#define LOG_ERR(msg) log(logt_err, msg, __func__)
#define LOG_WARN(msg) log(logt_warn, msg, __func__)
#define LOG_INFO(msg) log(logt_info, msg, __func__)
