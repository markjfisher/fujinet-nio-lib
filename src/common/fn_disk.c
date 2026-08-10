#include <string.h>

#include "fn_protocol.h"
#include "fn_raw.h"
#include "fujinet-nio.h"

enum {
    FN_DISK_CMD_MOUNT = 0x01,
    FN_DISK_CMD_UNMOUNT = 0x02,
    FN_DISK_CMD_READ_SECTOR = 0x03,
    FN_DISK_CMD_WRITE_SECTOR = 0x04,
    FN_DISK_CMD_INFO = 0x05,
    FN_DISK_CMD_CLEAR_CHANGED = 0x06
};

enum {
    /* The fixed Info fields end after sectorCount. */
    FN_DISK_INFO_RESPONSE_SIZE = 12,
    /* Older peers omit lastError; newer peers may append it. */
    FN_DISK_INFO_RESPONSE_SIZE_WITH_LAST_ERROR = 13
};

/* DiskDevice calls are synchronous in the client library. Keep the large
 * codec buffers out of 6502 stack frames; cc65 has a deliberately small local
 * variable budget and the raw transport already serializes requests. */
static uint8_t disk_request[FN_MAX_PACKET_SIZE];
static uint8_t disk_reply[FN_MAX_PACKET_SIZE];

static uint8_t disk_status(uint8_t status)
{
    switch (status) {
        case 0: return FN_OK;
        case 1: return FN_ERR_NOT_FOUND;
        case 2: return FN_ERR_INVALID;
        case 3: return FN_ERR_BUSY;
        case 4: return FN_ERR_NOT_READY;
        case 5: return FN_ERR_IO;
        case 6: return FN_ERR_TIMEOUT;
        case 7: return FN_ERR_INTERNAL;
        case 8: return FN_ERR_UNSUPPORTED;
        default: return FN_ERR_UNKNOWN;
    }
}

static uint16_t get_u16le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t get_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void put_u16le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_u32le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint8_t parse_info(const uint8_t *reply, uint16_t length,
                          fn_disk_info_t *info)
{
    /*
     * The original DiskDevice Info response ended at sectorCount (12 bytes),
     * while newer responses may append the optional lastError byte (13 bytes).
     * Accepting both preserves compatibility with existing BBC/MS-DOS clients
     * and older NIO peers; this is not arbitrary size tolerance.
     */
    if (!info || (length != FN_DISK_INFO_RESPONSE_SIZE &&
                  length != FN_DISK_INFO_RESPONSE_SIZE_WITH_LAST_ERROR) ||
        reply[0] != FN_DISK_PROTOCOL_VERSION) {
        return FN_ERR_IO;
    }
    info->flags = reply[1];
    info->slot = reply[4];
    info->type = reply[5];
    info->sector_size = get_u16le(reply + 6);
    info->sector_count = get_u32le(reply + 8);
    info->last_error = (length == FN_DISK_INFO_RESPONSE_SIZE_WITH_LAST_ERROR)
                           ? reply[12]
                           : 0;
    return FN_OK;
}

static uint8_t disk_call(uint8_t command, const void *request,
                         uint16_t request_length, void *reply,
                         uint16_t reply_capacity, uint16_t *reply_length)
{
    fn_raw_response_t response;
    uint8_t result = fn_raw_call(FN_DEVICE_DISK, command,
                                 request, request_length, reply,
                                 reply_capacity, &response);
    if (result != FN_OK) return result;
    if (reply_length) *reply_length = response.payload_length;
    return disk_status(response.status);
}

uint8_t fn_disk_mount(uint8_t slot, const char *uri, uint8_t readonly,
                      uint8_t type, uint16_t sector_size_hint,
                      fn_disk_info_t *info)
{
    uint16_t uri_length;
    uint16_t request_length;
    uint16_t reply_length = 0;
    uint8_t result;

    if (!uri) return FN_ERR_INVALID;
#if !defined(__CC65__)
    if (strlen(uri) > 0xFFFFu) return FN_ERR_INVALID;
#endif
    uri_length = (uint16_t)strlen(uri);
    if ((uint32_t)8 + uri_length > FN_MAX_PACKET_SIZE - FN_HEADER_SIZE) {
        return FN_ERR_INVALID;
    }
    disk_request[0] = FN_DISK_PROTOCOL_VERSION;
    disk_request[1] = slot;
    disk_request[2] = readonly ? 1 : 0;
    disk_request[3] = type;
    put_u16le(disk_request + 4, sector_size_hint);
    put_u16le(disk_request + 6, uri_length);
    memcpy(disk_request + 8, uri, uri_length);
    request_length = (uint16_t)(8 + uri_length);

    result = disk_call(FN_DISK_CMD_MOUNT, disk_request, request_length,
                       disk_reply, sizeof(disk_reply), &reply_length);
    if (result != FN_OK) return result;
    if (reply_length != 12 || disk_reply[0] != FN_DISK_PROTOCOL_VERSION) {
        return FN_ERR_IO;
    }
    if (info) {
        info->flags = disk_reply[1];
        info->slot = disk_reply[4];
        info->type = disk_reply[5];
        info->sector_size = get_u16le(disk_reply + 6);
        info->sector_count = get_u32le(disk_reply + 8);
        info->last_error = 0;
    }
    return FN_OK;
}

