#include <string.h>

#include "fn_bbc_internal.h"

uint8_t fn_set_body_length(uint16_t len)
{
    uint8_t block[16];

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_SET_BODY_LEN;
    block[2] = (uint8_t)(len & 0xFFu);
    block[3] = (uint8_t)(len >> 8);

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}

uint8_t fn_set_content_profile(uint8_t profile)
{
    uint8_t block[16];

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_CONTENT_TYPE;
    block[2] = profile;

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}
