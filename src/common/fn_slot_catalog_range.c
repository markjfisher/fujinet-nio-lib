#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

uint8_t fn_slot_catalog_range(fn_appstore_io_t *io,
                              uint8_t lower,
                              uint8_t upper,
                              uint8_t cursor,
                              uint8_t request_flags,
                              uint8_t max_uri_bytes,
                              uint16_t max_payload_bytes,
                              fn_slot_catalog_page_t *out)
{
    uint8_t *req;
    uint8_t *resp;
    uint16_t response_len;
    uint16_t variable_len;
    uint16_t entry_data_len;
    uint8_t result;

    if (out == 0 || fn_appstore_validate_io(io, 8) != FN_OK ||
        lower > upper || cursor < lower || cursor > upper ||
        max_uri_bytes == 0 || max_payload_bytes < 4 ||
        max_payload_bytes > (uint16_t)(io->capacity - 7)) {
        return FN_ERR_INVALID;
    }

    req = io->buffer;
    req[0] = FN_FILEPROTO_VERSION;
    req[1] = lower;
    req[2] = upper;
    req[3] = cursor;
    req[4] = request_flags;
    req[5] = max_uri_bytes;
    FN_PUT_LE16(&req[6], max_payload_bytes);

    result = fn_appstore_call(io, FN_CMD_SLOT_CATALOG_RANGE, 8, &response_len);
    if (result != FN_OK) {
        return result;
    }

    resp = io->buffer;
    if (response_len < 7 || resp[0] != FN_FILEPROTO_VERSION) {
        return FN_ERR_IO;
    }
    entry_data_len = FN_GET_LE16(&resp[5]);
    variable_len = (uint16_t)(resp[3] + entry_data_len);
    if ((uint16_t)(7 + variable_len) > response_len ||
        variable_len > max_payload_bytes) {
        return FN_ERR_IO;
    }

    out->flags = resp[1];
    out->next_index = resp[2];
    out->presence_len = resp[3];
    out->entry_count = resp[4];
    out->entry_data_len = entry_data_len;
    out->presence = &resp[7];
    out->entry_data = &resp[7 + resp[3]];
    return FN_OK;
}
