#include "fn_legacy_appkey_internal.h"

void fuji_set_appkey_details(uint16_t creator_id, uint8_t app_id, enum AppKeySize keysize)
{
    _fn_legacy_appkey_creator_id = creator_id;
    _fn_legacy_appkey_app_id = app_id;
    _fn_legacy_appkey_size = (uint8_t)keysize;
}
