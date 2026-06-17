#include <stdio.h>

#include "fujinet-nio.h"

static fn_handle_t g_handle;
static uint8_t g_buf[256];
static uint16_t g_bytes_read;
static uint8_t g_flags;
static uint8_t g_result;

int main(void)
{
    g_bytes_read = 0;
    g_flags = 0;

    g_result = fn_init();
    if (g_result != FN_OK) {
        printf("[INIT %u]\n", g_result);
        return 1;
    }

    g_result = fn_open(&g_handle, 0, "tcp://example.com:7777", FN_OPEN_STREAM_NO_PROBE);
    if (g_result != FN_OK) {
        printf("[OPEN %u]\n", g_result);
        return 1;
    }

    g_result = fn_read(g_handle, 0, g_buf, sizeof(g_buf), &g_bytes_read, &g_flags);
    if (g_result == FN_OK) {
        printf("[OK %u %u]", g_bytes_read, g_flags);
    } else if (g_result == FN_ERR_NOT_READY) {
        printf("[NR]");
    } else {
        printf("[ER %u]", g_result);
    }

    fn_close(g_handle);
    return 0;
}
