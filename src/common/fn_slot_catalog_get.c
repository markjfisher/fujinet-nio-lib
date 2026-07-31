#include "fujinet-nio.h"
#include "fn_slot_catalog_internal.h"

uint8_t fn_slot_catalog_get(fn_slot_catalog_io_t *io,
                            uint8_t index,
                            fn_slot_catalog_entry_t *out)
{
    uint16_t response_len;
    uint8_t result;
    if (out == 0 || fn_slot_catalog_validate_io(io, 5) != FN_OK) {
        return FN_ERR_INVALID;
    }
    io->buffer[0] = FN_SLOT_CATALOG_PROTOCOL_VERSION;
    io->buffer[1] = index;
    result = fn_slot_catalog_call(
        io, FN_CMD_SLOT_CATALOG_GET, 2, &response_len);
    if (result != FN_OK) {
        return result;
    }
    result = fn_slot_catalog_parse_entry(io, response_len, out);
    if (result == FN_OK && out->index != index) {
        return FN_ERR_IO;
    }
    return result;
}
