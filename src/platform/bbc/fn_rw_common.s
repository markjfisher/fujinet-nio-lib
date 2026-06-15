        .export _fn_bbc_rw_setup_read
        .export _fn_bbc_rw_setup_write
        .export invalid_with_stack_fix

        .import _fn_find_session
        .import addysp
        .importzp c_sp, ptr3, tmp3, tmp4

        .include "fn_protocol.inc"

STACK_READ_HANDLE                 := 10
STACK_WRITE_HANDLE                := 8

.rodata
session_offsets:
        .repeat FN_MAX_SESSIONS, I
        .byte   I * FN_SESSION_SIZE
        .endrepeat

.code
_fn_bbc_rw_setup_read:
        ldy     #STACK_READ_HANDLE + 1
        bne     rw_common

_fn_bbc_rw_setup_write:
        ldy     #STACK_WRITE_HANDLE + 1

rw_common:
        pha                         ; save fastcall ptr low
        txa
        pha                         ; save fastcall ptr high

        ; handle
        lda     (c_sp),y
        tax
        dey
        lda     (c_sp),y
        sta     tmp3                ; handle low byte
        txa
        ora     tmp3
        bne     @is_valid
        ldx     #FN_ERR_INVALID
        bne     @do_error

@is_valid:
        lda     tmp3
        jsr     _fn_find_session
        cpx     #$FF
        bne     @found
        ldx     #FN_ERR_NOT_FOUND
        bne     @do_error

@found:
        tay
        lda     session_offsets,y
        sta     tmp4                ; session offset

        pla
        sta     ptr3+1
        pla
        sta     ptr3
        lda     tmp4
        ldx     #$00
        clc
        rts

; sets A to error code, and sets C = 1
; still have 2 params on the h/w stack to remove
@do_error:
        pla
        pla
        txa             ; transfer error code into A
        sec
        rts

; enter with Y the size of bytes to fix
invalid_with_stack_fix:
        pla
        pla
        jsr     addysp
        ldx     #$00
        lda     #FN_ERR_INVALID
        rts