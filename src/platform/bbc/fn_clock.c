#include "fn_bbc_internal.h"

#define FN_BBC_CLOCK_BUF_SIZE (FN_MAX_TIMEZONE_LEN + 32u)

static uint8_t clock_buf[FN_BBC_CLOCK_BUF_SIZE];

uint8_t fn_clock_exchange(uint8_t command,
                          const uint8_t *payload,
                          uint16_t payload_len,
                          uint8_t *status,
                          uint16_t *data_offset,
                          uint16_t *data_len)
{
    uint8_t result;

    result = fn_bbc_device_call(FN_DEVICE_CLOCK,
                                command,
                                payload,
                                payload_len,
                                clock_buf,
                                sizeof(clock_buf),
                                data_len);
    if (result != FN_OK) {
        return result;
    }

    *status = FN_OK;
    *data_offset = 0;
    return FN_OK;
}

uint8_t fn_clock_response_byte(uint16_t offset)
{
    return clock_buf[offset];
}
