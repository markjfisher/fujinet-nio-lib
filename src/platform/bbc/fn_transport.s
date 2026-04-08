;=============================================================================
; fn_transport.s - BBC RS423 streaming transport
;
; Sends FujiBus packets as SLIP streams directly over RS423 and decodes
; incoming SLIP data directly into the caller's response buffer.
;=============================================================================

        .export _fn_transport_init
        .export _fn_transport_ready
        .export _fn_transport_exchange
        .export _fn_platform_name

        .import __fn_transport_ctx
        .importzp c_sp, ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, tmp4

        .include "oslib/os.inc"
        .include "oslib/osbyte.inc"
        .include "fn_protocol.inc"

WAIT_FIRST_MAX := $1E00
WAIT_NEXT_MAX  := $0800

OSBYTE_SERIAL_RX_RATE       = $07
OSBYTE_SERIAL_TX_RATE       = $08
OSBYTE_INPUT_STREAM         = $02
OSBYTE_OUTPUT_STREAM        = $03
OSBYTE_FLUSH_BUFFER         = $15
OSBYTE_BUFFER_STATUS        = $80
OSBYTE_BUFFER_REMOVE        = $91

BAUD_19200                  = $08
INPUT_SERIAL                = $01
INPUT_KEYBOARD              = $00
OUTPUT_SERIAL               = $03
OUTPUT_SCREEN               = $00
BUFFER_SERIAL_INPUT         = $01

.data
_platform_name:
        .byte "bbc", 0

.bss
escape_flag:
        .res 1

.code

setup_serial_19200:
        ldx     #BAUD_19200
        ldy     #$00
        lda     #OSBYTE_SERIAL_RX_RATE
        jsr     OSBYTE

        ldx     #BAUD_19200
        ldy     #$00
        lda     #OSBYTE_SERIAL_TX_RATE
        jsr     OSBYTE

        ldx     #INPUT_SERIAL
        ldy     #$00
        lda     #OSBYTE_INPUT_STREAM
        jsr     OSBYTE

        ldx     #OUTPUT_SERIAL
        ldy     #$00
        lda     #OSBYTE_OUTPUT_STREAM
        jsr     OSBYTE

flush_serial:
        ldx     #BUFFER_SERIAL_INPUT
        ldy     #$00
        lda     #OSBYTE_FLUSH_BUFFER
        jmp     OSBYTE

restore_output_to_screen:
        ldx     #OUTPUT_SCREEN
        ldy     #$00
        lda     #OSBYTE_OUTPUT_STREAM
        jsr     OSBYTE

        ldx     #INPUT_KEYBOARD
        ldy     #$00
        lda     #OSBYTE_INPUT_STREAM
        jmp     OSBYTE

check_rs423_buffer:
        lda     #OSBYTE_BUFFER_STATUS
        ldx     #$FE
        ldy     #$FF
        jsr     OSBYTE
        txa
        rts

read_rs423_char:
        lda     #OSBYTE_BUFFER_REMOVE
        ldx     #$01
        ldy     #$00
        jsr     OSBYTE
        bcs     @no_char
        tya
        clc
        rts
@no_char:
        sec
        rts

write_slip_byte:
        cmp     #SLIP_END
        beq     @write_esc_end
        cmp     #SLIP_ESCAPE
        beq     @write_esc_esc
        jmp     OSWRCH

@write_esc_end:
        pha
        lda     #SLIP_ESCAPE
        jsr     OSWRCH
        lda     #SLIP_ESC_END
        jsr     OSWRCH
        pla
        rts

@write_esc_esc:
        pha
        lda     #SLIP_ESCAPE
        jsr     OSWRCH
        lda     #SLIP_ESC_ESC
        jsr     OSWRCH
        pla
        rts

_fn_transport_init:
        lda     #FN_OK
        rts

_fn_transport_ready:
        lda     #$01
        rts

