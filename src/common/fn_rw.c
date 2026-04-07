#include "fn_internal.h"
#include "fn_platform.h"

uint8_t fn_write(fn_handle_t handle,
                 uint32_t offset,
                 const uint8_t *data,
                 uint16_t len,
                 uint16_t *written)
{
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t result;
    int8_t slot;
    uint8_t status;
    uint16_t data_offset;
    uint16_t data_len;

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

    result = fn_transport_exchange(_fn_req_buf, req_len, _fn_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }

    result = fn_parse_response_header(_fn_resp_buf, resp_len, &status, &data_offset, &data_len);
    if (result != FN_OK) {
        return result;
    }

    if (status != FN_OK) {
        return status;
    }

    if (data_len >= 12 && written != NULL) {
        *written = FN_READ_LE16(_fn_resp_buf, data_offset + 10);
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
    uint16_t resp_len;
    uint8_t result;
    int8_t slot;
    fn_handle_t resp_handle;
    uint32_t offset_echo;

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

    result = fn_transport_exchange(_fn_req_buf, req_len, _fn_resp_buf, FN_MAX_PACKET_SIZE, &resp_len);
    if (result != FN_OK) {
        return result;
    }

    result = fn_parse_read_response(_fn_resp_buf, resp_len, &resp_handle, &offset_echo, flags, buf, max_len, bytes_read);
    if (result != FN_OK) {
        return result;
    }

    if ((_fn_sessions[slot].proto_flags & FN_PROTO_FLAG_SEQUENTIAL_READ) && *bytes_read > 0) {
        _fn_sessions[slot].read_offset += *bytes_read;
    }

    return FN_OK;
}
