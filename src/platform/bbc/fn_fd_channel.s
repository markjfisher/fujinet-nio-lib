        .export _fn_bbc_fd_getchannel

        .import __fd_getchannel

; unsigned char __fastcall__ fn_bbc_fd_getchannel(unsigned char fd)
.proc _fn_bbc_fd_getchannel
        jmp     __fd_getchannel
.endproc
