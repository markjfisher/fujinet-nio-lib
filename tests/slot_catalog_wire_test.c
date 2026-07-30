#include <stdio.h>
#include <string.h>

#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

static uint8_t last_command;
static uint8_t last_payload[8];
static uint16_t last_payload_len;

uint8_t fn_appstore_call(fn_appstore_io_t *io,
                         uint8_t command,
                         uint16_t request_len,
                         uint16_t *response_len)
{
    uint8_t *out = io->buffer;
    static const char uri[] = "games/elite.ssd";

    last_command = command;
    last_payload_len = request_len;
    memcpy(last_payload, io->buffer, request_len);

    out[0] = FN_FILEPROTO_VERSION;
    out[1] = 0;
    out[2] = 70;
    out[3] = 2;
    out[4] = 1;
    out[5] = (uint8_t)(3 + sizeof(uri) - 1);
    out[6] = 0;
    out[7] = 0x20;
    out[8] = 0;
    out[9] = 69;
    out[10] = FN_SLOT_CATALOG_ENTRY_VALID;
    out[11] = (uint8_t)(sizeof(uri) - 1);
    memcpy(&out[12], uri, sizeof(uri) - 1);
    *response_len = (uint16_t)(12 + sizeof(uri) - 1);
    return FN_OK;
}

int main(void)
{
    uint8_t scratch[64];
    fn_appstore_io_t io = { scratch, sizeof(scratch) };
    fn_slot_catalog_page_t page;
    fn_slot_catalog_entry_t entry;
    uint16_t offset = 0;
    uint8_t result;
    static const uint8_t expected_request[] = {
        1, 64, 72, 64, FN_SLOT_CATALOG_TAIL_URI, 30, 40, 0
    };

    result = fn_slot_catalog_range(&io, 64, 72, 64,
                                   FN_SLOT_CATALOG_TAIL_URI, 30, 40, &page);
    if (result != FN_OK ||
        last_command != FN_CMD_SLOT_CATALOG_RANGE ||
        last_payload_len != sizeof(expected_request) ||
        memcmp(last_payload, expected_request, sizeof(expected_request)) != 0) {
        puts("slot catalogue request mismatch");
        return 1;
    }
    if (page.flags != 0 || page.next_index != 70 ||
        page.presence_len != 2 || page.entry_count != 1 ||
        page.presence[0] != 0x20 || page.presence[1] != 0) {
        puts("slot catalogue response metadata mismatch");
        return 1;
    }
    result = fn_slot_catalog_next_entry(&page, &offset, &entry);
    if (result != FN_OK || entry.index != 69 ||
        entry.flags != FN_SLOT_CATALOG_ENTRY_VALID ||
        entry.uri_len != 15 ||
        memcmp(entry.uri, "games/elite.ssd", 15) != 0 ||
        offset != 18) {
        puts("slot catalogue entry parse mismatch");
        return 1;
    }

    puts("slot catalogue wire tests passed");
    return 0;
}
