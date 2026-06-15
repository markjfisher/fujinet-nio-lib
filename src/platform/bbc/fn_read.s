        .export _fn_read

        .import __fn_initialized, __fn_sessions
        .import invalid_with_stack_fix
        .import _fn_bbc_rw_setup_read, _fn_bbc_osbget, addysp, return0
        .importzp c_sp, ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, tmp4

        .include "fn_protocol.inc"

FN_READ_STACK_BYTES               := 12

STACK_BYTES_READ_PTR              := 0
STACK_MAX_LEN                     := 2
STACK_BUF_PTR                     := 4
STACK_OFFSET                      := 6

; uint8_t fn_read(fn_handle_t handle,
;                 uint32_t offset,
;                 uint8_t *buf,
;                 uint16_t max_len,
;                 uint16_t *bytes_read,
;                 uint8_t *flags)

_fn_read:
        ldy     __fn_initialized
        bne     @check_args
        ldy     #FN_READ_STACK_BYTES
        jmp     invalid_with_stack_fix

@check_args:
        jsr     _fn_bbc_rw_setup_read
        bcc     @have_session
        cmp     #FN_ERR_NOT_FOUND
        bne     @invalid

@not_found:
        jsr     fix_stack
        ldx     #$00
        lda     #FN_ERR_NOT_FOUND
        rts

@invalid:
        jsr     fix_stack
        ldx     #$00
        lda     #FN_ERR_INVALID
        rts

@have_session:
        ldy     #STACK_BYTES_READ_PTR
        lda     (c_sp),y
        sta     ptr1
        iny
        lda     (c_sp),y
        sta     ptr1+1
        lda     ptr1
        ora     ptr1+1
        beq     @invalid

        ldy     #STACK_MAX_LEN
        lda     (c_sp),y
        sta     ptr4
        iny
        lda     (c_sp),y
        sta     ptr4+1

        ldy     #STACK_BUF_PTR
        lda     (c_sp),y
        sta     ptr2
        iny
        lda     (c_sp),y
        sta     ptr2+1
        lda     ptr2
        ora     ptr2+1
        beq     @invalid

        ldy     #STACK_OFFSET
        lda     (c_sp),y
        ldy     tmp4
        cmp     __fn_sessions + FN_SESSION_READ_OFFSET,y
        bne     @invalid
        ldy     #STACK_OFFSET + 1
        lda     (c_sp),y
        ldy     tmp4
        cmp     __fn_sessions + FN_SESSION_READ_OFFSET + 1,y
        bne     @invalid
        ldy     #STACK_OFFSET + 2
        lda     (c_sp),y
        ldy     tmp4
        cmp     __fn_sessions + FN_SESSION_READ_OFFSET + 2,y
        bne     @invalid
        ldy     #STACK_OFFSET + 3
        lda     (c_sp),y
        ldy     tmp4
        cmp     __fn_sessions + FN_SESSION_READ_OFFSET + 3,y
        bne     @invalid

        lda     #$00
        sta     tmp1
        sta     tmp2

@loop:
        lda     tmp1
        cmp     ptr4
        lda     tmp2
        sbc     ptr4+1
        bcs     @done

        lda     tmp3
        jsr     _fn_bbc_osbget
        cpx     #$FF
        beq     @done

        ldy     #$00
        sta     (ptr2),y
        inc     ptr2
        bne     @count
        inc     ptr2+1
@count: inc     tmp1
        bne     @loop
        inc     tmp2
        bne     @loop

@done:
        ldy     #$00
        lda     tmp1
        sta     (ptr1),y
        iny
        lda     tmp2
        sta     (ptr1),y

        lda     ptr3
        ora     ptr3+1
        beq     @update_offset

        lda     tmp1
        cmp     ptr4
        lda     tmp2
        sbc     ptr4+1
        ldy     #$00
        bcc     @set_eof
        tya
        beq     @store_flag
@set_eof:
        lda     #FN_READ_EOF
@store_flag:
        sta     (ptr3),y

@update_offset:
        ldy     tmp4
        clc
        lda     __fn_sessions + FN_SESSION_READ_OFFSET,y
        adc     tmp1
        sta     __fn_sessions + FN_SESSION_READ_OFFSET,y
        lda     __fn_sessions + FN_SESSION_READ_OFFSET + 1,y
        adc     tmp2
        sta     __fn_sessions + FN_SESSION_READ_OFFSET + 1,y
        lda     __fn_sessions + FN_SESSION_READ_OFFSET + 2,y
        adc     #$00
        sta     __fn_sessions + FN_SESSION_READ_OFFSET + 2,y
        lda     __fn_sessions + FN_SESSION_READ_OFFSET + 3,y
        adc     #$00
        sta     __fn_sessions + FN_SESSION_READ_OFFSET + 3,y
        jsr     fix_stack
        jmp     return0

fix_stack:
        ldy     #FN_READ_STACK_BYTES
        jmp     addysp
