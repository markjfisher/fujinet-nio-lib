;=============================================================================
; fn_protocol.inc - Protocol Constants for Assembly
;=============================================================================

SLIP_END         = $C0
SLIP_ESCAPE      = $DB
SLIP_ESC_END     = $DC
SLIP_ESC_ESC     = $DD

FN_DEVICE_NETWORK    = $FD

FN_OK               = $00
FN_ERR_NOT_FOUND    = $01
FN_ERR_INVALID      = $02
FN_ERR_BUSY         = $03
FN_ERR_NOT_READY    = $04
FN_ERR_IO           = $05
FN_ERR_TIMEOUT      = $06
FN_ERR_INTERNAL     = $07
FN_ERR_UNSUPPORTED  = $08
FN_ERR_TRANSPORT    = $10
FN_ERR_URL_TOO_LONG = $11
FN_ERR_NO_HANDLES   = $12

FN_MAX_SESSIONS     = 3

FN_PROTO_FLAG_SEQUENTIAL_READ  = $01
FN_PROTO_FLAG_SEQUENTIAL_WRITE = $02
FN_PROTO_FLAG_SEQUENTIAL_BOTH  = FN_PROTO_FLAG_SEQUENTIAL_READ | FN_PROTO_FLAG_SEQUENTIAL_WRITE

FN_SESSION_ACTIVE              = 0
FN_SESSION_PROTO_FLAGS         = 1
FN_SESSION_NEEDS_BODY          = 2
FN_SESSION_RESERVED            = 3
FN_SESSION_HANDLE              = 4
FN_SESSION_WRITE_OFFSET        = 6
FN_SESSION_READ_OFFSET         = 10
FN_SESSION_SIZE                = 14

FN_READ_EOF                    = $01

FN_BBC_REASON_JSON_QUERY       = $00
FN_BBC_REASON_SET_BODY_LEN     = $01
FN_BBC_REASON_WRITE_DATA       = $02
FN_BBC_REASON_CONTENT_TYPE     = $03
FN_BBC_REASON_SET_OPEN_URL     = $04
FN_BBC_REASON_SET_OPEN_FLAGS   = $05
FN_BBC_OSBGET_NOT_READY        = $FE
