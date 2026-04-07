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
