#include "fn_slot_catalog_internal.h"

uint8_t fn_slot_catalog_parse_entry(
    fn_slot_catalog_io_t *io,
    uint16_t response_len,
    fn_slot_catalog_entry_t *out)
{
    uint16_t uri_len;
    uint8_t *resp;
    if (out == 0 || response_len < 5) {
        return FN_ERR_IO;
    }
    resp = io->buffer;
    if (resp[0] != FN_SLOT_CATALOG_PROTOCOL_VERSION) {
        return FN_ERR_IO;
    }
    uri_len = FN_GET_LE16(&resp[3]);
    if ((uint16_t)(5 + uri_len) != response_len) {
        return FN_ERR_IO;
    }
    out->flags = resp[1];
    out->index = resp[2];
    out->uri_len = uri_len;
    out->uri = &resp[5];
    return FN_OK;
}
