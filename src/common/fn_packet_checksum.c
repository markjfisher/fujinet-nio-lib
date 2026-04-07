#include "fn_internal.h"

uint8_t fn_calc_checksum(const uint8_t *data, uint16_t len)
{
    uint16_t chk;
    uint16_t i;

    chk = 0;
    for (i = 0; i < len; ++i) {
        chk += data[i];
        chk = ((chk >> 8) + (chk & 0xFF)) & 0xFFFF;
    }

    return (uint8_t)(chk & 0xFF);
}
