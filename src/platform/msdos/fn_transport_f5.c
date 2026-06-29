#include "fujinet-nio.h"
#include "fn_platform.h"

uint8_t fn_transport_init(void)
{
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_transport_ready(void)
{
    return 0;
}

uint8_t fn_transport_exchange(void)
{
    return FN_ERR_UNSUPPORTED;
}

void fn_transport_close(void)
{
}

const char *fn_platform_name(void)
{
    return "msdos-f5";
}
