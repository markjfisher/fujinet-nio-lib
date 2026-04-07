#include "fn_internal.h"

uint16_t fn_build_header(uint8_t *buffer,
                         uint8_t device_id,
                         uint8_t command,
                         uint16_t total_len)
{
    uint16_t offset;

    offset = 0;
    buffer[offset++] = device_id;
    buffer[offset++] = command;
    buffer[offset++] = (uint8_t)(total_len & 0xFF);
    buffer[offset++] = (uint8_t)((total_len >> 8) & 0xFF);
    buffer[offset++] = 0;
    buffer[offset++] = 0;

    return offset;
}

uint16_t fn_add_param(uint8_t *buffer,
                      uint8_t param,
                      uint32_t value,
                      uint8_t size)
{
    uint16_t offset;

    offset = (uint16_t)param * FN_PARAM_DESC_SIZE;
    buffer[offset] = size;
    buffer[offset + 1] = 0;
    buffer[offset + 2] = (uint8_t)(value & 0xFF);
    buffer[offset + 3] = (uint8_t)((value >> 8) & 0xFF);

    return FN_PARAM_DESC_SIZE;
}
