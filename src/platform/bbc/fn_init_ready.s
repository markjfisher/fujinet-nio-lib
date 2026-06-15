        .export _fn_init
        .export _fn_is_ready

        .import __fn_initialized, __fn_sessions
        .import _fn_bbc_probe_rom, return0

        .include "fn_protocol.inc"

; uint8_t fn_init(void)
_fn_init:
        ldy     __fn_initialized
        bne     @ready

        jsr     _fn_bbc_probe_rom
        bne     @clear_sessions
        ldx     #$00
        lda     #FN_ERR_NOT_FOUND
        rts

@clear_sessions:
        lda     #$00
        ldx     #FN_MAX_SESSIONS * FN_SESSION_SIZE - 1
@clear_loop:
        sta     __fn_sessions,x
        dex
        bpl     @clear_loop
        ldy     #$01
        sty     __fn_initialized

@ready:
        jmp     return0

_fn_is_ready:
        jmp     _fn_bbc_probe_rom
