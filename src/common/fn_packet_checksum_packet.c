#include "fn_internal.h"

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
