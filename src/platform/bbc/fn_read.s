        .export _fn_read

        .import __fn_initialized, __fn_sessions
        .import _fn_find_session, _fn_bbc_osbget, addysp, return0
        .importzp c_sp, ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, tmp4

FN_ERR_NOT_FOUND                  := $01
FN_ERR_INVALID                    := $02
FN_READ_EOF                       := $01
FN_SESSION_READ_OFFSET            := 10
FN_READ_STACK_BYTES               := 12
FN_SESSION_SIZE                   := 14

STACK_BYTES_READ_PTR              := 0
STACK_MAX_LEN                     := 2
STACK_BUF_PTR                     := 4
STACK_OFFSET                      := 6
STACK_HANDLE                      := 10

session_offsets:
        .repeat 5, I
        .byte   I * FN_SESSION_SIZE
        .endrepeat

; uint8_t fn_read(fn_handle_t handle,
;                 uint32_t offset,
;                 uint8_t *buf,
;                 uint16_t max_len,
;                 uint16_t *bytes_read,
;                 uint8_t *flags)

_fn_read:
        pha                         ; save flags ptr low
        txa
        pha                         ; save flags ptr high

        lda     __fn_initialized
        bne     @check_args

        pla
        pla
        jsr     fix_stack
        ldx     #$00
        lda     #FN_ERR_INVALID
        rts

@check_args:
        ldy     #STACK_HANDLE + 1
        lda     (c_sp),y
        tax
        dey
        lda     (c_sp),y
        sta     tmp3               ; handle low byte
        txa
        ora     tmp3
        beq     @invalid

        lda     tmp3
        jsr     _fn_find_session
        cpx     #$FF
        bne     @found

@not_found:
        pla
        pla
        jsr     fix_stack
        ldx     #$00
        lda     #FN_ERR_NOT_FOUND
        rts

@found:
        pla
        sta     ptr3+1
        pla
        sta     ptr3

        tay
        lda     session_offsets,y
        sta     tmp4

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
        beq     @is_valid

@invalid:
        jsr     fix_stack
        ldx     #$00
        lda     #FN_ERR_INVALID
        rts

@is_valid:
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
        bne     :+
        inc     ptr2+1
:
        inc     tmp1
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

        tya                     ; set A=0
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
