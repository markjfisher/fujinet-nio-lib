#include "fn_internal.h"

uint8_t fn_calc_checksum(const uint8_t *data, uint16_t len)
{
    return (uint8_t)fn_checksum_fold(data, len, 0);
}
