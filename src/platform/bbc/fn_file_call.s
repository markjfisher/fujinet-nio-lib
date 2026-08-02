; BBC OSWORD &78 device-call helpers.
;
; These implement the default cc65 fastcall interfaces declared in
; fn_bbc_internal.h. The rightmost response_len argument arrives in AX and
; the remaining arguments are stored right-to-left on the cc65 C stack.

        .macpack longbranch

        .export _fn_bbc_device_call_raw
        .export _fn_bbc_device_call
        .export _fn_bbc_file_call

        .import _fn_bbc_osword78
        .import _fn_bbc_status_to_result
        .import pusha, pushax, addysp
        .importzp c_sp, ptr1, tmp1

FN_OK                         = $00
FN_ERR_INVALID                = $02
FN_ERR_UNKNOWN                = $FF
FN_DEVICE_FILE                = $FE
FN_BBC_REASON_DEVICE_CALL     = $06
FN_BBC_STATUS_OK              = $00

; fn_bbc_device_call_raw C-stack offsets. response_len is in AX.
RAW_DEVICE_STATUS             = 0
RAW_RESPONSE_CAPACITY         = 2
RAW_RESPONSE                  = 4
RAW_REQUEST_LEN               = 6
RAW_REQUEST                   = 8
RAW_COMMAND                   = 10
RAW_DEVICE                    = 11
RAW_ARG_BYTES                 = 12

; fn_bbc_device_call C-stack offsets. response_len is in AX.
CALL_RESPONSE_CAPACITY        = 0
CALL_RESPONSE                 = 2
CALL_REQUEST_LEN              = 4
CALL_REQUEST                  = 6
CALL_COMMAND                  = 8
CALL_DEVICE                   = 9
CALL_ARG_BYTES                = 10

; fn_bbc_file_call C-stack offsets. response_len is in AX.
FILE_RESPONSE_CAPACITY        = 0
FILE_RESPONSE                 = 2
FILE_REQUEST_LEN              = 4
FILE_REQUEST                  = 6
FILE_COMMAND                  = 8
FILE_ARG_BYTES                = 9

        .bss
device_block:          .res 16
saved_response_len:    .res 2
mapped_device_status:  .res 1

        .code

; Placed for closer access from branches
raw_invalid:
        lda     #FN_ERR_INVALID
raw_result:
        ldx     #0
        ldy     #RAW_ARG_BYTES
        jmp     addysp

; uint8_t fn_bbc_device_call_raw(uint8_t device, uint8_t command,
;     const uint8_t *request, uint16_t request_len, uint8_t *response,
;     uint16_t response_capacity, uint8_t *device_status,
;     uint16_t *response_len)
_fn_bbc_device_call_raw:
        sta     saved_response_len
        stx     saved_response_len+1
        ora     saved_response_len+1
        jeq     raw_invalid

        ; device_status must be non-null.
        ldy     #RAW_DEVICE_STATUS
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     raw_invalid

        ; A non-empty request requires a request buffer.
        ldy     #RAW_REQUEST_LEN
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        beq     raw_request_ok
        ldy     #RAW_REQUEST
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     raw_invalid
raw_request_ok:

        ; A non-empty response capacity requires a response buffer.
        ldy     #RAW_RESPONSE_CAPACITY
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        beq     raw_response_ok
        ldy     #RAW_RESPONSE
        lda     (c_sp),y
        iny
        ora     (c_sp),y
        jeq     raw_invalid
raw_response_ok:

        ; Clear all reserved bytes in the sixteen-byte OSWORD block.
        lda     #0
        ldx     #15
