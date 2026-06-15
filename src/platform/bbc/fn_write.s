        .export _fn_write

        .import __fn_initialized, __fn_sessions
        .import _fn_find_session, _fn_bbc_osword78, addysp, return0
        .importzp c_sp, ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, tmp4

FN_ERR_NOT_FOUND                  := $01
FN_ERR_INVALID                    := $02
FN_ERR_IO                         := $05
FN_WRITE_STACK_BYTES              := 10
FN_SESSION_SIZE                   := 14
FN_SESSION_WRITE_OFFSET           := 6
FN_BBC_REASON_WRITE_DATA          := $02

STACK_LEN                         := 0
STACK_DATA_PTR                    := 2
STACK_OFFSET                      := 4
STACK_HANDLE                      := 8

session_offsets:
        .repeat 3, I
        .byte   I * FN_SESSION_SIZE
        .endrepeat

        .bss
write_block:
        .res    16
        .code

_fn_write:
        pha                         ; save written ptr low
        txa
        pha                         ; save written ptr high

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
        sta     tmp3                ; handle low
        txa
        ora     tmp3
        beq     @invalid_preserved

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

@invalid_preserved:
        pla
        pla
        jsr     fix_stack
        ldx     #$00
        lda     #FN_ERR_INVALID
        rts

@found:
        tay
        lda     session_offsets,y
        sta     tmp4                ; session offset

        pla
        sta     ptr3+1
        pla
        sta     ptr3                ; written ptr

        ldy     #STACK_OFFSET
        lda     (c_sp),y
        ldy     tmp4
        cmp     __fn_sessions + FN_SESSION_WRITE_OFFSET,y
        beq     :+
        jmp     @invalid
: 
        ldy     #STACK_OFFSET + 1
        lda     (c_sp),y
        ldy     tmp4
        cmp     __fn_sessions + FN_SESSION_WRITE_OFFSET + 1,y
        beq     :+
        jmp     @invalid
: 
        ldy     #STACK_OFFSET + 2
        lda     (c_sp),y
        ldy     tmp4
        cmp     __fn_sessions + FN_SESSION_WRITE_OFFSET + 2,y
        beq     :+
        jmp     @invalid
: 
        ldy     #STACK_OFFSET + 3
        lda     (c_sp),y
        ldy     tmp4
        cmp     __fn_sessions + FN_SESSION_WRITE_OFFSET + 3,y
        beq     :+
        jmp     @invalid
: 

        lda     ptr3
        ora     ptr3+1
        beq     :+
        ldy     #$00
        tya
        sta     (ptr3),y
        iny
        sta     (ptr3),y
:
        ldy     #STACK_LEN
        lda     (c_sp),y
        sta     ptr4
        iny
        lda     (c_sp),y
        sta     ptr4+1              ; len total
        lda     ptr4
        ora     ptr4+1
        bne     :+
        jmp     @success
: 

        ldy     #STACK_DATA_PTR
        lda     (c_sp),y
        sta     ptr2
        iny
        lda     (c_sp),y
        sta     ptr2+1              ; current data pointer
        lda     ptr2
        ora     ptr2+1
        bne     :+
        jmp     @invalid
: 

        lda     #$00
        sta     tmp1
        sta     tmp2                ; total written

        ldy     #15
        tya
@zero_block:
        sta     write_block,y
        dey
        bpl     @zero_block
        lda     #FN_BBC_REASON_WRITE_DATA
        sta     write_block
        lda     tmp3
        sta     write_block + 6

@loop:
        sec
        lda     ptr4
        sbc     tmp1
        sta     ptr1
        lda     ptr4+1
        sbc     tmp2
        tax                         ; X = remaining high
        cpx     #$02
        bcc     @use_remaining
        lda     #$00
        sta     write_block + 4
        lda     #$02
        sta     write_block + 5
        bne     @have_chunk

@use_remaining:
        lda     ptr1
        sta     write_block + 4
        txa
        sta     write_block + 5

@have_chunk:
        lda     ptr2
        sta     write_block + 2
        lda     ptr2+1
        sta     write_block + 3

        lda     #<write_block
        ldx     #>write_block
        jsr     _fn_bbc_osword78
        beq     @advance
        cmp     #$03
        bne     :+
        jmp     @write_not_found
: 
        cmp     #$01
        bne     :+
        jmp     @invalid
: 
        jmp     @io_error

@write_not_found:
        jsr     fix_stack
        ldx     #$00
        lda     #FN_ERR_NOT_FOUND
        rts

@io_error:
        jsr     fix_stack
        ldx     #$00
        lda     #FN_ERR_IO
        rts

@advance:
        clc
        lda     ptr2
        adc     write_block + 4
        sta     ptr2
        lda     ptr2+1
        adc     write_block + 5
        sta     ptr2+1

        clc
        lda     tmp1
        adc     write_block + 4
        sta     tmp1
        lda     tmp2
        adc     write_block + 5
        sta     tmp2

        lda     tmp1
        cmp     ptr4
        lda     tmp2
        sbc     ptr4+1
        bcs     :+
        jmp     @loop
: 

        ldy     tmp4
        clc
        lda     __fn_sessions + FN_SESSION_WRITE_OFFSET,y
        adc     tmp1
        sta     __fn_sessions + FN_SESSION_WRITE_OFFSET,y
        lda     __fn_sessions + FN_SESSION_WRITE_OFFSET + 1,y
        adc     tmp2
        sta     __fn_sessions + FN_SESSION_WRITE_OFFSET + 1,y
        lda     __fn_sessions + FN_SESSION_WRITE_OFFSET + 2,y
        adc     #$00
        sta     __fn_sessions + FN_SESSION_WRITE_OFFSET + 2,y
        lda     __fn_sessions + FN_SESSION_WRITE_OFFSET + 3,y
        adc     #$00
        sta     __fn_sessions + FN_SESSION_WRITE_OFFSET + 3,y

        lda     ptr3
        ora     ptr3+1
        beq     @success
        ldy     #$00
        lda     tmp1
        sta     (ptr3),y
        iny
        lda     tmp2
        sta     (ptr3),y

@success:
        jsr     fix_stack
        jmp     return0

@invalid:
        jsr     fix_stack
        ldx     #$00
        lda     #FN_ERR_INVALID
        rts

fix_stack:
        ldy     #FN_WRITE_STACK_BYTES
        jmp     addysp
