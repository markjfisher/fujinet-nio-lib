#include <string.h>

#include "fn_internal.h"

uint16_t fn_build_read_packet(uint8_t *buffer,
                              fn_handle_t handle,
                              uint32_t offset_val,
                              uint16_t max_bytes)
{
    uint16_t offset;

    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_READ, FN_HEADER_SIZE + 9);
    buffer[offset++] = FN_PROTOCOL_VERSION;
    buffer[offset++] = (uint8_t)(handle & 0xFF);
    buffer[offset++] = (uint8_t)((handle >> 8) & 0xFF);
    buffer[offset++] = (uint8_t)(offset_val & 0xFF);
    buffer[offset++] = (uint8_t)((offset_val >> 8) & 0xFF);
    buffer[offset++] = (uint8_t)((offset_val >> 16) & 0xFF);
    buffer[offset++] = (uint8_t)((offset_val >> 24) & 0xFF);
    buffer[offset++] = (uint8_t)(max_bytes & 0xFF);
    buffer[offset++] = (uint8_t)((max_bytes >> 8) & 0xFF);
    buffer[FN_CHECKSUM_OFFSET] = fn_calc_packet_checksum(buffer, offset);

    return offset;
}

uint16_t fn_build_write_packet(uint8_t *buffer,
                               fn_handle_t handle,
                               uint32_t offset_val,
                               const uint8_t *data,
                               uint16_t data_len)
{
    uint16_t offset;

    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_WRITE,
                             (uint16_t)(FN_HEADER_SIZE + 1 + 2 + 4 + 2 + data_len));
    buffer[offset++] = FN_PROTOCOL_VERSION;
    buffer[offset++] = (uint8_t)(handle & 0xFF);
    buffer[offset++] = (uint8_t)((handle >> 8) & 0xFF);
    buffer[offset++] = (uint8_t)(offset_val & 0xFF);
    buffer[offset++] = (uint8_t)((offset_val >> 8) & 0xFF);
    buffer[offset++] = (uint8_t)((offset_val >> 16) & 0xFF);
    buffer[offset++] = (uint8_t)((offset_val >> 24) & 0xFF);
    buffer[offset++] = (uint8_t)(data_len & 0xFF);
    buffer[offset++] = (uint8_t)((data_len >> 8) & 0xFF);

    if (data_len > 0 && data != NULL) {
        memcpy(buffer + offset, data, data_len);
        offset += data_len;
    }

    buffer[FN_CHECKSUM_OFFSET] = fn_calc_packet_checksum(buffer, offset);
    return offset;
}
