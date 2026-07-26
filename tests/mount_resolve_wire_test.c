#include <stdio.h>
#include <string.h>

#include "fujinet-nio.h"
#include "fn_protocol.h"
#include "fn_raw.h"

static unsigned call_count;
static uint8_t last_device;
static uint8_t last_command;
static uint8_t last_payload[512];
static uint16_t last_payload_len;

static void put_u16le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xFFu);
    p[1] = (uint8_t)(value >> 8);
}

uint8_t fn_raw_call(uint8_t device,
                    uint8_t command,
                    const void *payload,
                    uint16_t payload_length,
                    void *reply,
                    uint16_t reply_capacity,
                    fn_raw_response_t *response)
{
    uint8_t *out = (uint8_t *)reply;
    static const char resolved[] = "tnfs://192.168.1.101/bbc/bwc.ssd";
    static const char display[] = "/bbc/bwc.ssd";
    uint16_t off;

    ++call_count;
    last_device = device;
    last_command = command;
    last_payload_len = payload_length;
    memcpy(last_payload, payload, payload_length);

    if (device != FN_DEVICE_FILE || command != FN_CMD_FILE_RESOLVE_PATH) {
        return FN_ERR_INVALID;
    }
    if (reply_capacity < 8 + sizeof(resolved) + sizeof(display)) {
        return FN_ERR_INVALID;
    }

    out[0] = 1;
    out[1] = 0x02;
    out[2] = 0;
    out[3] = 0;
    off = 4;
    put_u16le(&out[off], (uint16_t)(sizeof(resolved) - 1));
    off = (uint16_t)(off + 2);
    memcpy(&out[off], resolved, sizeof(resolved) - 1);
    off = (uint16_t)(off + sizeof(resolved) - 1);
    put_u16le(&out[off], (uint16_t)(sizeof(display) - 1));
    off = (uint16_t)(off + 2);
    memcpy(&out[off], display, sizeof(display) - 1);
    off = (uint16_t)(off + sizeof(display) - 1);

    response->status = FN_OK;
    response->payload_length = off;
    return FN_OK;
}

static int test_resolve_mount_target(void)
{
    uint8_t scratch[128];
    fn_mount_resolve_io_t io = { scratch, sizeof(scratch) };
    char canonical[96];
    char display[32];
    uint8_t flags = 0;

    call_count = 0;
    if (fn_resolve_mount_target(&io,
                                "192.168.1.101",
                                "bbc/",
                                "bwc.ssd",
                                canonical,
                                sizeof(canonical),
                                display,
                                sizeof(display),
                                &flags) != FN_OK) {
        puts("resolve_mount_target failed");
        return 1;
    }
    if (call_count != 1 || last_device != FN_DEVICE_FILE || last_command != FN_CMD_FILE_RESOLVE_PATH) {
        puts("resolve_mount_target wrong call");
        return 1;
    }
    if (last_payload[0] != 1) {
        puts("resolve_mount_target bad version");
        return 1;
    }
    if (memcmp(canonical, "tnfs://192.168.1.101/bbc/bwc.ssd", 33) != 0) {
        puts("resolve_mount_target canonical mismatch");
        return 1;
    }
    if (strcmp(display, "/bbc/bwc.ssd") != 0 || flags != 0x02) {
        puts("resolve_mount_target display mismatch");
        return 1;
    }
    return 0;
}

static int test_format_mount_display(void)
{
    uint8_t scratch[128];
    fn_mount_resolve_io_t io = { scratch, sizeof(scratch) };
    char display[32];

    call_count = 0;
    if (fn_format_mount_display(&io,
                                "tnfs://192.168.1.101/bbc/bwc.ssd",
                                display,
                                sizeof(display),
                                0) != FN_OK) {
        puts("format_mount_display failed");
        return 1;
    }
    if (call_count != 1 || strcmp(display, "/bbc/bwc.ssd") != 0) {
        puts("format_mount_display mismatch");
        return 1;
    }
    return 0;
}

int main(void)
{
    if (test_resolve_mount_target()) return 1;
    if (test_format_mount_display()) return 1;
    puts("mount resolve wire tests passed");
    return 0;
}
