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
        .export _fn_atari_last_sio_status
        
        .import __fn_transport_ctx
        
        .importzp ptr1, ptr2
        .importzp tmp1, tmp2, tmp3, tmp4
        
        .include "atari.inc"
        .include "fn_protocol.inc"

;=============================================================================
; Constants
;=============================================================================

; SIO timeout in frames (approximately 1/60 second per frame)
SIO_TIMEOUT      = 60 * 5   ; 5 seconds

; SIO transfers are chunked so the transport does not need whole encoded
; request/response staging buffers. FujiBus packets are SLIP framed directly
; to/from the public request and response buffers.
SIO_CHUNK_SIZE   = 128

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

last_sio_status: .res 1
chunk_len:       .res 1
transport_error: .res 1
slip_started:    .res 1
slip_escaped:    .res 1
sio_chunk_buffer: .res SIO_CHUNK_SIZE

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
        ; request pointer from transport context
        ldy #0
        lda __fn_transport_ctx,y
        sta ptr1
        iny
        lda __fn_transport_ctx,y
        sta ptr1+1
        
        ; request length from transport context
        ldy #4
        lda __fn_transport_ctx,y
        sta tmp1
        iny
        lda __fn_transport_ctx,y
        sta tmp2
        
        ; response pointer from transport context
        ldy #2
        lda __fn_transport_ctx,y
        sta ptr2
        iny
        lda __fn_transport_ctx,y
        sta ptr2+1
        
        lda #$00
        sta chunk_len
        sta transport_error

        ; Stream SLIP encoded request bytes through small SIO write chunks.
        lda #SLIP_END
        jsr @tx_append_byte
        lda transport_error
        beq @tx_start_ok
        jmp @write_error
@tx_start_ok:

@tx_loop:
        lda tmp1
        ora tmp2
        beq @tx_frame_done

        ldy #$00
        lda (ptr1),y
        jsr @tx_emit_slip_byte
        lda transport_error
        beq @tx_byte_ok
        jmp @write_error
@tx_byte_ok:

        inc ptr1
        bne @tx_dec_remaining
        inc ptr1+1
@tx_dec_remaining:
        lda tmp1
        bne @tx_dec_low
        dec tmp2
@tx_dec_low:
        dec tmp1
        jmp @tx_loop

@tx_frame_done:
        lda #SLIP_END
        jsr @tx_append_byte
        lda transport_error
        beq @tx_end_ok
        jmp @write_error
@tx_end_ok:
        jsr @tx_flush_chunk
        lda transport_error
        beq @tx_flush_ok_to_read
        jmp @write_error
@tx_flush_ok_to_read:

        ; response max from transport context
        ldy #6
        lda __fn_transport_ctx,y
        sta tmp3
        iny
        lda __fn_transport_ctx,y
        sta tmp4

        ; Decode the response SLIP frame directly into the caller response
        ; buffer as SIO read chunks arrive.
        lda #$00
        sta tmp1           ; decoded response length low
        sta tmp2           ; decoded response length high
        sta slip_started
        sta slip_escaped

@read_loop:
        ; Set up DCB for read
        lda #FN_SIO_DEVICE_FUJIBUS
        sta ddevic
        lda #$01
        sta dunit
        lda #'R'           ; Read command
        sta dcomnd
        lda #$40           ; Read operation
        sta dstats
        lda #<sio_chunk_buffer
        sta dbuflo
        lda #>sio_chunk_buffer
        sta dbufhi
        lda #<SIO_CHUNK_SIZE
        sta dbytlo
        lda #>SIO_CHUNK_SIZE
        sta dbythi
        lda #<SIO_TIMEOUT
        sta dtimlo
        lda #$00
        sta dunuse
        lda #<SIO_CHUNK_SIZE
        sta daux1
        lda #>SIO_CHUNK_SIZE
        sta daux2
        
        ; Call SIO
        jsr siov
        lda dstats
        sta last_sio_status
        
        ; Check read result
        lda dstats
        cmp #$01
        beq @read_ok
        jmp @read_error
@read_ok:

        ldx #$00
@rx_decode_chunk:
        cpx #SIO_CHUNK_SIZE
        beq @read_loop

        lda sio_chunk_buffer,x
        cmp #SLIP_END
        beq @rx_slip_end

        ldy slip_started
        beq @rx_next_byte

        ldy slip_escaped
        bne @rx_escaped_byte

        cmp #SLIP_ESCAPE
        beq @rx_escape
        jmp @rx_store_byte

