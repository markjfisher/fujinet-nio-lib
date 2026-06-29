#include <i86.h>
#include <stddef.h>
#include <string.h>

#include "fn_internal.h"
#include "fn_msdos.h"
#include "fn_platform.h"
#include "fn_protocol.h"

static uint8_t cached_drive;
static uint16_t last_dos_error;
static uint8_t last_detail;
static uint8_t last_nio_status;
static uint8_t last_device;
static uint8_t last_command;
static uint16_t last_response_len;
static uint8_t last_diag_error;
static uint8_t last_diag_status;
static uint16_t last_diag_rx_len;
static uint16_t last_diag_expected_len;
static uint8_t last_diag_lsr;
static fn_msdos_ioctl_query_t query_buf;
static fn_msdos_ioctl_nio_call_t call_buf;

static int valid_sig(const char *sig)
{
    return memcmp(sig, FN_MSDOS_IOCTL_SIGNATURE, 4) == 0;
}

static uint8_t calc_checksum_skip_offset(const uint8_t *data,
                                         uint16_t len,
                                         uint16_t skip_offset)
{
    uint16_t chk;
    uint16_t i;

    chk = 0;
    for (i = 0; i < len; ++i) {
        if (i == skip_offset) {
            continue;
        }
        chk += data[i];
        chk = ((chk >> 8) + (chk & 0xFF)) & 0xFFFF;
    }

    return (uint8_t)(chk & 0xFF);
}

static int ioctl_call(uint8_t drive, uint8_t subfunction, void *buffer, uint16_t size)
{
    union REGS regs;
    struct SREGS sregs;

    regs.h.ah = 0x44;
    regs.h.al = subfunction;
    regs.h.bl = drive;
    regs.w.cx = size;
    regs.x.dx = FP_OFF(buffer);
    sregs.ds = FP_SEG(buffer);

    int86x(0x21, &regs, &regs, &sregs);
    if (regs.x.cflag & INTR_CF) {
        last_dos_error = regs.x.ax;
        return 0;
    }

    last_dos_error = 0;
    return 1;
}

static int ioctl_recv(uint8_t drive, void *buffer, uint16_t size)
{
    return ioctl_call(drive, 0x04, buffer, size);
}

void fn_msdos_ioctl_set_drive(uint8_t drive)
{
    cached_drive = drive;
}

uint16_t fn_msdos_ioctl_last_error(void)
{
    return last_dos_error;
}

uint8_t fn_msdos_ioctl_last_detail(void)
{
    return last_detail;
}

uint8_t fn_msdos_ioctl_last_nio_status(void)
{
    return last_nio_status;
}

uint8_t fn_msdos_ioctl_last_device(void)
{
    return last_device;
}

uint8_t fn_msdos_ioctl_last_command(void)
{
    return last_command;
}

uint16_t fn_msdos_ioctl_last_response_len(void)
{
    return last_response_len;
}

uint8_t fn_msdos_ioctl_last_diag_error(void)
{
    return last_diag_error;
}

uint8_t fn_msdos_ioctl_last_diag_status(void)
{
    return last_diag_status;
}

uint16_t fn_msdos_ioctl_last_diag_rx_len(void)
{
    return last_diag_rx_len;
}

uint16_t fn_msdos_ioctl_last_diag_expected_len(void)
{
    return last_diag_expected_len;
}

uint8_t fn_msdos_ioctl_last_diag_lsr(void)
{
    return last_diag_lsr;
}

uint8_t fn_msdos_ioctl_find_drive(void)
{
    uint8_t drive;

    if (cached_drive != 0) {
        return cached_drive;
    }

    for (drive = 3; drive <= 26; ++drive) {
        memset(&query_buf, 0, sizeof(query_buf));
        query_buf.command = FN_MSDOS_IOCTL_QUERY;
        if (ioctl_recv(drive, &query_buf, sizeof(query_buf)) &&
            valid_sig(query_buf.signature)) {
            cached_drive = drive;
            return drive;
        }
    }

    return 0;
}

