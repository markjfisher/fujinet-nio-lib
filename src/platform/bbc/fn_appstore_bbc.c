#include "fn_appstore_internal.h"
#include "fn_bbc_internal.h"

static uint8_t app_buf[FN_MAX_PACKET_SIZE];

uint8_t *fn_appstore_request_buffer(void)
{
    return app_buf;
}

uint8_t *fn_appstore_response_buffer(void)
{
    return app_buf;
}

uint16_t fn_appstore_request_capacity(void)
{
    return (uint16_t)(FN_MAX_PACKET_SIZE - FN_HEADER_SIZE);
}

uint16_t fn_appstore_response_capacity(void)
{
    return sizeof(app_buf);
}

uint8_t fn_appstore_call(uint8_t command, uint16_t request_len, uint16_t *response_len)
{
    if (request_len > fn_appstore_request_capacity()) {
        return FN_ERR_INVALID;
    }

    return fn_bbc_file_call(command,
                            app_buf,
                            request_len,
                            app_buf,
                            sizeof(app_buf),
                            response_len);
}
