#ifndef AMIGA_TRANSPORT_HOST_EXEC_IO_H
#define AMIGA_TRANSPORT_HOST_EXEC_IO_H

#include <exec/ports.h>
#include <exec/types.h>

struct Device;
struct Unit;

struct IORequest {
    struct Message io_Message;
    struct Device *io_Device;
    struct Unit *io_Unit;
    UWORD io_Command;
    UBYTE io_Flags;
    BYTE io_Error;
};

#define CMD_NONSTD 9U

#endif
