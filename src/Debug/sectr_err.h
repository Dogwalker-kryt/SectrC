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
    ERRC_Undefined,
    ERRC_ZERO = -1
};

#define ERR_BUFF_SIZE KiB

typedef struct err_val_t {
    int errc;
    const char *std_err_buff;
    char err_buff[ERR_BUFF_SIZE];

} err_val_t;

extern err_val_t err_val;

const char *errorMessage(int code);

void set_err_vals(enum ErrorCode code, const char *std_err_msg, const char *fmt, ...);

int printError_chk(const char *file, int line, const char *func);

#define ERR(code, ...) set_err_vals((code), __VA_ARGS__)

#define PERR() printError_chk(__FILE__, __LINE__, __func__)
