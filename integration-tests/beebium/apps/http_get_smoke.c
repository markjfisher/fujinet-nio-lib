#include <stdio.h>
#include <stdint.h>

#include "fujinet-nio.h"

#define SMOKE_URL "http://example.com/data.txt"

int main(void)
{
    uint8_t result;
    fn_handle_t handle;
    uint8_t buf[4];
    uint16_t bytes_read;
    uint8_t flags;

    result = fn_init();
    if (result != FN_OK) {
        printf("INIT %u\n", result);
        return 1;
    }

    result = fn_open(&handle, FN_METHOD_GET, SMOKE_URL, 0);
    if (result != FN_OK) {
        printf("OPEN %u\n", result);
        return 1;
    }

    result = fn_read(handle, 0, buf, 2, &bytes_read, &flags);
    if (result != FN_OK) {
        printf("READ %u\n", result);
        fn_close(handle);
        return 1;
    }

    if (bytes_read != 2) {
        printf("LEN %u\n", bytes_read);
        fn_close(handle);
        return 1;
    }

    result = fn_close(handle);
    if (result != FN_OK) {
        printf("CLOSE %u\n", result);
        return 1;
    }

    putchar(buf[0]);
    putchar(buf[1]);
    printf(" OK\n");
    return 0;
}
