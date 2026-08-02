; BBC-specific implementation of fn_appstore_read().
;
; cc65 default fastcall entry:
;   AX       = out
;   c_sp+0   = max_len
;   c_sp+2   = buf
;   c_sp+4   = offset (uint32_t)
;   c_sp+8   = key
;   c_sp+10  = namespace_name
;   c_sp+12  = io
;
; The out pointer is pushed on entry, making the offsets below two bytes
; larger. All sixteen argument bytes are removed on return.

        .macpack longbranch

        .export _fn_appstore_read

        .import _fn_appstore_build_prefix
        .import _fn_appstore_call
        .import _memmove
        .import pushax, pusha, pushwysp, addysp
        .importzp c_sp, ptr1, ptr2, tmp1, tmp2

FN_OK                         = $00
FN_ERR_INVALID                = $02
FN_ERR_IO                     = $05
FN_APPSTORE_PROTOCOL_VERSION  = $01
FN_CMD_APPSTORE_READ          = $02

IO_BUFFER                     = 0
IO_CAPACITY                   = 2

READ_OUT_FLAGS                = 0
READ_OUT_OFFSET               = 1
READ_OUT_BYTES                = 5

ARG_OUT                       = 0
ARG_MAX_LEN                   = 2
ARG_BUF                       = 4
ARG_OFFSET                    = 6
ARG_KEY                       = 10
ARG_NAMESPACE                 = 12
ARG_IO                        = 14
ARG_BYTES                     = 16

        .bss
response_len:   .res 2
work_len:       .res 2
saved_flags:    .res 1
saved_offset:   .res 4

        .code

_fn_appstore_read:
        ; Keep the fastcall out pointer on the software stack so it survives
        ; all nested C and assembly calls without persistent pointer storage.
        jsr     pushax

        ; out must be non-null.
        ldy     #ARG_OUT+1
        lda     (c_sp),y
        dey
        ora     (c_sp),y
        jeq     invalid

        ; max_len must be non-zero.
        ldy     #ARG_MAX_LEN
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     invalid

        ; A non-empty read requires a destination buffer. max_len is already
        ; known to be non-zero here.
        ldy     #ARG_BUF
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     invalid

        ; Required capacity is max_len + the ten-byte response header.
        ldy     #ARG_MAX_LEN
        lda     (c_sp),y
        clc
        adc     #10
        sta     tmp1
        iny
        lda     (c_sp),y
        adc     #0
        sta     tmp2
        jcs     invalid

        jsr     load_io
        jcs     invalid
        jsr     capacity_at_least_tmp
        jcs     invalid

        ; fn_appstore_build_prefix(io, namespace_name, key, 1)
        ; Each push moves the next original pointer to ARG_IO.
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

        ; The offset and maximum length add six request bytes.
        lda     work_len
        clc
        adc     #6
        sta     tmp1
        lda     work_len+1
        adc     #0
        sta     tmp2
        jcs     invalid

        jsr     load_io
        jcs     invalid
        jsr     capacity_at_least_tmp
        jcs     invalid

        ; ptr2 = io->buffer + prefix length.
        ldy     #IO_BUFFER
        lda     (ptr1),y
        clc
        adc     work_len
        sta     ptr2
        iny
        lda     (ptr1),y
        adc     work_len+1
        sta     ptr2+1

        ; Append the 32-bit requested offset.
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

        ; Append max_len.
        ldy     #ARG_MAX_LEN
        lda     (c_sp),y
        ldy     #4
        sta     (ptr2),y
        ldy     #ARG_MAX_LEN+1
        lda     (c_sp),y
        ldy     #5
        sta     (ptr2),y

        ; tmp held prefix + 6, which is the complete request length.
        lda     tmp1
        sta     work_len
        lda     tmp2
        sta     work_len+1

        ; fn_appstore_call(io, READ, request_len, &response_len)
        ldy     #ARG_IO+3
        jsr     pushwysp
        lda     #FN_CMD_APPSTORE_READ
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

        ; Response header is ten bytes and starts with the protocol version.
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

        ; work_len = response data length.
        ldy     #8
        lda     (ptr1),y
        sta     work_len
        iny
        lda     (ptr1),y
        sta     work_len+1

        ; Preserve response metadata before memmove. The caller may use
        ; io->buffer as its destination, in which case moving data from
        ; response+10 overwrites the response header.
        ldy     #1
        lda     (ptr1),y
        sta     saved_flags
        ldy     #4
        lda     (ptr1),y
        sta     saved_offset
        iny
        lda     (ptr1),y
        sta     saved_offset+1
        iny
        lda     (ptr1),y
        sta     saved_offset+2
        iny
        lda     (ptr1),y
        sta     saved_offset+3

        ; response_len must contain the header plus all returned data.
        lda     work_len
        clc
        adc     #10
        sta     tmp1
        lda     work_len+1
        adc     #0
        sta     tmp2
        jcs     io_error

        lda     response_len+1
        cmp     tmp2
        jcc     io_error
        bne     response_contains_data
        lda     response_len
        cmp     tmp1
        jcc     io_error
response_contains_data:

        ; Returned data may not exceed the caller's requested maximum.
        ldy     #ARG_MAX_LEN+1
        lda     work_len+1
        cmp     (c_sp),y
        jcc     data_fits
        jne     io_error
        dey
        lda     work_len
        cmp     (c_sp),y
        jeq     data_fits
        jcs     io_error

data_fits:
        lda     work_len
        ora     work_len+1
        beq     store_result

        ; memmove(buf, io->buffer + 10, work_len). The overlap-safe helper is
        ; retained because the public API permits buf to refer into io->buffer.
        ldy     #ARG_BUF+3
        jsr     pushwysp

        ; The original io argument moved by two bytes after pushing buf.
        ldy     #ARG_IO+2
        lda     (c_sp),y
        sta     ptr1
        iny
        lda     (c_sp),y
        sta     ptr1+1
        ldy     #IO_BUFFER
        lda     (ptr1),y
        clc
        adc     #10
        sta     ptr2
        iny
        lda     (ptr1),y
        adc     #0
        tax
        lda     ptr2
        jsr     pushax

        lda     work_len
        ldx     work_len+1
        jsr     _memmove

store_result:
        ; ptr2 = out.
        ldy     #ARG_OUT
        lda     (c_sp),y
        sta     ptr2
        iny
        lda     (c_sp),y
        sta     ptr2+1

        ; flags
        lda     saved_flags
        ldy     #READ_OUT_FLAGS
        sta     (ptr2),y

        ; echoed offset
        lda     saved_offset
        ldy     #READ_OUT_OFFSET
        sta     (ptr2),y
        lda     saved_offset+1
        ldy     #READ_OUT_OFFSET+1
        sta     (ptr2),y
        lda     saved_offset+2
        ldy     #READ_OUT_OFFSET+2
        sta     (ptr2),y
        lda     saved_offset+3
        ldy     #READ_OUT_OFFSET+3
        sta     (ptr2),y

        ; bytes_read
        lda     work_len
        ldy     #READ_OUT_BYTES
        sta     (ptr2),y
        lda     work_len+1
        iny
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

; Load and validate the io pointer and io->buffer. Carry is set on failure.
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

; Load io->buffer into ptr1. Carry is set if io or its buffer is null.
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

; Compare io->capacity against tmp2:tmp1. Carry is set when capacity is less.
; ptr1 must point to io.
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
