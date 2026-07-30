#include "fujinet-nio.h"

uint8_t fn_slot_catalog_next_entry(const fn_slot_catalog_page_t *page,
                                   uint16_t *offset,
                                   fn_slot_catalog_entry_t *out)
{
    uint16_t pos;
    uint8_t uri_len;

    if (page == 0 || offset == 0 || out == 0) {
        return FN_ERR_INVALID;
    }
    pos = *offset;
    if ((uint16_t)(pos + 3) > page->entry_data_len) {
        return FN_ERR_IO;
    }
    uri_len = page->entry_data[pos + 2];
    if ((uint16_t)(pos + 3 + uri_len) > page->entry_data_len) {
        return FN_ERR_IO;
    }

    out->index = page->entry_data[pos];
    out->flags = page->entry_data[pos + 1];
    out->uri_len = uri_len;
    out->uri = &page->entry_data[pos + 3];
    *offset = (uint16_t)(pos + 3 + uri_len);
    return FN_OK;
}
