        .export _fn_bbc_osbget

        .import OSBGET

        .include "oslib/os.inc"
        .include "fn_protocol.inc"

; int __fastcall__ fn_bbc_osbget(unsigned char channel)
; returns byte 0..255 or -1 on EOF
_fn_bbc_osbget:
        tay
        jsr     OSBGET
        bcs     @eof
        ldx     #$00
        rts

@eof:   cmp     #FN_BBC_OSBGET_NOT_READY
        beq     @not_ready
        lda     #$FF
        tax
        rts

@not_ready:
        tax
        rts
