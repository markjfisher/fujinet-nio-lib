#include <string.h>

#include "fujinet-nio.h"
#include "fn_appstore_internal.h"

uint8_t fn_appstore_list_next_key(const uint8_t *key_data,
                                  uint16_t key_data_len,
                                  uint16_t *offset,
                                  char *key_out,
                                  uint16_t key_out_capacity)
{
    uint16_t off;
    uint16_t key_len;

    if (key_data == 0 || offset == 0 || key_out == 0 || key_out_capacity == 0) {
        return FN_ERR_INVALID;
    }
    off = *offset;
    if (off == key_data_len) {
        return FN_ERR_NOT_READY;
    }
    if ((uint16_t)(off + 2) > key_data_len) {
        return FN_ERR_IO;
    }
    key_len = FN_GET_LE16(&key_data[off]);
    off = (uint16_t)(off + 2);
    if ((uint16_t)(off + key_len) > key_data_len || key_len >= key_out_capacity) {
        return FN_ERR_IO;
    }

    memcpy(key_out, &key_data[off], key_len);
    key_out[key_len] = 0;
    off = (uint16_t)(off + key_len);
    *offset = off;
    return FN_OK;
}
