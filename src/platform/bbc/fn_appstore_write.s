; BBC-specific implementation of fn_appstore_write().
;
; cc65 default fastcall entry:
;   AX       = out
;   c_sp+0   = len
;   c_sp+2   = data
;   c_sp+4   = offset (uint32_t)
;   c_sp+8   = key
;   c_sp+10  = namespace_name
;   c_sp+12  = io
;
; The out pointer is pushed on entry, making the offsets below two bytes
; larger. All sixteen argument bytes are removed on return.

        .macpack longbranch

        .export _fn_appstore_write

        .import _fn_appstore_build_prefix
        .import _fn_appstore_call
        .import pushax, pusha, pushwysp, addysp
        .importzp c_sp, ptr1, ptr2, tmp1, tmp2

FN_OK                         = $00
FN_ERR_INVALID                = $02
FN_ERR_IO                     = $05
FN_APPSTORE_PROTOCOL_VERSION  = $01
FN_CMD_APPSTORE_WRITE         = $03

IO_BUFFER                     = 0
IO_CAPACITY                   = 2

WRITE_OUT_OFFSET              = 0
WRITE_OUT_BYTES               = 4

ARG_OUT                       = 0
ARG_LEN                       = 2
ARG_DATA                      = 4
ARG_OFFSET                    = 6
ARG_KEY                       = 10
ARG_NAMESPACE                 = 12
ARG_IO                        = 14
ARG_BYTES                     = 16

        .bss
response_len:   .res 2
work_len:       .res 2

        .code

_fn_appstore_write:
        jsr     pushax

        ; out must be non-null.
        ldy     #ARG_OUT+1
        lda     (c_sp),y
        dey
        ora     (c_sp),y
        jeq     invalid

        ; A non-empty write requires a source pointer.
        ldy     #ARG_LEN
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        beq     data_pointer_ok
        ldy     #ARG_DATA
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     invalid
data_pointer_ok:

        ; io and its buffer must exist, with room for a response header.
        jsr     load_io
        jcs     invalid
        lda     #10
        sta     tmp1
        lda     #0
        sta     tmp2
        jsr     capacity_at_least_tmp
        jcs     invalid

        ; fn_appstore_build_prefix(io, namespace_name, key, 1)
        ldy     #ARG_IO+3
        jsr     pushwysp
        ldy     #ARG_IO+3
        jsr     pushwysp
        ldy     #ARG_IO+3
        jsr     pushwysp
        lda     #1
        jsr     _fn_appstore_build_prefix

        sta     work_len
        stx     work_len+1
        ora     work_len+1
        jeq     invalid

        ; tmp = prefix + six-byte write header + len.
        lda     work_len
        clc
        adc     #6
        sta     tmp1
        lda     work_len+1
        adc     #0
        sta     tmp2
        jcs     invalid

        ldy     #ARG_LEN
        lda     tmp1
        clc
        adc     (c_sp),y
        sta     tmp1
        iny
        lda     tmp2
        adc     (c_sp),y
        sta     tmp2
        jcs     invalid

        jsr     load_io
        jcs     invalid
        jsr     capacity_at_least_tmp
        jcs     invalid

        ; ptr1 = io->buffer + prefix length.
        ldy     #IO_BUFFER
        lda     (ptr1),y
        clc
        adc     work_len
        sta     ptr2
        iny
        lda     (ptr1),y
        adc     work_len+1
        sta     ptr2+1

        ; Append the 32-bit write offset.
        ldy     #ARG_OFFSET
        lda     (c_sp),y
        ldy     #0
        sta     (ptr2),y
        ldy     #ARG_OFFSET+1
        lda     (c_sp),y
        ldy     #1
        sta     (ptr2),y
        ldy     #ARG_OFFSET+2
        lda     (c_sp),y
        ldy     #2
        sta     (ptr2),y
        ldy     #ARG_OFFSET+3
        lda     (c_sp),y
        ldy     #3
        sta     (ptr2),y

        ; Append len.
        ldy     #ARG_LEN
        lda     (c_sp),y
        ldy     #4
        sta     (ptr2),y
        ldy     #ARG_LEN+1
        lda     (c_sp),y
        ldy     #5
        sta     (ptr2),y

        ; Advance destination past the write header.
        lda     ptr2
        clc
        adc     #6
        sta     ptr2
        bcc     copy_setup
        inc     ptr2+1

