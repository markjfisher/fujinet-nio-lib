#include <string.h>

#include "fn_internal.h"

uint16_t fn_build_open_packet(uint8_t *buffer,
                              uint8_t method,
                              uint8_t flags,
                              const char *url)
{
    uint16_t offset;
    uint16_t url_len;
    uint16_t payload_len;
    uint16_t total_len;

    url_len = 0;
    while (url[url_len] != '\0') {
        ++url_len;
        if (url_len > FN_MAX_URL_LEN) {
            return 0;
        }
    }

    payload_len = 1 + 1 + 1 + 2 + url_len + 2 + 4 + 2;
    total_len = FN_HEADER_SIZE + payload_len;
    offset = fn_build_header(buffer, FN_DEVICE_NETWORK, FN_CMD_OPEN, total_len);

    buffer[offset++] = FN_PROTOCOL_VERSION;
    buffer[offset++] = method;
    buffer[offset++] = flags;
    buffer[offset++] = (uint8_t)(url_len & 0xFF);
    buffer[offset++] = (uint8_t)((url_len >> 8) & 0xFF);
    memcpy(buffer + offset, url, url_len);
    offset += url_len;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;
    buffer[offset++] = 0;

    buffer[FN_CHECKSUM_OFFSET] = fn_calc_packet_checksum(buffer, offset);
    return offset;
}
