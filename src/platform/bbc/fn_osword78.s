        .export _fn_bbc_osword78

        .import OSWORD
        .importzp ptr1

        .include "oslib/os.inc"

; uint8_t __fastcall__ fn_bbc_osword78(uint8_t *block)
;   AX = pointer to 16-byte parameter block
.proc _fn_bbc_osword78
        sta     ptr1
        stx     ptr1+1

        tax                     ; put low pointer into X
        ldy     ptr1+1          ; and high into y
        lda     #$78
        jsr     OSWORD

        ldy     #$01
        lda     (ptr1),y
        ldx     #$00
        rts
.endproc
