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

static const char *log_messgae(enum LOG_TYPE log_type);
static char *get_time(void);

void sectr_log(enum LOG_TYPE log_type, const char *msg);

#define LOG_ERR(msg) sectr_log(logt_err, msg)

#define LOG_WARN(msg) sectr_log(logt_warn, msg, __func__)
#define LOG_INFO(msg) sectr_log(logt_info, msg, __func__)

