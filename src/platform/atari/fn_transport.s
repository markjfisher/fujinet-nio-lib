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

; Buffer sizes (keep small for 8-bit). The SIO transfer buffers are fixed
; in BSS so the linker places them inside the loaded program, above MEMLO.
MAX_PACKET       = 512
SLIP_BUFFER_SIZE = 768
SIO_READ_CHUNK_SIZE = 128

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
slip_end_count: .res 1
sio_tx_buffer:   .res SLIP_BUFFER_SIZE
sio_rx_buffer:   .res SLIP_BUFFER_SIZE

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
        
        ; SLIP encode the request
        ; fn_slip_encode(request, req_len, sio_tx_buffer)
        ; cc65 fastcall: rightmost argument in A:X, earlier arguments pushed
        ; left-to-right. For this call, push request and req_len, then pass
        ; output in A:X.
        lda ptr1
        ldx ptr1+1
        jsr pushax         ; request
        lda tmp1
        ldx tmp2
        jsr pushax         ; req_len
        lda #<sio_tx_buffer
        ldx #>sio_tx_buffer
        jsr _fn_slip_encode
        ; Result in A:X is encoded length
        sta tmp1           ; save encoded length low
        stx tmp2           ; save encoded length high
        
        ; Send via SIO
        ; Set up DCB for write
        lda #FN_SIO_DEVICE_FUJIBUS
        sta ddevic
        lda #$01
        sta dunit
        lda #'W'           ; Write command
        sta dcomnd
        lda #$80           ; Write operation
        sta dstats
        lda #<sio_tx_buffer
        sta dbuflo
        lda #>sio_tx_buffer
        sta dbufhi
        lda tmp1
        sta dbytlo
        lda tmp2
        sta dbythi
        lda #<SIO_TIMEOUT
        sta dtimlo
        lda #$00
        sta dunuse
        lda tmp1
        sta daux1
        lda tmp2
        sta daux2
        
        ; Call SIO
        jsr siov
        lda dstats
        sta last_sio_status
        
        ; Check write result
        lda dstats
        cmp #$01
        beq @write_ok
        jmp @write_error
@write_ok:
        
        ; Now read the response. SIO reads are fixed length, but FujiBus
        ; responses are variable-length SLIP frames. Read in small SIO chunks
        ; until the second SLIP END marker is seen.
        lda #$00
        sta tmp3           ; total raw response length low
        sta tmp4           ; total raw response length high
        sta slip_end_count

@read_loop:
        ; Stop if the receive buffer is full before a complete SLIP frame.
        lda tmp3
        cmp #<SLIP_BUFFER_SIZE
        lda tmp4
        sbc #>SLIP_BUFFER_SIZE
        bcc @read_space_available
        jmp @read_error
@read_space_available:

        ; Set up DCB for read
        lda #FN_SIO_DEVICE_FUJIBUS
        sta ddevic
        lda #$01
        sta dunit
        lda #'R'           ; Read command
        sta dcomnd
        lda #$40           ; Read operation
        sta dstats
        clc
        lda #<sio_rx_buffer
        adc tmp3
        sta dbuflo
        lda #>sio_rx_buffer
        adc tmp4
        sta dbufhi
        lda #<SIO_READ_CHUNK_SIZE
        sta dbytlo
        lda #>SIO_READ_CHUNK_SIZE
        sta dbythi
        lda #<SIO_TIMEOUT
        sta dtimlo
        lda #$00
        sta dunuse
        lda #<SIO_READ_CHUNK_SIZE
        sta daux1
        lda #>SIO_READ_CHUNK_SIZE
        sta daux2
        
        ; Call SIO
        jsr siov
        lda dstats
        sta last_sio_status
        
        ; Check read result
        lda dstats
        cmp #$01
        bne @read_error

        ; Scan this chunk for raw SLIP END markers.
        lda dbuflo
        sta ptr3
        lda dbufhi
        sta ptr3+1
        ldy #$00
@scan_chunk:
        cpy #SIO_READ_CHUNK_SIZE
        beq @chunk_done
        lda (ptr3),y
        cmp #SLIP_END
        bne @next_scan_byte
        inc slip_end_count
        lda slip_end_count
        cmp #$02
        bcs @frame_done
@next_scan_byte:
        iny
        bne @scan_chunk

@chunk_done:
        clc
        lda tmp3
        adc #<SIO_READ_CHUNK_SIZE
        sta tmp3
        lda tmp4
        adc #>SIO_READ_CHUNK_SIZE
        sta tmp4
        jmp @read_loop

@frame_done:
        clc
        lda tmp3
        adc #<SIO_READ_CHUNK_SIZE
        sta tmp1
        lda tmp4
        adc #>SIO_READ_CHUNK_SIZE
        sta tmp2
        
        ; SLIP decode the response
        ; fn_slip_decode(sio_rx_buffer, resp_len, response)
        ; cc65 fastcall: rightmost argument in A:X, earlier arguments pushed
        ; left-to-right. For this call, push input and in_len, then pass
        ; output in A:X.
        lda #<sio_rx_buffer
        ldx #>sio_rx_buffer
        jsr pushax         ; input
        lda tmp1
        ldx tmp2
        jsr pushax         ; resp_len
        lda ptr2
        ldx ptr2+1
        jsr _fn_slip_decode
        ; Result in A:X is decoded length
        
        ; Store response length into transport context
        ldy #8
        sta __fn_transport_ctx,y
        iny
        txa
        sta __fn_transport_ctx,y
        
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
