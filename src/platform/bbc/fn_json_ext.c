#include <string.h>

#include "fn_bbc_internal.h"

uint8_t fn_json_query(fn_handle_t handle, const char *path)
{
    uint8_t block[16];
    uint16_t len;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (path == 0) {
        return FN_ERR_INVALID;
    }

    len = (uint16_t)strlen(path);
    if (len > FN_BBC_OSWORD_STR_MAX) {
        return FN_ERR_URL_TOO_LONG;
    }

    if (fn_find_session(handle) < 0) {
        return FN_ERR_NOT_FOUND;
    }

    memset(block, 0, sizeof(block));
    block[0] = FN_BBC_REASON_JSON_QUERY;
    block[2] = (uint8_t)((unsigned)path & 0xFFu);
    block[3] = (uint8_t)(((unsigned)path >> 8) & 0xFFu);
    block[4] = (uint8_t)(len & 0xFFu);
    block[5] = (uint8_t)(len >> 8);
    block[6] = (uint8_t)handle;

    return fn_bbc_status_to_result(fn_bbc_osword78(block));
}
