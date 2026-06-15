        .export _fn_bbc_osbget

        .import OSBGET

        .include "oslib/os.inc"

; int __fastcall__ fn_bbc_osbget(unsigned char channel)
; returns byte 0..255 or -1 on EOF
.proc _fn_bbc_osbget
        tay
        jsr     OSBGET
        bcs     @eof
        ldx     #$00
        rts

@eof:   lda     #$FF
        tax
        rts
.endproc
