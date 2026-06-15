        .export _fn_find_free_slot
        .export _fn_find_session
        .export _fn_free_handle

        .import __fn_sessions, popax, return0
        .import session_offsets

        .importzp ptr1, tmp1

        .include "fn_protocol.inc"

; int8_t fn_find_free_slot(void)
_fn_find_free_slot:
        ldx     #$00
@loop_free:
        lda     session_offsets,x
        tay
        lda     __fn_sessions + FN_SESSION_ACTIVE,y
        beq     @found_free
        inx
        cpx     #FN_MAX_SESSIONS
        bcc     @loop_free
        lda     #$FF
        tax
        rts
@found_free:
        txa
        ldx     #$00
        rts

; int8_t fn_find_session(fn_handle_t handle)
; handle low byte in A for current BBC callers, high byte ignored/expected zero
_fn_find_session:
        sta     tmp1
        ldx     #$00
@loop_find:
        lda     session_offsets,x
        tay
        lda     __fn_sessions + FN_SESSION_ACTIVE,y
        beq     @next_find
        lda     __fn_sessions + FN_SESSION_HANDLE + 1,y
        bne     @next_find
        lda     __fn_sessions + FN_SESSION_HANDLE,y
        cmp     tmp1
        beq     @found_session
@next_find:
        inx
        cpx     #FN_MAX_SESSIONS
        bcc     @loop_find
        lda     #$FF
        tax
        rts
@found_session:
        txa
        ldx     #$00
        rts

; void fn_free_handle(fn_handle_t handle)
; standard C calling convention: handle on stack
_fn_free_handle:
        jsr     popax
        sta     tmp1
        txa
        bne     @done
        lda     tmp1
        beq     @done

        jsr     _fn_find_session
        cpx     #$FF
        beq     @done
        tay
        lda     session_offsets,y
        tay
        lda     #$00
        sta     __fn_sessions + FN_SESSION_ACTIVE,y
        ldx     #FN_SESSION_SIZE - FN_SESSION_HANDLE
@clear_rest:
        sta     __fn_sessions + FN_SESSION_HANDLE - 1,x
        dex
        bne     @clear_rest
@done:
        jmp     return0
