#pragma once

#include "../defs.h"

enum ErrorCode {
    ERRC_PermissionDenied,
    ERRC_DeviceNotFound,
    ERRC_IOError,
    ERRC_InvalidDevice,
    ERRC_DeviceBusy,
    ERRC_InvalidInput,
    ERRC_OutOfRange,
    ERRC_ProcessFailure,
    ERRC_FileNotFound,
    ERRC_CorruptedData,
    ERRC_DataUnavailable,
    ERRC_NoInput,
    ERRC_FailedInput,
    ERRC_AllocationFault,
    ERRC_Unknown, 
    ERRC_Undefined
};

#define ERR_BUFF_SIZE 1024

typedef struct err_val_t {
    int errc;
    char *std_err_buff;
    char err_buff[ERR_BUFF_SIZE];

} err_val_t;

extern err_val_t err_val;

char *errorMessage(int code);

void set_err_vals(int code, const char *extra_msg);

int printError(const char *file, int line, const char *func);

#define ERR(code, extra_msg) set_err_vals(code, extra_msg)

#define PERR() printError(__FILE__, __LINE__, __func__)
