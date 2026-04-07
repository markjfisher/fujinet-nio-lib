#include "fujinet-nio.h"

const char *fn_error_string(uint8_t error)
{
    switch (error) {
        case FN_OK:               return "OK";
        case FN_ERR_NOT_FOUND:    return "Device not found";
        case FN_ERR_INVALID:      return "Invalid parameter";
        case FN_ERR_BUSY:         return "Device busy";
        case FN_ERR_NOT_READY:    return "Not ready";
        case FN_ERR_IO:           return "I/O error";
        case FN_ERR_TIMEOUT:      return "Timeout";
        case FN_ERR_INTERNAL:     return "Internal error";
        case FN_ERR_UNSUPPORTED:  return "Unsupported";
        case FN_ERR_TRANSPORT:    return "Transport error";
        case FN_ERR_URL_TOO_LONG: return "URL too long";
        case FN_ERR_NO_HANDLES:   return "No free handles";
        default:                  return "Unknown error";
    }
}

const char *fn_version(void)
{
    return "1.0.0";
}