@rx_escaped_byte:
        ldy #$00
        sty slip_escaped
        cmp #SLIP_ESC_END
        beq @rx_store_slip_end
        cmp #SLIP_ESC_ESC
        beq @rx_store_slip_escape
        jmp @read_error

@rx_store_slip_end:
        lda #SLIP_END
        jmp @rx_store_byte

@rx_store_slip_escape:
        lda #SLIP_ESCAPE
        jmp @rx_store_byte

@rx_escape:
        lda #$01
        sta slip_escaped
        jmp @rx_next_byte

@rx_slip_end:
        lda slip_started
        bne @rx_after_started_end
        lda #$01
        sta slip_started
        jmp @rx_next_byte

@rx_after_started_end:
        lda tmp1
        ora tmp2
        bne @frame_done
        jmp @rx_next_byte

@rx_store_byte:
        pha
        lda tmp2
        cmp tmp4
        bcc @rx_has_space
        bne @rx_no_space
        lda tmp1
        cmp tmp3
        bcc @rx_has_space
@rx_no_space:
        pla
        jmp @read_error

@rx_has_space:
        pla
        ldy #$00
        sta (ptr2),y
        inc ptr2
        bne @rx_inc_len
        inc ptr2+1
@rx_inc_len:
        inc tmp1
        bne @rx_next_byte
        inc tmp2

@rx_next_byte:
        inx
        jmp @rx_decode_chunk

@frame_done:
        ; Store response length into transport context
        ldy #8
        lda tmp1
        sta __fn_transport_ctx,y
        iny
        lda tmp2
        sta __fn_transport_ctx,y
        
        ; Return success
        lda #$00           ; FN_OK
        rts
        
@write_error:
        lda #FN_ERR_TRANSPORT
        rts
        
@read_error:
        lda #FN_ERR_TRANSPORT
        rts

@tx_emit_slip_byte:
        cmp #SLIP_END
        beq @tx_emit_escaped_end
        cmp #SLIP_ESCAPE
        beq @tx_emit_escaped_escape
        jmp @tx_append_byte

@tx_emit_escaped_end:
        lda #SLIP_ESCAPE
        jsr @tx_append_byte
        lda transport_error
        bne @tx_emit_done
        lda #SLIP_ESC_END
        jmp @tx_append_byte

@tx_emit_escaped_escape:
        lda #SLIP_ESCAPE
        jsr @tx_append_byte
        lda transport_error
        bne @tx_emit_done
        lda #SLIP_ESC_ESC
        jmp @tx_append_byte

@tx_emit_done:
        rts

@tx_append_byte:
        pha
        lda chunk_len
        cmp #SIO_CHUNK_SIZE
        bcc @tx_append_has_space
        jsr @tx_flush_chunk
        lda transport_error
        beq @tx_append_has_space
        pla
        rts

@tx_append_has_space:
        pla
        ldx chunk_len
        sta sio_chunk_buffer,x
        inc chunk_len
        rts

@tx_flush_chunk:
        lda chunk_len
        beq @tx_flush_done

        lda #FN_SIO_DEVICE_FUJIBUS
        sta ddevic
        lda #$01
        sta dunit
        lda #'W'           ; Write command
        sta dcomnd
        lda #$80           ; Write operation
        sta dstats
        lda #<sio_chunk_buffer
        sta dbuflo
        lda #>sio_chunk_buffer
        sta dbufhi
        lda chunk_len
        sta dbytlo
        lda #$00
        sta dbythi
        lda #<SIO_TIMEOUT
        sta dtimlo
        lda #$00
        sta dunuse
        lda chunk_len
        sta daux1
        lda #$00
        sta daux2

        jsr siov
        lda dstats
        sta last_sio_status
        cmp #$01
        beq @tx_flush_ok
        lda #FN_ERR_TRANSPORT
        sta transport_error
        rts

@tx_flush_ok:
        lda #$00
        sta chunk_len
@tx_flush_done:
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

;-----------------------------------------------------------------------------
; fn_atari_last_sio_status - Get last SIO DSTATS value.
;
; uint8_t fn_atari_last_sio_status(void)
;-----------------------------------------------------------------------------
_fn_atari_last_sio_status:
        lda last_sio_status
        rts

;=============================================================================
; End of file
;=============================================================================
