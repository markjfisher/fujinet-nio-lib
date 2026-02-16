;=============================================================================
; fn_protocol.inc - Protocol Constants for Assembly
; 
; Assembly-language definitions for FujiBus protocol
;=============================================================================

; SLIP Protocol Constants
SLIP_END         = $C0
SLIP_ESCAPE      = $DB
SLIP_ESC_END     = $DC
SLIP_ESC_ESC     = $DD

; Wire Device IDs
FN_DEVICE_FUJI       = $70
FN_DEVICE_NETWORK    = $FD
FN_DEVICE_DISK       = $FC
FN_DEVICE_FILE       = $FE

; Network Commands
FN_CMD_OPEN      = $01
FN_CMD_READ      = $02
FN_CMD_WRITE     = $03
FN_CMD_CLOSE     = $04
FN_CMD_INFO      = $05

; Protocol Version
FN_PROTOCOL_VERSION = $01

; Open Flags
FN_OPEN_FLAG_TLS           = $01
FN_OPEN_FLAG_FOLLOW_REDIR  = $02
FN_OPEN_FLAG_BODY_UNKNOWN  = $04
FN_OPEN_FLAG_ALLOW_EVICT   = $08

; Error Codes
FN_OK               = $00
FN_ERR_INVALID      = $01
FN_ERR_BUSY         = $02
FN_ERR_NOT_READY    = $03
FN_ERR_IO           = $04
FN_ERR_NO_MEMORY    = $05
FN_ERR_NOT_FOUND    = $06
FN_ERR_TIMEOUT      = $07
FN_ERR_TRANSPORT    = $08
FN_ERR_URL_TOO_LONG = $09
FN_ERR_NO_HANDLES   = $0A
FN_ERR_UNKNOWN      = $FF

; HTTP Methods
FN_METHOD_GET     = $01
FN_METHOD_POST    = $02
FN_METHOD_PUT     = $03
FN_METHOD_DELETE  = $04
FN_METHOD_HEAD    = $05

; Read Flags
FN_READ_EOF       = $01
FN_READ_TRUNCATED = $02

; Info Flags
FN_INFO_HAS_STATUS  = $04
FN_INFO_HAS_LENGTH  = $02
FN_INFO_CONNECTED   = $10
FN_INFO_PEER_CLOSED = $20

; Configuration
FN_MAX_URL_LEN    = 256
FN_MAX_SESSIONS   = 4
FN_MAX_PACKET     = 1024
