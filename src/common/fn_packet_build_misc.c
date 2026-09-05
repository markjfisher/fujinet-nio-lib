#include "fn_internal.h"

uint16_t fn_build_close_packet(uint8_t *buffer, fn_handle_t handle)
{
    uint16_t offset;

    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_CLOSE, FN_HEADER_SIZE + 3);
    buffer[offset++] = FN_PROTOCOL_VERSION;
    buffer[offset++] = (uint8_t)(handle & 0xFF);
    buffer[offset++] = (uint8_t)((handle >> 8) & 0xFF);
    buffer[FN_CHECKSUM_OFFSET] = fn_calc_packet_checksum(buffer, offset);

    return offset;
}

uint16_t fn_build_info_packet(uint8_t *buffer, fn_handle_t handle)
{
    uint16_t offset;

    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_INFO, FN_HEADER_SIZE + 3);
    buffer[offset++] = FN_PROTOCOL_VERSION;
    buffer[offset++] = (uint8_t)(handle & 0xFF);
    buffer[offset++] = (uint8_t)((handle >> 8) & 0xFF);
    buffer[FN_CHECKSUM_OFFSET] = fn_calc_packet_checksum(buffer, offset);

    return offset;
}
