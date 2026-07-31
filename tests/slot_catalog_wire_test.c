#include <stdio.h>
#include <string.h>

#include "fujinet-nio.h"
#include "fn_slot_catalog_internal.h"

static uint8_t last_command;
static uint8_t last_payload[128];
static uint16_t last_payload_len;

uint8_t fn_slot_catalog_call(fn_slot_catalog_io_t *io,
                             uint8_t command,
                             uint16_t request_len,
                             uint16_t *response_len)
{
    uint8_t *out = io->buffer;
    uint8_t requested_index = io->buffer[1];
    static const char uri[] = "games/elite.ssd";

    last_command = command;
    last_payload_len = request_len;
    memcpy(last_payload, io->buffer, request_len);

    if (command == FN_CMD_SLOT_CATALOG_GET ||
        command == FN_CMD_SLOT_CATALOG_PUT) {
        out[0] = FN_SLOT_CATALOG_PROTOCOL_VERSION;
        out[1] = FN_SLOT_CATALOG_ENTRY_VALID;
        out[2] = requested_index;
        out[3] = (uint8_t)(sizeof(uri) - 1);
        out[4] = 0;
        memcpy(&out[5], uri, sizeof(uri) - 1);
        *response_len = (uint16_t)(5 + sizeof(uri) - 1);
        return FN_OK;
    }
    if (command == FN_CMD_SLOT_CATALOG_DELETE) {
        out[0] = FN_SLOT_CATALOG_PROTOCOL_VERSION;
        out[1] = 1;
        out[2] = requested_index;
        *response_len = 3;
        return FN_OK;
    }

    out[0] = FN_SLOT_CATALOG_PROTOCOL_VERSION;
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
    uint8_t scratch[128];
    fn_slot_catalog_io_t io = { scratch, sizeof(scratch) };
    fn_slot_catalog_page_t page;
    fn_slot_catalog_entry_t entry;
    uint16_t offset = 0;
    uint8_t result;
    static const uint8_t expected_request[] = {
        1, 64, 72, 64, FN_SLOT_CATALOG_TAIL_URI, 30, 40, 0
    };
    static const uint8_t expected_put[] = {
        1, 100, FN_SLOT_CATALOG_ENTRY_READ_ONLY, 9, 0,
        'e', 'l', 'i', 't', 'e', '.', 's', 's', 'd'
    };
    uint8_t deleted;

    result = fn_slot_catalog_get(&io, 69, &entry);
    if (result != FN_OK || last_command != FN_CMD_SLOT_CATALOG_GET ||
        last_payload_len != 2 || last_payload[0] != 1 ||
        last_payload[1] != 69 || entry.index != 69 ||
        entry.uri_len != 15) {
        puts("slot catalogue get mismatch");
        return 1;
    }

    result = fn_slot_catalog_put(&io, 100, FN_SLOT_CATALOG_ENTRY_READ_ONLY,
                                 "elite.ssd", &entry);
    if (result != FN_OK || last_command != FN_CMD_SLOT_CATALOG_PUT ||
        last_payload_len != sizeof(expected_put) ||
        memcmp(last_payload, expected_put, sizeof(expected_put)) != 0 ||
        entry.index != 100) {
        puts("slot catalogue put mismatch");
        return 1;
    }

    result = fn_slot_catalog_delete(&io, 33, &deleted);
    if (result != FN_OK || !deleted ||
        last_command != FN_CMD_SLOT_CATALOG_DELETE ||
        last_payload_len != 2 || last_payload[0] != 1 ||
        last_payload[1] != 33) {
        puts("slot catalogue delete mismatch");
        return 1;
    }

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
