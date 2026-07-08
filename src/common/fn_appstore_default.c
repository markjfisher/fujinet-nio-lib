#include "fn_appstore_internal.h"
#include "fn_raw.h"

static uint8_t app_req_buf[FN_MAX_PACKET_SIZE - FN_HEADER_SIZE];
static uint8_t app_resp_buf[FN_MAX_PACKET_SIZE];

static uint8_t map_status(uint8_t status)
{
    switch (status) {
        case 0: return FN_OK;
        case 1: return FN_ERR_NOT_FOUND;
        case 2: return FN_ERR_INVALID;
        case 3: return FN_ERR_BUSY;
        case 4: return FN_ERR_NOT_READY;
        case 5: return FN_ERR_IO;
        case 6: return FN_ERR_TIMEOUT;
        case 7: return FN_ERR_INTERNAL;
        case 8: return FN_ERR_UNSUPPORTED;
        default: return FN_ERR_UNKNOWN;
    }
}

uint8_t *fn_appstore_request_buffer(void)
{
    return app_req_buf;
}

uint8_t *fn_appstore_response_buffer(void)
{
    return app_resp_buf;
}

uint16_t fn_appstore_request_capacity(void)
{
    return sizeof(app_req_buf);
}

uint16_t fn_appstore_response_capacity(void)
{
    return sizeof(app_resp_buf);
}

uint8_t fn_appstore_call(uint8_t command, uint16_t request_len, uint16_t *response_len)
{
    fn_raw_response_t raw;
    uint8_t result;

    result = fn_raw_call(FN_DEVICE_FILE,
                         command,
                         app_req_buf,
                         request_len,
                         app_resp_buf,
                         sizeof(app_resp_buf),
                         &raw);
    if (result != FN_OK) {
        return result;
    }
    *response_len = raw.payload_length;
    return map_status(raw.status);
}
