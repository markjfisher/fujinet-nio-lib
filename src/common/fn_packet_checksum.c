#include "fn_internal.h"

static uint16_t fn_checksum_fold(const uint8_t *data, uint16_t len,
                                 uint16_t checksum)
{
    uint16_t i;

    for (i = 0; i < len; ++i) {
        checksum = (uint16_t)(checksum + data[i]);
        checksum = (uint16_t)((checksum >> 8) + (checksum & 0xFFu));
    }
    return checksum;
}

uint8_t fn_calc_checksum(const uint8_t *data, uint16_t len)
{
    return (uint8_t)fn_checksum_fold(data, len, 0);
}

uint8_t fn_calc_packet_checksum(const uint8_t *packet, uint16_t len)
{
    uint16_t checksum;

    if (len <= FN_CHECKSUM_OFFSET) return fn_calc_checksum(packet, len);
    checksum = fn_checksum_fold(packet, FN_CHECKSUM_OFFSET, 0);
    checksum = fn_checksum_fold(packet + FN_CHECKSUM_OFFSET + 1,
                                (uint16_t)(len - FN_CHECKSUM_OFFSET - 1),
                                checksum);
    return (uint8_t)checksum;
}
