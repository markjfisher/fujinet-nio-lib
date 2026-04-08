#include "fn_internal.h"
#include "fn_platform.h"

uint8_t fn_write(fn_handle_t handle,
                 uint32_t offset,
                 const uint8_t *data,
                 uint16_t len,
                 uint16_t *written)
{
    uint16_t req_len;
    uint8_t result;
    int8_t slot;

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

    if (offset != _fn_sessions[slot].write_offset) {
        return FN_ERR_INVALID;
    }

    req_len = fn_build_write_packet(_fn_req_buf, handle, offset, data, len);
    if (req_len == 0) {
        return FN_ERR_INVALID;
    }

    _fn_transport_ctx.request = _fn_req_buf;
    _fn_transport_ctx.req_len = req_len;
    _fn_transport_ctx.response = _fn_resp_buf;
    _fn_transport_ctx.resp_max = FN_MAX_PACKET_SIZE;

    result = fn_transport_exchange();
    if (result != FN_OK) {
        return result;
    }

    _fn_parse_ctx.response = _fn_resp_buf;
    _fn_parse_ctx.resp_len = _fn_transport_ctx.resp_len;
    result = fn_parse_response_header();
    if (result != FN_OK) {
        return result;
    }

    if (_fn_parse_ctx.status != FN_OK) {
        return _fn_parse_ctx.status;
    }

    if (_fn_parse_ctx.data_len >= 12 && written != NULL) {
        *written = FN_READ_LE16(_fn_resp_buf, _fn_parse_ctx.data_offset + 10);
        _fn_sessions[slot].write_offset += *written;
    } else if (written != NULL) {
        *written = 0;
    }

    return FN_OK;
}

uint8_t fn_read(fn_handle_t handle,
                uint32_t offset,
                uint8_t *buf,
                uint16_t max_len,
                uint16_t *bytes_read,
                uint8_t *flags)
{
    uint16_t req_len;
    uint8_t result;
    int8_t slot;

    if (!_fn_initialized) {
        return FN_ERR_INVALID;
    }

    if (handle == FN_INVALID_HANDLE || buf == NULL || bytes_read == NULL) {
        return FN_ERR_INVALID;
    }

    slot = fn_find_session(handle);
    if (slot < 0) {
        return FN_ERR_NOT_FOUND;
    }

    if ((_fn_sessions[slot].proto_flags & FN_PROTO_FLAG_SEQUENTIAL_READ) &&
        offset != _fn_sessions[slot].read_offset) {
        return FN_ERR_INVALID;
    }

    req_len = fn_build_read_packet(_fn_req_buf, handle, offset, max_len);
    if (req_len == 0) {
        return FN_ERR_INVALID;
    }

    _fn_transport_ctx.request = _fn_req_buf;
    _fn_transport_ctx.req_len = req_len;
    _fn_transport_ctx.response = _fn_resp_buf;
    _fn_transport_ctx.resp_max = FN_MAX_PACKET_SIZE;

    result = fn_transport_exchange();
    if (result != FN_OK) {
        return result;
    }

    _fn_parse_ctx.response = _fn_resp_buf;
    _fn_parse_ctx.resp_len = _fn_transport_ctx.resp_len;
    _fn_parse_ctx.data = buf;
    _fn_parse_ctx.data_max = max_len;
    result = fn_parse_read_response();
    if (result != FN_OK) {
        return result;
    }

    if (flags != NULL) {
        *flags = _fn_parse_ctx.flags;
    }
    *bytes_read = _fn_parse_ctx.data_len;

    if ((_fn_sessions[slot].proto_flags & FN_PROTO_FLAG_SEQUENTIAL_READ) && _fn_parse_ctx.data_len > 0) {
        _fn_sessions[slot].read_offset += _fn_parse_ctx.data_len;
    }

    return FN_OK;
}
