; BBC short-URL implementation of fn_open().
;
; Default cc65 fastcall entry:
;   A        = flags
;   c_sp+0   = url
;   c_sp+2   = method
;   c_sp+3   = handle
;
; Flags are pushed on entry, so the offsets below include that saved byte.

        .macpack longbranch

        .export _fn_open

        .import __fn_initialized
        .import _strlen
        .import _fn_bbc_open_flags
        .import _fn_bbc_arm_open_flags
        .import _fn_bbc_prepare_short_open_name
        .import _osfind
        .import _fn_bbc_claim_channel
        .import pusha, pushwysp, addysp
        .importzp c_sp

FN_OK                         = $00
FN_ERR_INVALID                = $02
FN_ERR_IO                     = $05
FN_ERR_UNSUPPORTED            = $08
FN_ERR_URL_TOO_LONG           = $11

FN_OPEN_STREAM_NO_PROBE       = $10
FN_OPEN_FLAG_STREAM_NO_PROBE  = $10
FN_BBC_DIRECT_URL_MAX_PLUS_1  = $80

ARG_FLAGS                     = 0
ARG_URL                       = 1
ARG_METHOD                    = 3
ARG_HANDLE                    = 4
ARG_BYTES                     = 6

        .bss
work_byte:     .res 1                   ; open mode, then returned channel
url_len:       .res 2

        .code

_fn_open:
        jsr     pusha

        lda     __fn_initialized
        jeq     invalid

        ldy     #ARG_HANDLE
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     invalid
        ldy     #ARG_URL
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     invalid

        ; Preserve the BBC OSFIND mode across strlen and optional OSWORD calls.
        ldy     #ARG_METHOD
        lda     (c_sp),y
        jsr     _fn_bbc_open_flags
        cpx     #$80
        jcs     unsupported
        sta     work_byte

        ldy     #ARG_URL+1
        lda     (c_sp),y
        tax
        dey
        lda     (c_sp),y
        jsr     _strlen
        sta     url_len
        stx     url_len+1

        ; The direct OSFIND path accepts at most 127 bytes. This also subsumes
        ; the wider FN_MAX_URL_LEN check because both failures return $11.
        cpx     #0
        bne     too_long
        cmp     #FN_BBC_DIRECT_URL_MAX_PLUS_1
        bcs     too_long

        ldy     #ARG_FLAGS
        lda     (c_sp),y
        and     #FN_OPEN_STREAM_NO_PROBE
        beq     flags_ready
        lda     #FN_OPEN_FLAG_STREAM_NO_PROBE
        jsr     _fn_bbc_arm_open_flags
        cmp     #FN_OK
        bne     result
flags_ready:

        ; fn_bbc_prepare_short_open_name(url, url_len)
        ldy     #ARG_URL+3
        jsr     pushwysp
        lda     url_len
        ldx     url_len+1
        jsr     _fn_bbc_prepare_short_open_name

        ; url_len <= 127 guarantees the preparation helper returns its static
        ; buffer. Preserve that pointer on the hardware stack while pushing the
        ; OSFIND mode onto the cc65 C stack.
        pha
        txa
        pha
        lda     work_byte
        jsr     pusha
        pla
        tax
        pla
        jsr     _osfind
        cmp     #0                      ; OSFIND's return flags do not describe A
        beq     io_error
        sta     work_byte

        ldy     #ARG_HANDLE+3
        jsr     pushwysp
        lda     work_byte
        jsr     _fn_bbc_claim_channel
        jmp     result

unsupported:
        lda     #FN_ERR_UNSUPPORTED
        bne     result
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
