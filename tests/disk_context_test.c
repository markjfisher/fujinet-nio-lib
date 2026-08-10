#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fn_raw.h"
#include "fujinet-nio.h"

typedef struct exchange_fixture {
    fn_disk_client_context_t *nested_context;
    uint8_t nested_slot;
    uint8_t invoke_nested;
    uint8_t request_preserved;
} exchange_fixture_t;

uint8_t fn_raw_call(uint8_t device, uint8_t command, const void *payload,
                    uint16_t payload_length, void *reply,
                    uint16_t reply_capacity, fn_raw_response_t *response)
{
    (void)device; (void)command; (void)payload; (void)payload_length;
    (void)reply; (void)reply_capacity; (void)response;
    return FN_ERR_UNSUPPORTED;
}

static uint8_t checksum(const uint8_t *data, uint16_t length)
{
    uint16_t sum = 0;
    uint16_t i;
    for (i = 0; i < length; ++i) {
        sum += data[i];
        sum = (sum >> 8) + (sum & 0xFFu);
    }
    return (uint8_t)sum;
}

static uint8_t exchange(void *opaque, const uint8_t *request,
                        uint16_t request_length, uint8_t *response,
                        uint16_t response_capacity, uint16_t *response_length)
{
    exchange_fixture_t *fixture = opaque;
    uint8_t saved[8];
    fn_disk_info_t nested_info;
    uint8_t slot;
    uint16_t length = 20;

    if (request_length != 8 || response_capacity < length ||
        request[0] != 0xFC || request[1] != 0x05) return FN_ERR_IO;
    memcpy(saved, request, sizeof(saved));
    slot = request[7];
    if (fixture->invoke_nested) {
        fixture->invoke_nested = 0;
        if (fn_disk_info_context(fixture->nested_context,
                                 fixture->nested_slot,
                                 &nested_info) != FN_OK ||
            nested_info.slot != fixture->nested_slot) return FN_ERR_IO;
    }
    fixture->request_preserved =
        memcmp(saved, request, sizeof(saved)) == 0 ? 1 : 0;

    memset(response, 0, length);
    response[0] = 0xFC;
    response[1] = 0x05;
    response[2] = (uint8_t)length;
    response[5] = 1;
    response[6] = 0;
    response[7] = FN_DISK_PROTOCOL_VERSION;
    response[8] = FN_DISK_FLAG_MOUNTED | FN_DISK_FLAG_READONLY;
    response[11] = slot;
    response[12] = FN_DISK_TYPE_RAW;
    response[13] = 0;
    response[14] = 2;
    response[15] = 0xE0;
    response[16] = 0x06;
    response[4] = checksum(response, length);
    *response_length = length;
    return FN_OK;
}

int main(void)
{
    fn_disk_client_context_t first;
    fn_disk_client_context_t second;
    exchange_fixture_t first_fixture = { &second, 2, 1, 0 };
    exchange_fixture_t second_fixture = { NULL, 0, 0, 0 };
    fn_disk_info_t info;

    if (fn_disk_context_init(&first, exchange, &first_fixture) != FN_OK ||
        fn_disk_context_init(&second, exchange, &second_fixture) != FN_OK ||
        fn_disk_info_context(&first, 1, &info) != FN_OK || info.slot != 1 ||
        !first_fixture.request_preserved ||
        !second_fixture.request_preserved) {
        puts("interleaved DiskDevice contexts crossed state");
        return 1;
    }
    puts("independent DiskDevice contexts remain isolated");
    return 0;
}
