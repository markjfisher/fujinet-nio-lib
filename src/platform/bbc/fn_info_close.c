#include "fn_bbc_internal.h"

uint8_t fn_info(fn_handle_t handle,
                uint16_t *http_status,
                uint32_t *content_length,
                uint8_t *flags)
{
    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (fn_find_session(handle) < 0) {
        return FN_ERR_NOT_FOUND;
    }

    if (http_status != 0) {
        *http_status = 0;
    }
    if (content_length != 0) {
        *content_length = 0;
    }
    if (flags != 0) {
        *flags = FN_INFO_CONNECTED;
    }

    return FN_OK;
}

uint8_t fn_close(fn_handle_t handle)
{
    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == FN_INVALID_HANDLE) {
        return FN_ERR_INVALID;
    }

    if (fn_find_session(handle) < 0) {
        return FN_ERR_NOT_FOUND;
    }

    if (close_file((uint8_t)handle) != 0) {
        return FN_ERR_IO;
    }

    fn_free_handle(handle);
    return FN_OK;
}