copy_setup:
        ; ptr1 = data, response_len is a temporary copy counter.
        ldy     #ARG_DATA
        lda     (c_sp),y
        sta     ptr1
        iny
        lda     (c_sp),y
        sta     ptr1+1
        ldy     #ARG_LEN
        lda     (c_sp),y
        sta     response_len
        iny
        lda     (c_sp),y
        sta     response_len+1
        ora     response_len
        beq     copy_done

copy_loop:
        ldy     #0
        lda     (ptr1),y
        sta     (ptr2),y
        inc     ptr1
        bne     :+
        inc     ptr1+1
:       inc     ptr2
        bne     :+
        inc     ptr2+1
:       lda     response_len
        bne     :+
        dec     response_len+1
:       dec     response_len
        lda     response_len
        ora     response_len+1
        bne     copy_loop

copy_done:
        ; tmp still contains the complete request length.
        lda     tmp1
        sta     work_len
        lda     tmp2
        sta     work_len+1

        ; fn_appstore_call(io, WRITE, request_len, &response_len)
        ldy     #ARG_IO+3
        jsr     pushwysp
        lda     #FN_CMD_APPSTORE_WRITE
        jsr     pusha
        lda     work_len
        ldx     work_len+1
        jsr     pushax
        lda     #<response_len
        ldx     #>response_len
        jsr     _fn_appstore_call
        cmp     #FN_OK
        jne     result

        jsr     load_response
        jcs     io_error

        lda     response_len+1
        bne     response_size_ok
        lda     response_len
        cmp     #10
        jcc     io_error
response_size_ok:
        ldy     #0
        lda     (ptr1),y
        cmp     #FN_APPSTORE_PROTOCOL_VERSION
        jne     io_error

        ; ptr2 = out.
        ldy     #ARG_OUT
        lda     (c_sp),y
        sta     ptr2
        iny
        lda     (c_sp),y
        sta     ptr2+1

        ; echoed offset
        ldy     #4
        lda     (ptr1),y
        ldy     #WRITE_OUT_OFFSET
        sta     (ptr2),y
        ldy     #5
        lda     (ptr1),y
        ldy     #WRITE_OUT_OFFSET+1
        sta     (ptr2),y
        ldy     #6
        lda     (ptr1),y
        ldy     #WRITE_OUT_OFFSET+2
        sta     (ptr2),y
        ldy     #7
        lda     (ptr1),y
        ldy     #WRITE_OUT_OFFSET+3
        sta     (ptr2),y

        ; bytes_written
        ldy     #8
        lda     (ptr1),y
        ldy     #WRITE_OUT_BYTES
        sta     (ptr2),y
        ldy     #9
        lda     (ptr1),y
        ldy     #WRITE_OUT_BYTES+1
        sta     (ptr2),y

        lda     #FN_OK
        beq     result

invalid:
        lda     #FN_ERR_INVALID
        bne     result

io_error:
        lda     #FN_ERR_IO

result:
        ldx     #0
        ldy     #ARG_BYTES
        jmp     addysp

load_io:
        ldy     #ARG_IO
        lda     (c_sp),y
        sta     ptr1
        iny
        lda     (c_sp),y
        sta     ptr1+1
        ora     ptr1
        beq     load_invalid

        ldy     #IO_BUFFER
        lda     (ptr1),y
        tax
        iny
        lda     (ptr1),y
        beq     check_buffer_low
        clc
        rts
check_buffer_low:
        txa
        beq     load_invalid
        clc
        rts
load_invalid:
        sec
        rts

load_response:
        jsr     load_io
        bcs     load_response_done
        ldy     #IO_BUFFER
        lda     (ptr1),y
        tax
        iny
        lda     (ptr1),y
        sta     ptr1+1
        stx     ptr1
        clc
load_response_done:
        rts

capacity_at_least_tmp:
        ldy     #IO_CAPACITY+1
        lda     (ptr1),y
        cmp     tmp2
        bcc     capacity_too_small
        bne     capacity_ok
        dey
        lda     (ptr1),y
        cmp     tmp1
        bcc     capacity_too_small
capacity_ok:
        clc
        rts
capacity_too_small:
        sec
        rts
