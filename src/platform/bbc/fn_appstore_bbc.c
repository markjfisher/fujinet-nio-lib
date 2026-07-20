#include "fn_appstore_internal.h"
#include "fn_bbc_internal.h"

uint8_t fn_appstore_call(fn_appstore_io_t *io,
                         uint8_t command,
                         uint16_t request_len,
                         uint16_t *response_len)
{
    if (fn_appstore_validate_io(io, request_len) != FN_OK) {
        return FN_ERR_INVALID;
    }

    return fn_bbc_file_call(command,
                            io->buffer,
                            request_len,
                            io->buffer,
                            io->capacity,
                            response_len);
}
