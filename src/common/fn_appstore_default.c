#include "fn_appstore_internal.h"
#include "fn_raw.h"

static uint8_t map_status(uint8_t status)
{
    switch (status) {
        case 0: return FN_OK;
        case 1: return FN_ERR_NOT_FOUND;
        case 2: return FN_ERR_INVALID;
        case 3: return FN_ERR_BUSY;
        case 4: return FN_ERR_NOT_READY;
        case 5: return FN_ERR_IO;
        case 6: return FN_ERR_TIMEOUT;
        case 7: return FN_ERR_INTERNAL;
        case 8: return FN_ERR_UNSUPPORTED;
        default: return FN_ERR_UNKNOWN;
    }
}

uint8_t fn_appstore_call(fn_appstore_io_t *io,
                         uint8_t command,
                         uint16_t request_len,
                         uint16_t *response_len)
{
    fn_raw_response_t raw;
    uint8_t result;

    result = fn_raw_call(FN_DEVICE_FILE,
                         command,
                         io->buffer,
                         request_len,
                         io->buffer,
                         io->capacity,
                         &raw);
    if (result != FN_OK) {
        return result;
    }
    *response_len = raw.payload_length;
    return map_status(raw.status);
}
