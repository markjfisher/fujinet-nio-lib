; Shared OSFIND name workspace for synchronous BBC network opens.

        .export _fn_bbc_open_name

FN_BBC_DIRECT_URL_MAX_PLUS_1 = 128

        .bss
_fn_bbc_open_name:
        .res    FN_BBC_DIRECT_URL_MAX_PLUS_1
