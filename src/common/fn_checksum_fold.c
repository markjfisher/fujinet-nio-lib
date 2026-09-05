#include "fn_internal.h"

uint16_t fn_checksum_fold(const uint8_t *data, uint16_t len,
                          uint16_t checksum)
{
    uint16_t i;

    for (i = 0; i < len; ++i) {
        checksum = (uint16_t)(checksum + data[i]);
        checksum = (uint16_t)((checksum >> 8) + (checksum & 0xFFu));
    }
    return checksum;
}
