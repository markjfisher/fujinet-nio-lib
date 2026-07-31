#include "fn_slot_catalog_internal.h"
#include "fn_bbc_internal.h"

uint8_t fn_slot_catalog_call(fn_slot_catalog_io_t *io,
                             uint8_t command,
                             uint16_t request_len,
                             uint16_t *response_len)
{
    return fn_bbc_device_call(FN_DEVICE_SLOT_CATALOG,
                              command,
                              io->buffer,
                              request_len,
                              io->buffer,
                              io->capacity,
                              response_len);
}
