#include <stdio.h>
#include <stdint.h>

#include "fujinet-nio.h"

#define SHORT_URL "http://example.com/short.txt"
#define URL_CHUNK "0123456789abcdef"
#define LONG_URL "http://example.com/" \
                 URL_CHUNK URL_CHUNK URL_CHUNK URL_CHUNK \
                 URL_CHUNK URL_CHUNK URL_CHUNK URL_CHUNK

static uint8_t read_url(const char *url)
{
    uint8_t result;
    fn_handle_t handle;
    uint8_t buf[2];
    uint16_t bytes_read;
    uint8_t flags;

    result = fn_open_long(&handle, FN_METHOD_GET, url, 0);
    if (result != FN_OK) {
        return result;
    }

    result = fn_read(handle, 0, buf, sizeof(buf), &bytes_read, &flags);
    if (result != FN_OK || bytes_read != sizeof(buf) ||
        buf[0] != 'O' || buf[1] != 'K') {
        printf("READ %u %u\n", result, bytes_read);
        fn_close(handle);
        return FN_ERR_IO;
    }

    result = fn_close(handle);
    return result;
}

int main(void)
{
    uint8_t result;
    fn_handle_t handle;

    result = fn_init();
    if (result != FN_OK) {
        printf("INIT %u\n", result);
        return 1;
    }

    result = fn_open(&handle, FN_METHOD_GET, LONG_URL, 0);
    if (result != FN_ERR_URL_TOO_LONG) {
        printf("BOUND %u\n", result);
        return 1;
    }

    result = read_url(SHORT_URL);
    if (result != FN_OK) {
        printf("SHORT %u\n", result);
        return 1;
    }

    result = read_url(LONG_URL);
    if (result != FN_OK) {
        printf("LONG %u\n", result);
        return 1;
    }

    printf("LONG OK\n");
    return 0;
}