_fn_transport_exchange:
        ; request pointer from _fn_transport_ctx + 0
        ldy     #$00
        lda     __fn_transport_ctx,y
        sta     ptr1
        iny
        lda     __fn_transport_ctx,y
        sta     ptr1+1

        ; request length from _fn_transport_ctx + 4
        ldy     #$04
        lda     __fn_transport_ctx,y
        sta     tmp1
        iny
        lda     __fn_transport_ctx,y
        sta     tmp2

        ; response pointer/current pointer from _fn_transport_ctx + 2
        ldy     #$02
        lda     __fn_transport_ctx,y
        sta     ptr2
        sta     ptr4
        iny
        lda     __fn_transport_ctx,y
        sta     ptr2+1
        sta     ptr4+1

        ; resp_max remaining capacity from _fn_transport_ctx + 6
        ldy     #$06
        lda     __fn_transport_ctx,y
        sta     tmp3
        iny
        lda     __fn_transport_ctx,y
        sta     tmp4

        jsr     setup_serial_19200

        lda     #SLIP_END
        jsr     OSWRCH

@send_loop:
        lda     tmp1
        ora     tmp2
        beq     @send_done

        ldy     #$00
        lda     (ptr1),y
        jsr     write_slip_byte

        inc     ptr1
        bne     :+
        inc     ptr1+1
:
        lda     tmp1
        bne     :+
        dec     tmp2
:
        dec     tmp1
        jmp     @send_loop

@send_done:
        lda     #SLIP_END
        jsr     OSWRCH

        lda     #<WAIT_FIRST_MAX
        sta     tmp1
        lda     #>WAIT_FIRST_MAX
        sta     tmp2

@wait_start:
        jsr     check_rs423_buffer
        bne     @have_start_char
        lda     tmp1
        bne     :+
        dec     tmp2
:
        dec     tmp1
        lda     tmp1
        ora     tmp2
        bne     @wait_start
        jmp     @error_timeout

@have_start_char:
        jsr     read_rs423_char
        bcc     :+
        jmp     @read_error
: 
        cmp     #SLIP_END
        bne     @wait_start

        lda     #$00
        sta     escape_flag

@frame_loop:
        lda     #<WAIT_NEXT_MAX
        sta     tmp1
        lda     #>WAIT_NEXT_MAX
        sta     tmp2

@wait_char:
        jsr     check_rs423_buffer
        bne     @read_char
        lda     tmp1
        bne     :+
        dec     tmp2
:
        dec     tmp1
        lda     tmp1
        ora     tmp2
        bne     @wait_char
        jmp     @error_timeout

@read_char:
        jsr     read_rs423_char
        bcc     :+
        jmp     @read_error
: 
        cmp     #SLIP_END
        beq     @handle_end

        ldx     escape_flag
        bne     @escaped_byte

        cmp     #SLIP_ESCAPE
        beq     @set_escape

@store_byte:
        pha
        lda     tmp3
        ora     tmp4
        beq     @overflow

        lda     tmp3
        bne     :+
        dec     tmp4
:
        dec     tmp3

        pla
        ldy     #$00
        sta     (ptr2),y
        inc     ptr2
        bne     :+
        inc     ptr2+1
:
        jmp     @frame_loop

@overflow:
        pla
        jmp     @error_transport

@set_escape:
        lda     #$01
        sta     escape_flag
        jmp     @frame_loop

@escaped_byte:
        lda     #$00
        sta     escape_flag
        cmp     #SLIP_ESC_END
        beq     :+
        cmp     #SLIP_ESC_ESC
        beq     :++
        jmp     @error_transport
:
        lda     #SLIP_END
        jmp     @store_byte
:
        lda     #SLIP_ESCAPE
        jmp     @store_byte

@handle_end:
        lda     ptr2
        cmp     ptr4
        bne     @done
        lda     ptr2+1
        cmp     ptr4+1
        bne     @done
        jmp     @frame_loop

@done:
        jsr     restore_output_to_screen
        lda     escape_flag
        bne     @error_transport_raw

        lda     ptr2
        sec
        sbc     ptr4
        ldy     #$08
        sta     __fn_transport_ctx,y
        lda     ptr2+1
        sbc     ptr4+1
        iny
        sta     __fn_transport_ctx,y
        lda     #FN_OK
        rts

@error_timeout:
        jsr     restore_output_to_screen
        lda     #FN_ERR_TIMEOUT
        rts

@read_error:
        jmp     @error_transport

@error_transport:
        jsr     restore_output_to_screen
@error_transport_raw:
        lda     #FN_ERR_TRANSPORT
        rts

_fn_platform_name:
        lda     #<_platform_name
        ldx     #>_platform_name
        rts
