#include "sectr_err.h"

err_val_t err_val = {
    -1,
    NULL,
    "\0"
};

char *errorMessage(int code) {
    switch (code) {
        case ERRC_PermissionDenied: return "Permission denied";
        case ERRC_DeviceNotFound: return "Device not found";
        case ERRC_IOError: return "I/O Error";
        case ERRC_InvalidDevice: return "Invalid device";
        case ERRC_DeviceBusy: return "Device busy";
        case ERRC_InvalidInput: return "Invalid input";
        case ERRC_OutOfRange: return "Value out of range";
        case ERRC_ProcessFailure: return "Process failed to execute/load";
        case ERRC_FileNotFound: return "File not found";
        case ERRC_CorruptedData: return "Data is corrupted";
        case ERRC_DataUnavailable: return "Data is unavailable";
        case ERRC_NoInput: return "Input was empty";
        case ERRC_FailedInput: return "Input failed";
        case ERRC_AllocationFault: return "Allocation failed";
        default: return "Unknown Error";
    }
}

void set_err_vals(enum ErrorCode code, const char *std_err_msg, const char *fmt, ...) {
    err_val.errc = code;
    err_val.std_err_buff = std_err_msg ? std_err_msg : errorMessage(code);

    va_list args;
    va_start(args, fmt);
    vsnprintf(err_val.err_buff, sizeof err_val.err_buff, fmt, args);
    va_end(args);
}


int printError_chk(const char *file, int line, const char *func) {
    if (err_val.errc != -1) {
        fprintf(stderr, "%s[ERROR]%s %s (%s:%d, %s) - %s\n", RED, RST, err_val.std_err_buff, file, line, func, err_val.err_buff);
        set_err_vals(-1, NULL, "\0");
    }
    
    return 1;
}
