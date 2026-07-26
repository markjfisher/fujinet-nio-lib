#include <stdio.h>
#include <string.h>

#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

static unsigned call_count;
static uint8_t last_command;
static uint8_t last_payload[128];
static uint16_t last_payload_len;

static void put_u16le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xFFu);
    p[1] = (uint8_t)(value >> 8);
}

static void put_u32le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 0xFFu);
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

uint8_t fn_appstore_call(fn_appstore_io_t *io,
                         uint8_t command,
                         uint16_t request_len,
                         uint16_t *response_len)
{
    uint8_t *out = io->buffer;
    static const char data[] =
        "sd0:/\n"
        "fujinet.diller.org\n"
        "fujinet.online\n"
        "192.168.1.101\n";
    uint16_t data_len = (uint16_t)(sizeof(data) - 1);

    ++call_count;
    last_command = command;
    last_payload_len = request_len;
    memcpy(last_payload, io->buffer, request_len);

    out[0] = 1;
    out[1] = FN_APPSTORE_READ_EXISTS | FN_APPSTORE_READ_EOF;
    put_u16le(&out[2], 0);
    put_u32le(&out[4], 0);
    put_u16le(&out[8], data_len);
    memcpy(&out[10], data, data_len);
    *response_len = (uint16_t)(10 + data_len);
    return FN_OK;
}

int main(void)
{
    uint8_t scratch[96];
    fn_appstore_io_t io = { scratch, sizeof(scratch) };
    fn_appstore_read_t rr;
    uint8_t result;

    result = fn_appstore_read(&io, "config-nio", "hosts", 0,
                              scratch, 86, &rr);
    if (result != FN_OK) {
        puts("fn_appstore_read failed");
        return 1;
    }
    if (call_count != 1 || last_command != FN_CMD_APPSTORE_READ) {
        puts("wrong app-store call");
        return 1;
    }
    if (last_payload_len != 26 || last_payload[0] != 1) {
        puts("bad app-store request");
        return 1;
    }
    if (rr.flags != (FN_APPSTORE_READ_EXISTS | FN_APPSTORE_READ_EOF) ||
        rr.offset != 0 || rr.bytes_read != 54) {
        puts("app-store read metadata corrupted");
        return 1;
    }
    if (memcmp(scratch,
               "sd0:/\nfujinet.diller.org\nfujinet.online\n192.168.1.101\n",
               54) != 0) {
        puts("app-store read data mismatch");
        return 1;
    }

    puts("app-store read wire tests passed");
    return 0;
}
