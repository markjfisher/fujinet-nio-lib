#include <stdio.h>
#include <string.h>

#include "fn_internal.h"
#include "fn_platform.h"
#include "fn_protocol.h"
#include "fujinet-nio.h"

/* Drives fn_write() through a fake transport that checks each request fits one
 * packet and answers it the way the firmware would. */

#define TEST_HANDLE 0x0102

static int failures;
static unsigned exchanges;
static uint16_t largest_request;
static uint16_t accept_limit;
static uint32_t body_received;
static uint8_t body[3000];

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #condition); \
            ++failures; \
        } \
    } while (0)

/* A Write reply: version, flags, reserved, handle, offset echo, then the
 * length the firmware accepted. */
static uint16_t write_reply(uint32_t offset, uint16_t accepted)
{
    uint8_t *r = _fn_transport_ctx.response;
    uint16_t o = fn_build_header(r, FN_DEVICE_NETWORK, FN_CMD_WRITE, FN_HEADER_SIZE + 12);

    r[o++] = FN_PROTOCOL_VERSION;
    r[o++] = 0;
    r[o++] = 0;
    r[o++] = 0;
    r[o++] = (uint8_t)(TEST_HANDLE & 0xFF);
    r[o++] = (uint8_t)(TEST_HANDLE >> 8);
    r[o++] = (uint8_t)offset;
    r[o++] = (uint8_t)(offset >> 8);
    r[o++] = (uint8_t)(offset >> 16);
    r[o++] = (uint8_t)(offset >> 24);
    r[o++] = (uint8_t)(accepted & 0xFF);
    r[o++] = (uint8_t)(accepted >> 8);
    return o;
}

uint8_t fn_transport_exchange(void)
{
    const uint8_t *q = _fn_transport_ctx.request;
    uint16_t req_len = _fn_transport_ctx.req_len, o, n, accepted;
    uint32_t offset = FN_READ_LE32(q, FN_HEADER_SIZE + 3);

    ++exchanges;
    if (req_len > largest_request) {
        largest_request = req_len;
    }
    if (req_len > FN_MAX_PACKET_SIZE) {
        return FN_ERR_INVALID;
    }
    n = FN_READ_LE16(q, FN_HEADER_SIZE + 7);
    accepted = (accept_limit && n > accept_limit) ? accept_limit : n;
    CHECK(offset == body_received);
    CHECK((uint16_t)(FN_HEADER_SIZE + 9 + n) == req_len);
    CHECK(memcmp(q + FN_HEADER_SIZE + 9, body + body_received, accepted) == 0);
    body_received += accepted;
    o = write_reply(offset, accepted);
    _fn_transport_ctx.response[FN_CHECKSUM_OFFSET] =
        fn_calc_packet_checksum(_fn_transport_ctx.response, o);
    _fn_transport_ctx.resp_len = o;
    return FN_OK;
}

static void reset(void)
{
    uint16_t i;

    memset(_fn_sessions, 0, sizeof _fn_sessions);
    _fn_sessions[0].active = 1;
    _fn_sessions[0].handle = TEST_HANDLE;
    _fn_initialized = 1;
    exchanges = 0;
    largest_request = 0;
    accept_limit = 0;
    body_received = 0;
    for (i = 0; i < sizeof body; ++i) {
        body[i] = (uint8_t)(i * 7);
    }
}

static void test_write_of_one_full_packet(void)
{
    uint16_t written = 0;

    reset();
    CHECK(fn_write(TEST_HANDLE, 0, body, FN_WRITE_DATA_MAX, &written) == FN_OK);
    CHECK(written == FN_WRITE_DATA_MAX);
    CHECK(exchanges == 1);
    CHECK(largest_request == FN_MAX_PACKET_SIZE);
}

static void test_write_one_byte_over_a_packet(void)
{
    uint16_t written = 0;

    reset();
    CHECK(fn_write(TEST_HANDLE, 0, body, FN_WRITE_DATA_MAX + 1, &written) == FN_OK);
    CHECK(written == FN_WRITE_DATA_MAX + 1);
    CHECK(body_received == FN_WRITE_DATA_MAX + 1);
    CHECK(exchanges == 2);
    CHECK(largest_request <= FN_MAX_PACKET_SIZE);
}

static void test_big_write_is_split(void)
{
    uint16_t written = 0;

    reset();
    CHECK(fn_write(TEST_HANDLE, 0, body, sizeof body, &written) == FN_OK);
    CHECK(written == sizeof body);
    CHECK(body_received == sizeof body);
    CHECK(largest_request <= FN_MAX_PACKET_SIZE);
    CHECK(exchanges == (sizeof body + FN_WRITE_DATA_MAX - 1) / FN_WRITE_DATA_MAX);
    CHECK(_fn_sessions[0].write_offset == sizeof body);
}

static void test_partial_accepts_still_complete(void)
{
    uint16_t written = 0;

    reset();
    accept_limit = 700;
    CHECK(fn_write(TEST_HANDLE, 0, body, sizeof body, &written) == FN_OK);
    CHECK(written == sizeof body);
    CHECK(body_received == sizeof body);
    CHECK(largest_request <= FN_MAX_PACKET_SIZE);
}

static void test_half_close_sends_no_data(void)
{
    uint16_t written = 1;

    reset();
    CHECK(fn_write(TEST_HANDLE, 0, NULL, 0, &written) == FN_OK);
    CHECK(written == 0);
    CHECK(exchanges == 1);
    CHECK(largest_request == FN_HEADER_SIZE + 9);
}

int main(void)
{
    test_write_of_one_full_packet();
    test_write_one_byte_over_a_packet();
    test_big_write_is_split();
    test_partial_accepts_still_complete();
    test_half_close_sends_no_data();
    if (failures) {
        printf("write_wire_test: %d failure(s)\n", failures);
        return 1;
    }
    puts("write_wire_test: OK");
    return 0;
}
