#include "fn_bbc_internal.h"

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
