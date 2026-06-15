        .export _fn_bbc_claim_channel

        .import __fn_sessions
        .import _close_file
        .import _fn_find_free_slot
        .import popax
        .import return0

        .importzp ptr1
        .importzp tmp1

; uint8_t fn_bbc_claim_channel(fn_handle_t *handle, unsigned char channel)
;   A    = channel
;   stack: handle pointer

FN_ERR_NO_HANDLES                 := $12
FN_PROTO_FLAG_SEQUENTIAL_BOTH     := $03
FN_SESSION_ACTIVE                 := 0
FN_SESSION_PROTO_FLAGS            := 1
FN_SESSION_NEEDS_BODY             := 2
FN_SESSION_RESERVED               := 3
FN_SESSION_HANDLE                 := 4
FN_SESSION_WRITE_OFFSET           := 6
FN_SESSION_READ_OFFSET            := 10
FN_SESSION_SIZE                   := 14

session_offsets:
        .repeat 5, I
        .byte   I * FN_SESSION_SIZE
        .endrepeat

.proc _fn_bbc_claim_channel
        sta     tmp1                ; save channel

        jsr     popax               ; handle pointer
        sta     ptr1
        stx     ptr1+1

        ldy     #$00
        lda     tmp1
        sta     (ptr1),y            ; *handle low = channel
        tya                         ; set A=0
        iny
        sta     (ptr1),y            ; *handle high = 0

        jsr     _fn_find_free_slot
        cmp     #$FF
        beq     no_handles

        tay
        lda     session_offsets,y
        tay

        lda     #$01
        sta     __fn_sessions + FN_SESSION_ACTIVE,y
        lda     #FN_PROTO_FLAG_SEQUENTIAL_BOTH
        sta     __fn_sessions + FN_SESSION_PROTO_FLAGS,y
        lda     #$00
        sta     __fn_sessions + FN_SESSION_NEEDS_BODY,y
        sta     __fn_sessions + FN_SESSION_RESERVED,y
        sta     __fn_sessions + FN_SESSION_HANDLE + 1,y
        sta     __fn_sessions + FN_SESSION_WRITE_OFFSET,y
        sta     __fn_sessions + FN_SESSION_WRITE_OFFSET + 1,y
        sta     __fn_sessions + FN_SESSION_WRITE_OFFSET + 2,y
        sta     __fn_sessions + FN_SESSION_WRITE_OFFSET + 3,y
        sta     __fn_sessions + FN_SESSION_READ_OFFSET,y
        sta     __fn_sessions + FN_SESSION_READ_OFFSET + 1,y
        sta     __fn_sessions + FN_SESSION_READ_OFFSET + 2,y
        sta     __fn_sessions + FN_SESSION_READ_OFFSET + 3,y
        lda     tmp1
        sta     __fn_sessions + FN_SESSION_HANDLE,y
        jmp     return0

no_handles:
        lda     tmp1
        jsr     _close_file
        lda     #FN_ERR_NO_HANDLES
        ldx     #$00
        rts
.endproc