static uint8_t build_response_packet(uint8_t device,
                                     uint8_t command,
                                     uint8_t status,
                                     uint16_t payload_len)
{
    uint16_t total_len;
    uint16_t offset;

    if ((uint16_t)(1 + payload_len) > (uint16_t)(FN_MAX_PACKET_SIZE - FN_HEADER_SIZE)) {
        return FN_ERR_IO;
    }

    total_len = (uint16_t)(FN_HEADER_SIZE + 1 + payload_len);
    offset = fn_build_header(_fn_transport_ctx.response, device, command, total_len);
    _fn_transport_ctx.response[5] = 1; /* one A1 status byte */
    _fn_transport_ctx.response[offset++] = status;

    if (payload_len != 0) {
        memcpy(_fn_transport_ctx.response + offset, call_buf.data, payload_len);
        offset = (uint16_t)(offset + payload_len);
    }

    _fn_transport_ctx.response[4] = fn_calc_checksum(_fn_transport_ctx.response, offset);
    _fn_transport_ctx.resp_len = offset;
    return FN_OK;
}

uint8_t fn_transport_init(void)
{
    if (fn_msdos_ioctl_find_drive() == 0) {
        return FN_ERR_NOT_FOUND;
    }
    return FN_OK;
}

uint8_t fn_transport_ready(void)
{
    return fn_msdos_ioctl_find_drive() != 0;
}

uint8_t fn_transport_exchange(void)
{
    const uint8_t *request;
    uint16_t req_len;
    uint16_t packet_len;
    uint16_t payload_len;
    uint8_t drive;
    uint8_t checksum;

    request = _fn_transport_ctx.request;
    req_len = _fn_transport_ctx.req_len;

    if (request == 0 || req_len < FN_HEADER_SIZE ||
        _fn_transport_ctx.response == 0 || _fn_transport_ctx.resp_max < FN_HEADER_SIZE) {
        return FN_ERR_INVALID;
    }

    packet_len = FN_READ_LE16(request, 2);
    if (packet_len != req_len || packet_len < FN_HEADER_SIZE) {
        return FN_ERR_INVALID;
    }

    checksum = request[4];
    if (calc_checksum_skip_offset(request, req_len, 4) != checksum) {
        return FN_ERR_INVALID;
    }

    payload_len = (uint16_t)(req_len - FN_HEADER_SIZE);
    if (payload_len > FN_MSDOS_IOCTL_MAX_DATA) {
        return FN_ERR_INVALID;
    }

    last_detail = FN_MSDOS_IOCTL_DETAIL_NONE;
    last_nio_status = 0;
    last_device = request[0];
    last_command = request[1];
    last_response_len = 0;
    last_diag_error = 0;
    last_diag_status = 0;
    last_diag_rx_len = 0;
    last_diag_expected_len = 0;
    last_diag_lsr = 0;

    drive = fn_msdos_ioctl_find_drive();
    if (drive == 0) {
        return FN_ERR_NOT_FOUND;
    }

    memset(&call_buf, 0, sizeof(call_buf));
    call_buf.command = FN_MSDOS_IOCTL_NIO_CALL;
    call_buf.device = request[0];
    call_buf.nio_command = request[1];
    call_buf.request_len = payload_len;
    call_buf.response_len = FN_MSDOS_IOCTL_MAX_DATA;
    if (payload_len != 0) {
        memcpy(call_buf.data, request + FN_HEADER_SIZE, payload_len);
    }

    if (!ioctl_recv(drive, &call_buf, sizeof(call_buf))) {
        last_detail = FN_MSDOS_IOCTL_DETAIL_DOS_ERROR;
        return FN_ERR_IO;
    }

    if (!valid_sig(call_buf.signature)) {
        last_detail = FN_MSDOS_IOCTL_DETAIL_BAD_SIGNATURE;
        return FN_ERR_IO;
    }

    last_nio_status = call_buf.nio_status;
    last_response_len = call_buf.response_len;
    last_diag_error = call_buf.diag_error;
    last_diag_status = call_buf.diag_status;
    last_diag_rx_len = call_buf.diag_rx_len;
    last_diag_expected_len = call_buf.diag_expected_len;
    last_diag_lsr = call_buf.diag_lsr;

    if (call_buf.response_len > FN_MSDOS_IOCTL_MAX_DATA) {
        last_detail = FN_MSDOS_IOCTL_DETAIL_BAD_RESPONSE_LEN;
        return FN_ERR_IO;
    }

    if (call_buf.nio_status != FN_OK) {
        last_detail = FN_MSDOS_IOCTL_DETAIL_NIO_STATUS;
    }

    return build_response_packet(request[0],
                                 request[1],
                                 call_buf.nio_status,
                                 call_buf.response_len);
}

void fn_transport_close(void)
{
}

const char *fn_platform_name(void)
{
    return "msdos-ioctl";
}
