#include "../defs.h"

enum LOG_TYPE {
    logt_err,
    logt_warn,
    logt_info,
};

char *log_messgae(enum LOG_TYPE log_type);


