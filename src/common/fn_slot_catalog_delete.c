#include "fujinet-nio.h"
#include "fn_slot_catalog_internal.h"

uint8_t fn_slot_catalog_delete(fn_slot_catalog_io_t *io,
                               uint8_t index,
                               uint8_t *deleted)
{
    uint16_t response_len;
    uint8_t result;
    if (deleted == 0 || fn_slot_catalog_validate_io(io, 3) != FN_OK) {
        return FN_ERR_INVALID;
    }
    io->buffer[0] = FN_SLOT_CATALOG_PROTOCOL_VERSION;
    io->buffer[1] = index;
    result = fn_slot_catalog_call(
        io, FN_CMD_SLOT_CATALOG_DELETE, 2, &response_len);
    if (result != FN_OK) {
        return result;
    }
    if (response_len != 3 ||
        io->buffer[0] != FN_SLOT_CATALOG_PROTOCOL_VERSION ||
        io->buffer[2] != index) {
        return FN_ERR_IO;
    }
    *deleted = (uint8_t)(io->buffer[1] & 0x01);
    return FN_OK;
}
