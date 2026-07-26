#include "fn_mount_resolve_internal.h"
#include "fn_raw.h"

uint8_t fn_mount_resolve_call(fn_mount_resolve_io_t *io,
                              uint8_t command,
                              uint16_t request_len,
                              uint16_t *response_len)
{
    fn_raw_response_t response;
    uint8_t result;

    if (response_len == 0 || fn_mount_resolve_validate_io(io, request_len) != FN_OK) {
        return FN_ERR_INVALID;
    }
    result = fn_raw_call(FN_DEVICE_FILE,
                         command,
                         io->buffer,
                         request_len,
                         io->buffer,
                         io->capacity,
                         &response);
    if (result != FN_OK) {
        return result;
    }
    *response_len = response.payload_length;
    return response.status;
}
