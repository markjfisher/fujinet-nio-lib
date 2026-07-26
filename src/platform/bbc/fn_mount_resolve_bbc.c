#include "fn_bbc_internal.h"
#include "fn_mount_resolve_internal.h"

uint8_t fn_mount_resolve_call(fn_mount_resolve_io_t *io,
                              uint8_t command,
                              uint16_t request_len,
                              uint16_t *response_len)
{
    return fn_bbc_file_call(command,
                            io->buffer,
                            request_len,
                            io->buffer,
                            io->capacity,
                            response_len);
}
