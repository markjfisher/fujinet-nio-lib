;=============================================================================
; fn_transport.s - Atari SIO Transport Implementation
; 
; Implements the platform transport interface for Atari 8-bit systems
; using the SIO (Serial I/O) bus.
;
; @version 1.0.0
;=============================================================================

        .export _fn_transport_init
        .export _fn_transport_ready
        .export _fn_transport_exchange
        .export _fn_platform_name
        
        .import _fn_slip_encode
        .import _fn_slip_decode
        
        .importzp ptr1, ptr2, ptr3
        .importzp tmp1, tmp2, tmp3, tmp4
        
        .include "atari.inc"
        .include "fn_protocol.inc"

;=============================================================================
; Constants
;=============================================================================

; SIO timeout in frames (approximately 1/60 second per frame)
SIO_TIMEOUT      = 60 * 5   ; 5 seconds

; Buffer sizes (keep small for 8-bit)
MAX_PACKET       = 512
SLIP_BUFFER_SIZE = 768

;=============================================================================
; Data Section
;=============================================================================

.data

_platform_name:
        .byte "atari", 0

;=============================================================================
; BSS Section
;=============================================================================

.bss

; SLIP encoding/decoding buffers
slip_buffer:     .res SLIP_BUFFER_SIZE
resp_buffer:     .res SLIP_BUFFER_SIZE

;=============================================================================
; Code Section
;=============================================================================

.code

;-----------------------------------------------------------------------------
; fn_transport_init - Initialize the transport layer
; 
; uint8_t fn_transport_init(void)
;
; Returns: FN_OK (0) on success, error code on failure
;-----------------------------------------------------------------------------
_fn_transport_init:
        ; For now, just return success
        ; TODO: Check for FujiNet device presence
        lda #$00           ; FN_OK
        rts

;-----------------------------------------------------------------------------
; fn_transport_ready - Check if transport is ready
; 
; uint8_t fn_transport_ready(void)
;
; Returns: 1 if ready, 0 if not
;-----------------------------------------------------------------------------
_fn_transport_ready:
        ; Check if SIO is idle
        lda $0480          ; Check serial bus status
        and #$80           ; Check for activity
        bne @not_ready
        
        lda #$01
        rts
        
@not_ready:
        lda #$00
        rts

;-----------------------------------------------------------------------------
; fn_transport_exchange - Send request and receive response
; 
; uint8_t fn_transport_exchange(
;     const uint8_t *request,   ; sreg + sp
;     uint16_t req_len,         ; stack
;     uint8_t *response,        ; stack
;     uint16_t resp_max,        ; stack
;     uint16_t *resp_len        ; stack
; )
;
; Returns: FN_OK on success, error code on failure
;-----------------------------------------------------------------------------
_fn_transport_exchange:
        ; Save parameters from stack
        ; Stack layout after jsr:
        ;   return address (2 bytes)
        ;   request ptr (2 bytes)
        ;   req_len (2 bytes)
        ;   response ptr (2 bytes)
        ;   resp_max (2 bytes)
        ;   resp_len ptr (2 bytes)
        
        ; Get request pointer (at stack+2)
        ldy #2
        lda (c_sp),y
        sta ptr1
        iny
        lda (c_sp),y
        sta ptr1+1
        
        ; Get req_len (at stack+4)
        ldy #4
        lda (c_sp),y
        sta tmp1
        iny
        lda (c_sp),y
        sta tmp2
        
        ; Get response pointer (at stack+6)
        ldy #6
        lda (c_sp),y
        sta ptr2
        iny
        lda (c_sp),y
        sta ptr2+1
        
        ; Get resp_len pointer (at stack+10)
        ldy #10
        lda (c_sp),y
        sta ptr3
        iny
        lda (c_sp),y
        sta ptr3+1
        
        ; SLIP encode the request
        ; fn_slip_encode(request, req_len, slip_buffer)
        ; Parameters: A:X = output buffer, stack = req_len, stack = input
        lda ptr1
        ldx ptr1+1
        jsr pushax         ; request
        lda tmp1
        ldx tmp2
        jsr pushax         ; req_len
        lda #<slip_buffer
        ldx #>slip_buffer
        jsr pushax         ; output buffer
        jsr _fn_slip_encode
        ; Result in A:X is encoded length
        sta tmp1           ; save encoded length low
        stx tmp2           ; save encoded length high
        
        ; Send via SIO
        ; Set up DCB for write
        lda #FN_DEVICE_NETWORK
        sta ddevic
        lda #$01
        sta dunit
        lda #'W'           ; Write command
        sta dcomnd
        lda #$80           ; Write operation
        sta dstats
        lda #<slip_buffer
        sta dbuflo
        lda #>slip_buffer
        sta dbufhi
        lda tmp1
        sta dbytlo
        lda tmp2
        sta dbythi
        lda #<SIO_TIMEOUT
        sta dtimlo
        lda #0
        sta daux1
        sta daux2
        
        ; Call SIO
        jsr siov
        
        ; Check write result
        lda dstats
        cmp #$01
        bne @write_error
        
        ; Now read the response
        ; Set up DCB for read
        lda #FN_DEVICE_NETWORK
        sta ddevic
        lda #$01
        sta dunit
        lda #'R'           ; Read command
        sta dcomnd
        lda #$40           ; Read operation
        sta dstats
        lda #<resp_buffer
        sta dbuflo
        lda #>resp_buffer
        sta dbufhi
        lda #<SLIP_BUFFER_SIZE
        sta dbytlo
        lda #>SLIP_BUFFER_SIZE
        sta dbythi
        lda #<SIO_TIMEOUT
        sta dtimlo
        lda #0
        sta daux1
        sta daux2
        
        ; Call SIO
        jsr siov
        
        ; Check read result
        lda dstats
        cmp #$01
        bne @read_error
        
        ; Get actual bytes read
        lda dbytlo
        sta tmp1
        lda dbythi
        sta tmp2
        
        ; SLIP decode the response
        ; fn_slip_decode(resp_buffer, resp_len, response)
        lda #<resp_buffer
        ldx #>resp_buffer
        jsr pushax
        lda tmp1
        ldx tmp2
        jsr pushax
        lda ptr2
        ldx ptr2+1
        jsr pushax
        jsr _fn_slip_decode
        ; Result in A:X is decoded length
        
        ; Store response length
        ldy #0
        sta (ptr3),y
        iny
        txa
        sta (ptr3),y
        
        ; Return success
        lda #$00           ; FN_OK
        rts
        
@write_error:
        lda #$08           ; FN_ERR_TRANSPORT
        rts
        
@read_error:
        lda #$08           ; FN_ERR_TRANSPORT
        rts

;-----------------------------------------------------------------------------
; fn_platform_name - Get platform name string
; 
; const char *fn_platform_name(void)
;
; Returns: Pointer to platform name string
;-----------------------------------------------------------------------------
_fn_platform_name:
        lda #<_platform_name
        ldx #>_platform_name
        rts

;=============================================================================
; End of file
;=============================================================================
