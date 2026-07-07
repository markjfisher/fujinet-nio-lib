#ifndef FN_LEGACY_APPKEY_INTERNAL_H
#define FN_LEGACY_APPKEY_INTERNAL_H

#include <stdint.h>

#include "fn_legacy_appkey.h"

#define FN_LEGACY_APPKEY_DEFAULT_SIZE 64
#define FN_LEGACY_APPKEY_SIZE_256 256
#define FN_LEGACY_APPKEY_URI_MAX 32
#define FN_LEGACY_APPKEY_TRANSFER_MAX (1 + 2 + FN_LEGACY_APPKEY_URI_MAX + 4 + 2 + FN_LEGACY_APPKEY_SIZE_256)
#define FN_LEGACY_APPKEY_READ_RESPONSE_HEADER 10

extern uint16_t _fn_legacy_appkey_creator_id;
extern uint8_t _fn_legacy_appkey_app_id;
extern uint8_t _fn_legacy_appkey_size;
extern uint8_t _fn_legacy_appkey_transfer[FN_LEGACY_APPKEY_TRANSFER_MAX];

uint16_t _fn_legacy_appkey_capacity(void);
uint16_t _fn_legacy_appkey_build_uri(uint8_t key_id, char *uri, uint16_t uri_capacity);
uint16_t _fn_legacy_file_uri_prefix(uint8_t *buf, const char *uri, uint16_t uri_len);
uint8_t _fn_legacy_file_mkdir_fuji(void);

#endif /* FN_LEGACY_APPKEY_INTERNAL_H */
