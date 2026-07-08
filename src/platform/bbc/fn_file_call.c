#include <string.h>

#include "fn_bbc_internal.h"

static uint8_t map_device_status(uint8_t status)
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

uint8_t fn_bbc_device_call(uint8_t device,
                           uint8_t command,
                           const uint8_t *request,
                           uint16_t request_len,
                           uint8_t *response,
                           uint16_t response_capacity,
                           uint16_t *response_len)
{
    uint8_t block[16];
    uint8_t rom_status;

    if ((request_len != 0 && request == 0) ||
        (response_capacity != 0 && response == 0) ||
        response_len == 0) {
        return FN_ERR_INVALID;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_DEVICE_CALL;
    block[2] = device;
    block[3] = command;
    block[5] = (uint8_t)((unsigned)request & 0xFFu);
    block[6] = (uint8_t)(((unsigned)request >> 8) & 0xFFu);
    block[7] = (uint8_t)(request_len & 0xFFu);
    block[8] = (uint8_t)(request_len >> 8);
    block[9] = (uint8_t)((unsigned)response & 0xFFu);
    block[10] = (uint8_t)(((unsigned)response >> 8) & 0xFFu);
    block[11] = (uint8_t)(response_capacity & 0xFFu);
    block[12] = (uint8_t)(response_capacity >> 8);

    rom_status = fn_bbc_osword78(block);
    *response_len = (uint16_t)block[13] | ((uint16_t)block[14] << 8);
    if (rom_status != FN_BBC_STATUS_OK) {
        return fn_bbc_status_to_result(rom_status);
    }

    return map_device_status(block[4]);
}

uint8_t fn_bbc_file_call(uint8_t command,
                         const uint8_t *request,
                         uint16_t request_len,
                         uint8_t *response,
                         uint16_t response_capacity,
                         uint16_t *response_len)
{
    return fn_bbc_device_call(FN_DEVICE_FILE,
                              command,
                              request,
                              request_len,
                              response,
                              response_capacity,
                              response_len);
}
