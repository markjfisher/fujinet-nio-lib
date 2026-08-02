; BBC long-URL implementation of fn_open_long().
;
; Default cc65 fastcall entry:
;   A        = flags
;   c_sp+0   = url
;   c_sp+2   = method
;   c_sp+3   = handle
;
; Flags are pushed on entry so they survive validation and strlen(). The
; resulting argument offsets below include that one-byte saved value.

        .macpack longbranch

        .export _fn_open_long

        .import __fn_initialized
        .import _strlen
        .import _fn_open
        .import _fn_bbc_open_flags
        .import _fn_bbc_arm_open_url
        .import _osfind
        .import _fn_bbc_claim_channel
        .import pusha, pushwysp, addysp
        .importzp c_sp

FN_OK                         = $00
FN_ERR_INVALID                = $02
FN_ERR_IO                     = $05
FN_ERR_UNSUPPORTED            = $08
FN_ERR_URL_TOO_LONG           = $11

FN_BBC_DIRECT_URL_MAX_PLUS_1  = $80
FN_MAX_URL_LEN_HI             = $02       ; 512 = $0200

ARG_FLAGS                     = 0
ARG_URL                       = 1
ARG_METHOD                    = 3
ARG_HANDLE                    = 4
ARG_BYTES                     = 6

        .bss
work_byte:     .res 1                   ; open mode, then returned channel
url_len:       .res 2

        .rodata
long_sentinel:
        .byte   "://", $0D, $00

        .code

_fn_open_long:
        ; Preserve the fastcall flags byte on the C stack.
        jsr     pusha

        lda     __fn_initialized
        jeq     invalid

        ; handle and URL must both be non-null.
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

        ; Translate and validate the method before inspecting the URL, matching
        ; the error precedence of the C implementation.
        ldy     #ARG_METHOD
        lda     (c_sp),y
        jsr     _fn_bbc_open_flags
        cpx     #$80
        jcs     unsupported
        sta     work_byte

        ; strlen(url)
        ldy     #ARG_URL+1
        lda     (c_sp),y
        tax
        dey
        lda     (c_sp),y
        jsr     _strlen

        ; FN_MAX_URL_LEN is 512 on BBC: $0200 is valid, anything above is not.
        cpx     #FN_MAX_URL_LEN_HI
        bcc     length_ok
        bne     too_long
        cmp     #0
        bne     too_long
length_ok:

        ; The ordinary open path handles URLs through 127 bytes.
        cpx     #0
        bne     long_open
        cmp     #FN_BBC_DIRECT_URL_MAX_PLUS_1
        bcs     long_open

        ; fn_open(handle, method, url, flags)
        ldy     #ARG_HANDLE+3
        jsr     pushwysp
        ldy     #ARG_METHOD+2
        lda     (c_sp),y
        jsr     pusha
        ldy     #ARG_URL+6
        jsr     pushwysp
        ldy     #ARG_FLAGS+5
        lda     (c_sp),y
        jsr     _fn_open
        jmp     result

long_open:
        sta     url_len
        stx     url_len+1

        ; fn_bbc_arm_open_url(url, url_len)
        ldy     #ARG_URL+3
        jsr     pushwysp
        lda     url_len
        ldx     url_len+1
        jsr     _fn_bbc_arm_open_url
        cmp     #FN_OK
        bne     invalid

        ; osfind(mode, "://\r") consumes the armed URL in fn-rom.
        lda     work_byte
        jsr     pusha
        lda     #<long_sentinel
        ldx     #>long_sentinel
        jsr     _osfind
        cmp     #0                      ; OSFIND's return flags do not describe A
        beq     io_error
        sta     work_byte

        ; fn_bbc_claim_channel(handle, channel)
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
