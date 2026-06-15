#include <string.h>

#include "fn_bbc_internal.h"

uint8_t fn_bbc_status_to_result(uint8_t status)
{
    switch (status) {
        case FN_BBC_STATUS_OK:
            return FN_OK;
        case FN_BBC_STATUS_BAD_CALL:
            return FN_ERR_INVALID;
        case FN_BBC_STATUS_JSON_FAILED:
            return FN_ERR_IO;
        case FN_BBC_STATUS_BAD_CHANNEL:
            return FN_ERR_NOT_FOUND;
        default:
            return FN_ERR_IO;
    }
}

uint8_t fn_bbc_probe_rom(void)
{
    uint8_t block[16];

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_CONTENT_TYPE;
    block[1] = 0xFF;
    block[2] = 0;

    return fn_bbc_osword78(block) == FN_BBC_STATUS_OK;
}

int fn_bbc_open_flags(uint8_t method)
{
    switch (method) {
        case 0:
        case FN_METHOD_POST:
            return FN_BBC_OPEN_UPDATE;
        case FN_METHOD_GET:
            return FN_BBC_OPEN_READ;
        case FN_METHOD_PUT:
            return FN_BBC_OPEN_WRITE;
        default:
            return -1;
    }
}

uint8_t fn_bbc_arm_open_url(const char *url, uint16_t len)
{
    uint8_t block[16];

    if (len > FN_BBC_OSWORD_STR_MAX) {
        return FN_ERR_URL_TOO_LONG;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_SET_OPEN_URL;
    block[2] = (uint8_t)((unsigned)url & 0xFFu);
    block[3] = (uint8_t)(((unsigned)url >> 8) & 0xFFu);
    block[4] = (uint8_t)(len & 0xFFu);
    block[5] = (uint8_t)(len >> 8);

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}
