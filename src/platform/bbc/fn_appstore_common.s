        .export _fn_appstore_validate_io
        .export _fn_slot_catalog_validate_io
        .export _fn_appstore_build_prefix

        .import popax, return0

        .importzp ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3

FN_OK = $00
FN_ERR_INVALID = $02
FN_FILEPROTO_VERSION = $01

        .bss
key_required:  .res 1
ns_len:        .res 1
key_len:       .res 1
req_lo:        .res 1
req_hi:        .res 1

        .code

; uint8_t fn_appstore_validate_io(fn_appstore_io_t *io, uint16_t min_capacity)
; min_capacity arrives in AX, io is on the cc65 argument stack.
_fn_appstore_validate_io:
        sta     tmp1
        stx     tmp2
        jsr     popax
        sta     ptr1
        stx     ptr1+1
        jsr     validate_io_ptr1
        bcs     @invalid
        lda     #FN_OK
        tax
        rts
@invalid:
        lda     #FN_ERR_INVALID
        ldx     #$00
        rts

_fn_slot_catalog_validate_io := _fn_appstore_validate_io

; uint16_t fn_appstore_build_prefix(fn_appstore_io_t *io,
;                                   const char *namespace_name,
;                                   const char *key,
;                                   uint8_t key_required)
; key_required arrives in A, then key, namespace_name, io are on the stack.
_fn_appstore_build_prefix:
        sta     key_required
        jsr     popax
        sta     ptr3
        stx     ptr3+1
        jsr     popax
        sta     ptr2
        stx     ptr2+1
        jsr     popax
        sta     ptr1
        stx     ptr1+1

        jsr     strlen_ptr2_required
        bcc     @ns_ok
        jmp     @zero
@ns_ok:
        stx     ns_len

        lda     ptr3
        ora     ptr3+1
        bne     @have_key
        lda     key_required
        beq     @empty_key_ok
        jmp     @zero
@empty_key_ok:
        ldx     #$00
        beq     @key_ok
@have_key:
        jsr     strlen_ptr3
        bcc     @key_len_ok
        jmp     @zero
@key_len_ok:
        cpx     #$00
        bne     @key_ok
        lda     key_required
        beq     @key_ok
        jmp     @zero
@key_ok:
        stx     key_len

        lda     ns_len
        clc
        adc     key_len
        sta     req_lo
        lda     #$00
        adc     #$00
        sta     req_hi
        lda     req_lo
        clc
        adc     #$05
        sta     req_lo
        bcc     @req_done
        inc     req_hi
@req_done:
        lda     req_lo
        sta     tmp1
        lda     req_hi
        sta     tmp2
        jsr     validate_io_ptr1
        bcs     @zero

        ldy     #$00
        lda     (ptr1),y
        sta     ptr4
        iny
        lda     (ptr1),y
        sta     ptr4+1

        ldy     #$00
        lda     #FN_FILEPROTO_VERSION
        sta     (ptr4),y
        iny
        lda     ns_len
        sta     (ptr4),y
        iny
        lda     #$00
        sta     (ptr4),y
        iny

        lda     ptr4
        clc
        adc     #$03
        sta     ptr4
        bcc     @copy_ns_setup
        inc     ptr4+1
@copy_ns_setup:
        ldy     #$00
@copy_ns:
        cpy     ns_len
        beq     @after_ns
        lda     (ptr2),y
        sta     (ptr4),y
        iny
        bne     @copy_ns
@after_ns:
        lda     ptr4
        clc
        adc     ns_len
        sta     ptr4
        bcc     @store_key_len
        inc     ptr4+1
@store_key_len:
        ldy     #$00
        lda     key_len
        sta     (ptr4),y
        iny
        lda     #$00
        sta     (ptr4),y
        iny

        lda     ptr4
        clc
        adc     #$02
        sta     ptr4
        bcc     @copy_key_setup
        inc     ptr4+1
@copy_key_setup:
        ldy     #$00
@copy_key:
        cpy     key_len
        beq     @done
        lda     (ptr3),y
        sta     (ptr4),y
        iny
        bne     @copy_key
@done:
        lda     req_lo
        ldx     req_hi
        rts
@zero:
        jmp     return0

; C pointer validation with:
;   ptr1 = fn_appstore_io_t*
;   tmp2:tmp1 = required capacity
; Carry set on invalid.
validate_io_ptr1:
        lda     ptr1
        ora     ptr1+1
        beq     @invalid
        ldy     #$00
        lda     (ptr1),y
        tax
        iny
        lda     (ptr1),y
        beq     @check_buf_low
        bne     @have_buf
@check_buf_low:
        txa
        beq     @invalid
@have_buf:
        ldy     #$03
        lda     (ptr1),y
        cmp     tmp2
        bcc     @invalid
        bne     @valid
        dey
        lda     (ptr1),y
        cmp     tmp1
        bcc     @invalid
@valid:
        clc
        rts
@invalid:
        sec
        rts

strlen_ptr2_required:
        lda     ptr2
        ora     ptr2+1
        beq     @invalid
        ldx     #$00
        ldy     #$00
@loop:
        lda     (ptr2),y
        beq     @end
        inx
        iny
        bne     @loop
@invalid:
        sec
        rts
@end:
        cpx     #$00
        beq     @invalid
        clc
        rts

strlen_ptr3:
        ldx     #$00
        ldy     #$00
@loop:
        lda     (ptr3),y
        beq     @end
        inx
        iny
        bne     @loop
        sec
        rts
@end:
        clc
        rts
