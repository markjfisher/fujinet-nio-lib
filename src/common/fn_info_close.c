#include "fn_internal.h"
#include "fn_platform.h"

uint8_t fn_info(fn_handle_t handle,
                uint16_t *http_status,
                uint32_t *content_length,
                uint8_t *flags)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    int8_t slot;
    fn_handle_t resp_handle;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == FN_INVALID_HANDLE) {
        return FN_ERR_INVALID;
    }

    slot = fn_find_session(handle);
    if (slot < 0) {
        return FN_ERR_NOT_FOUND;
    }

    req_len = fn_build_info_packet(_fn_req_buf, handle);
    if (req_len == 0) {
        return FN_ERR_INVALID;
    }

    result = fn_transport_exchange(_fn_req_buf, req_len, _fn_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }

    return fn_parse_info_response(_fn_resp_buf, resp_len, &resp_handle, http_status, content_length, flags);
}

uint8_t fn_close(fn_handle_t handle)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == FN_INVALID_HANDLE) {
        return FN_ERR_INVALID;
    }

    req_len = fn_build_close_packet(_fn_req_buf, handle);
    if (req_len == 0) {
        return FN_ERR_INVALID;
    }

    result = fn_transport_exchange(_fn_req_buf, req_len, _fn_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);

    fn_free_handle(handle);

    return result;
}
