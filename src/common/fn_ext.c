#include "fujinet-nio.h"

#ifndef __BBC__
uint8_t fn_open_long(fn_handle_t *handle,
                     uint8_t method,
                     const char *url,
                     uint8_t flags)
{
    return fn_open(handle, method, url, flags);
}

uint8_t fn_set_body_length(uint16_t len)
{
    (void)len;
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_set_content_profile(uint8_t profile)
{
    (void)profile;
    return FN_ERR_UNSUPPORTED;
}

uint8_t fn_json_query(fn_handle_t handle, const char *path)
{
    (void)handle;
    (void)path;
    return FN_ERR_UNSUPPORTED;
}
#endif
