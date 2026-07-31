#include "fn_slot_catalog_internal.h"

uint8_t fn_slot_catalog_validate_io(
    fn_slot_catalog_io_t *io, uint16_t min_capacity)
{
    if (io == 0 || io->buffer == 0 || io->capacity < min_capacity) {
        return FN_ERR_INVALID;
    }
    return FN_OK;
}
