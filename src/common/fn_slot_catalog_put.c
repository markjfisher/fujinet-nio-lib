#include "fujinet-nio.h"
#include "fn_slot_catalog_internal.h"

#include <string.h>

uint8_t fn_slot_catalog_put(fn_slot_catalog_io_t *io,
                            uint8_t index,
                            uint8_t flags,
                            const char *target,
                            fn_slot_catalog_entry_t *out)
{
    uint16_t target_len;
    uint16_t response_len;
    uint8_t result;
    if (target == 0 || out == 0 ||
        fn_slot_catalog_validate_io(io, 5) != FN_OK) {
        return FN_ERR_INVALID;
    }
    target_len = (uint16_t)strlen(target);
    if (target_len == 0 || target_len > (uint16_t)(io->capacity - 5)) {
        return FN_ERR_INVALID;
    }
    io->buffer[0] = FN_SLOT_CATALOG_PROTOCOL_VERSION;
    io->buffer[1] = index;
    io->buffer[2] = flags;
    FN_PUT_LE16(&io->buffer[3], target_len);
    memcpy(&io->buffer[5], target, target_len);
    result = fn_slot_catalog_call(
        io, FN_CMD_SLOT_CATALOG_PUT,
        (uint16_t)(5 + target_len), &response_len);
    if (result != FN_OK) {
        return result;
    }
    result = fn_slot_catalog_parse_entry(io, response_len, out);
    if (result == FN_OK && out->index != index) {
        return FN_ERR_IO;
    }
    return result;
}
