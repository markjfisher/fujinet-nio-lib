#ifndef FN_APPSTORE_INTERNAL_H
#define FN_APPSTORE_INTERNAL_H

#include "fn_internal.h"

enum {
    FN_FILEPROTO_VERSION = 1,
    FN_APPSTORE_PREFIX_MAX = 1 + 2 + 255 + 2 + 255
};

uint8_t *fn_appstore_request_buffer(void);
uint8_t *fn_appstore_response_buffer(void);
uint16_t fn_appstore_request_capacity(void);
uint16_t fn_appstore_response_capacity(void);
uint8_t fn_appstore_call(uint8_t command, uint16_t request_len, uint16_t *response_len);
uint8_t fn_appstore_build_prefix(uint16_t *off,
                                 const char *namespace_name,
                                 const char *key,
                                 uint8_t key_required);

#endif
