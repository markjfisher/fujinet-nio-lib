#include <stdio.h>

#include "fujinet-nio.h"

static const uint16_t ports[] = { 0, 10, 100, 1000, 10000, 65535U };
static const char max_host[] =
    "0123456789012345678901234567890123456789"
    "0123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678";
static const char too_long_host[] =
    "0123456789012345678901234567890123456789"
    "0123456789012345678901234567890123456789"
    "0123456789012345678901234567890123456789";
static fn_handle_t handle;
static uint8_t result;
static uint8_t i;

int main(void)
{
    result = fn_init();
    if (result != FN_OK) {
        printf("[INIT %u]", result);
        return 1;
    }

    for (i = 0; i < sizeof(ports) / sizeof(ports[0]); ++i) {
        result = fn_tcp_open(&handle, "example.com", ports[i]);
        if (result != FN_OK) {
            printf("[OPEN %u %u]", i, result);
            return 1;
        }
        result = fn_close(handle);
        if (result != FN_OK) {
            printf("[CLOSE %u %u]", i, result);
            return 1;
        }
    }

    result = fn_tcp_open(&handle, max_host, 0);
    if (result != FN_OK) {
        printf("[MAX %u]", result);
        return 1;
    }
    result = fn_close(handle);
    if (result != FN_OK) {
        printf("[MAX CLOSE %u]", result);
        return 1;
    }

    result = fn_tcp_open(&handle, too_long_host, 0);
    if (result != FN_ERR_URL_TOO_LONG) {
        printf("[LONG %u]", result);
        return 1;
    }

    printf("[OK]");
    return 0;
}
