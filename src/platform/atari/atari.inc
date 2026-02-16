;=============================================================================
; atari.inc - Atari Hardware Definitions for CC65
; 
; Standard definitions for Atari SIO programming
;=============================================================================

;-----------------------------------------------------------------------------
; SIO Device Control Block (DCB)
; Located at $0300
;-----------------------------------------------------------------------------
ddevic  = $0300      ; Device ID
dunit   = $0301      ; Unit number
dcomnd  = $0302      ; Command
dstats  = $0303      ; Status
dbuflo  = $0304      ; Buffer low
dbufhi  = $0305      ; Buffer high
dtimlo  = $0306      ; Timeout
dbytlo  = $0308      ; Byte count low
dbythi  = $0309      ; Byte count high
daux1   = $030A      ; Aux byte 1
daux2   = $030B      ; Aux byte 2

;-----------------------------------------------------------------------------
; SIO Vector
;-----------------------------------------------------------------------------
siov    = $E459      ; SIO entry point

;-----------------------------------------------------------------------------
; CC65 Runtime Imports
;-----------------------------------------------------------------------------
.import pushax
.importzp c_sp