uint8_t fn_disk_unmount(uint8_t slot)
{
    uint16_t reply_length = 0;
    uint8_t result;
    disk_request[0] = FN_DISK_PROTOCOL_VERSION;
    disk_request[1] = slot;
    result = disk_call(FN_DISK_CMD_UNMOUNT, disk_request, 2,
                       disk_reply, sizeof(disk_reply), &reply_length);
    if (result != FN_OK) return result;
    if (reply_length != 5 || disk_reply[0] != FN_DISK_PROTOCOL_VERSION ||
        disk_reply[4] != slot) return FN_ERR_IO;
    return FN_OK;
}

uint8_t fn_disk_info(uint8_t slot, fn_disk_info_t *info)
{
    uint16_t reply_length = 0;
    uint8_t result;

    if (!info) return FN_ERR_INVALID;
    disk_request[0] = FN_DISK_PROTOCOL_VERSION;
    disk_request[1] = slot;
    result = disk_call(FN_DISK_CMD_INFO, disk_request, 2,
                       disk_reply, sizeof(disk_reply), &reply_length);
    if (result != FN_OK) return result;
    return parse_info(disk_reply, reply_length, info);
}

uint8_t fn_disk_clear_changed(uint8_t slot)
{
    uint16_t reply_length = 0;
    uint8_t result;
    disk_request[0] = FN_DISK_PROTOCOL_VERSION;
    disk_request[1] = slot;
    result = disk_call(FN_DISK_CMD_CLEAR_CHANGED, disk_request, 2,
                       disk_reply, sizeof(disk_reply), &reply_length);
    if (result != FN_OK) return result;
    if (reply_length != 5 || disk_reply[0] != FN_DISK_PROTOCOL_VERSION ||
        disk_reply[4] != slot) return FN_ERR_IO;
    return FN_OK;
}

uint8_t fn_disk_read_sector(uint8_t slot, uint32_t lba,
                            uint8_t *data, uint16_t data_capacity,
                            uint16_t *data_length)
{
    uint16_t reply_length = 0;
    uint16_t payload_length;
    uint8_t result;

    if (!data || !data_capacity || !data_length) return FN_ERR_INVALID;
    disk_request[0] = FN_DISK_PROTOCOL_VERSION;
    disk_request[1] = slot;
    put_u32le(disk_request + 2, lba);
    put_u16le(disk_request + 6, data_capacity);
    result = disk_call(FN_DISK_CMD_READ_SECTOR, disk_request, 8,
                       disk_reply, sizeof(disk_reply), &reply_length);
    if (result != FN_OK) return result;
    if (reply_length < 11 || disk_reply[0] != FN_DISK_PROTOCOL_VERSION ||
        disk_reply[4] != slot || get_u32le(disk_reply + 5) != lba) return FN_ERR_IO;
    payload_length = get_u16le(disk_reply + 9);
    if ((uint32_t)11 + payload_length != reply_length ||
        payload_length > data_capacity) return FN_ERR_IO;
    memcpy(data, disk_reply + 11, payload_length);
    *data_length = payload_length;
    return FN_OK;
}

uint8_t fn_disk_write_sector(uint8_t slot, uint32_t lba,
                             const uint8_t *data, uint16_t data_length)
{
    uint16_t reply_length = 0;
    uint8_t result;

    if (!data || !data_length || (uint32_t)8 + data_length > FN_MAX_PACKET_SIZE - FN_HEADER_SIZE) {
        return FN_ERR_INVALID;
    }
    disk_request[0] = FN_DISK_PROTOCOL_VERSION;
    disk_request[1] = slot;
    put_u32le(disk_request + 2, lba);
    put_u16le(disk_request + 6, data_length);
    memcpy(disk_request + 8, data, data_length);
    result = disk_call(FN_DISK_CMD_WRITE_SECTOR, disk_request,
                       (uint16_t)(8 + data_length), disk_reply,
                       sizeof(disk_reply),
                       &reply_length);
    if (result != FN_OK) return result;
    if (reply_length != 11 || disk_reply[0] != FN_DISK_PROTOCOL_VERSION ||
        disk_reply[4] != slot || get_u32le(disk_reply + 5) != lba ||
        get_u16le(disk_reply + 9) != data_length) return FN_ERR_IO;
    return FN_OK;
}