raw_clear_block:
        sta     device_block,x
        dex
        bpl     raw_clear_block

        lda     #FN_BBC_REASON_DEVICE_CALL
        sta     device_block
        ldy     #RAW_DEVICE
        lda     (c_sp),y
        sta     device_block+2
        ldy     #RAW_COMMAND
        lda     (c_sp),y
        sta     device_block+3

        ldy     #RAW_REQUEST
        lda     (c_sp),y
        sta     device_block+5
        iny
        lda     (c_sp),y
        sta     device_block+6
        ldy     #RAW_REQUEST_LEN
        lda     (c_sp),y
        sta     device_block+7
        iny
        lda     (c_sp),y
        sta     device_block+8

        ldy     #RAW_RESPONSE
        lda     (c_sp),y
        sta     device_block+9
        iny
        lda     (c_sp),y
        sta     device_block+10
        ldy     #RAW_RESPONSE_CAPACITY
        lda     (c_sp),y
        sta     device_block+11
        iny
        lda     (c_sp),y
        sta     device_block+12

        lda     #<device_block
        ldx     #>device_block
        jsr     _fn_bbc_osword78
        sta     tmp1

        ; The ROM always returns its response length, including ROM errors.
        lda     saved_response_len
        sta     ptr1
        lda     saved_response_len+1
        sta     ptr1+1
        ldy     #0
        lda     device_block+13
        sta     (ptr1),y
        iny
        lda     device_block+14
        sta     (ptr1),y

        lda     tmp1
        cmp     #FN_BBC_STATUS_OK
        bne     raw_rom_error

        ldy     #RAW_DEVICE_STATUS
        lda     (c_sp),y
        sta     ptr1
        iny
        lda     (c_sp),y
        sta     ptr1+1
        ldy     #0
        lda     device_block+4
        sta     (ptr1),y
        lda     #FN_OK
        jeq     raw_result

raw_rom_error:
        jsr     _fn_bbc_status_to_result
        jmp     raw_result


; uint8_t fn_bbc_device_call(uint8_t device, uint8_t command,
;     const uint8_t *request, uint16_t request_len, uint8_t *response,
;     uint16_t response_capacity, uint16_t *response_len)
_fn_bbc_device_call:
        sta     saved_response_len
        stx     saved_response_len+1

        ; Push the raw-call arguments from left to right. Once device is
        ; pushed, command is at offset 9; each remaining word is at offset 8.
        ldy     #CALL_DEVICE
        lda     (c_sp),y
        jsr     pusha
        ldy     #CALL_COMMAND+1
        lda     (c_sp),y
        jsr     pusha
        jsr     push_word_at_8
        jsr     push_word_at_8
        jsr     push_word_at_8
        jsr     push_word_at_8
        lda     #<mapped_device_status
        ldx     #>mapped_device_status
        jsr     pushax
        lda     saved_response_len
        ldx     saved_response_len+1
        jsr     _fn_bbc_device_call_raw
        cmp     #FN_OK
        bne     call_result

        ; Device statuses 0..8 intentionally match FN_OK..FN_ERR_UNSUPPORTED.
        lda     mapped_device_status
        cmp     #9
        bcc     call_result
        lda     #FN_ERR_UNKNOWN
call_result:
        ldx     #0
        ldy     #CALL_ARG_BYTES
        jmp     addysp

; uint8_t fn_bbc_file_call(uint8_t command, const uint8_t *request,
;     uint16_t request_len, uint8_t *response, uint16_t response_capacity,
;     uint16_t *response_len)
_fn_bbc_file_call:
        sta     saved_response_len
        stx     saved_response_len+1

        lda     #FN_DEVICE_FILE
        jsr     pusha
        ldy     #FILE_COMMAND+1
        lda     (c_sp),y
        jsr     pusha
        jsr     push_word_at_8
        jsr     push_word_at_8
        jsr     push_word_at_8
        jsr     push_word_at_8
        lda     saved_response_len
        ldx     saved_response_len+1
        jsr     _fn_bbc_device_call

        ldx     #0
        ldy     #FILE_ARG_BYTES
        jmp     addysp

; With two leading byte arguments already pushed, each successive original
; word argument is found at c_sp+8. pushax then advances the stack by two.
push_word_at_8:
        ldy     #8
        lda     (c_sp),y
        sta     tmp1
        iny
        lda     (c_sp),y
        tax
        lda     tmp1
        jmp     pushax
