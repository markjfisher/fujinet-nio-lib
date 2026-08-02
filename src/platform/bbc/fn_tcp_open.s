; BBC implementation of fn_tcp_open().
;
; Default cc65 fastcall entry:
;   A/X      = port
;   c_sp+0   = host
;   c_sp+2   = handle
;
; The port is saved on the C stack, so the offsets below include two bytes.
; The URL is built in the same 128-byte workspace used by short fn_open calls;
; there is no second URL buffer and no division runtime dependency.

        .macpack longbranch

        .export _fn_tcp_open

        .import __fn_initialized
        .import _fn_bbc_open_name
        .import _osfind
        .import _fn_bbc_claim_channel
        .import pusha, pushax, pushwysp, addysp
        .importzp c_sp, ptr1, ptr2, tmp1, tmp2

FN_ERR_INVALID                = $02
FN_ERR_IO                     = $05
FN_ERR_URL_TOO_LONG           = $11
FN_BBC_OPEN_UPDATE            = $C0

ARG_PORT                      = 0
ARG_HOST                      = 2
ARG_HANDLE                    = 4
ARG_BYTES                     = 6
OPEN_NAME_SIZE                = 128

        .rodata
tcp_prefix:
        .byte   "tcp://"
decimal_lo:
        .byte   <10000, <1000, <100, <10
decimal_hi:
        .byte   >10000, >1000, >100, >10

        .code

_fn_tcp_open:
        ; Preserve the fastcall port and make all arguments stack-addressable.
        jsr     pushax

        lda     __fn_initialized
        jeq     invalid

        ldy     #ARG_HANDLE
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     invalid
        ldy     #ARG_HOST
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     invalid

        ; Copy the fixed prefix.
        ldx     #0
prefix_loop:
        lda     tcp_prefix,x
        sta     _fn_bbc_open_name,x
        inx
        cpx     #6
        bne     prefix_loop

        ; Copy the host while reserving space for ':', one digit, and CR.
        ldy     #ARG_HOST
        lda     (c_sp),y
        sta     ptr1
        iny
        lda     (c_sp),y
        sta     ptr1+1
        ldy     #0
host_loop:
        lda     (ptr1),y
        beq     host_done
        cpx     #OPEN_NAME_SIZE-3
        jcs     too_long
        sta     _fn_bbc_open_name,x
        inx
        iny
        bne     host_loop             ; the length bound prevents Y wrapping

host_done:
        lda     #':'
        sta     _fn_bbc_open_name,x
        inx

        ; Convert the port using bounded repeated subtraction. An open happens
        ; rarely enough that at most 27 subtractions is a good size/speed trade.
        ldy     #ARG_PORT
        lda     (c_sp),y
        sta     ptr2
        iny
        lda     (c_sp),y
        sta     ptr2+1
        lda     #0
        sta     tmp2                    ; non-zero once a digit was emitted
        ldy     #0                      ; divisor table index

decimal_place:
        lda     #0
        sta     tmp1                    ; digit for this decimal place
subtract_loop:
        lda     ptr2+1
        cmp     decimal_hi,y
        bcc     place_done
        bne     subtract
        lda     ptr2
        cmp     decimal_lo,y
        bcc     place_done
subtract:
        lda     ptr2
        sec
        sbc     decimal_lo,y
        sta     ptr2
        lda     ptr2+1
        sbc     decimal_hi,y
        sta     ptr2+1
        inc     tmp1
        bne     subtract_loop

place_done:
        lda     tmp1
        bne     emit_place
        lda     tmp2
        beq     next_place
        lda     tmp1
emit_place:
        cpx     #OPEN_NAME_SIZE-1
        bcs     too_long
        ora     #'0'
        sta     _fn_bbc_open_name,x
        inx
        lda     #1
        sta     tmp2
next_place:
        iny
        cpy     #4
        bne     decimal_place

        ; The remainder is the units digit and is always emitted, including 0.
        cpx     #OPEN_NAME_SIZE-1
        bcs     too_long
        lda     ptr2
        ora     #'0'
        sta     _fn_bbc_open_name,x
        inx
        lda     #$0D
        sta     _fn_bbc_open_name,x

        ; TCP uses OSFIND update mode. The shared buffer is already CR-ended,
        ; so opening it directly avoids copying it through fn_open().
        lda     #FN_BBC_OPEN_UPDATE
        jsr     pusha
        lda     #<_fn_bbc_open_name
        ldx     #>_fn_bbc_open_name
        jsr     _osfind
        cmp     #0                      ; OSFIND flags do not describe A
        beq     io_error
        sta     tmp1

        ; fn_bbc_claim_channel(handle, channel)
        ldy     #ARG_HANDLE+3
        jsr     pushwysp
        lda     tmp1
        jsr     _fn_bbc_claim_channel
        jmp     result

too_long:
        lda     #FN_ERR_URL_TOO_LONG
        bne     result
io_error:
        lda     #FN_ERR_IO
        bne     result
invalid:
        lda     #FN_ERR_INVALID
result:
        ldx     #0
        ldy     #ARG_BYTES
        jmp     addysp
