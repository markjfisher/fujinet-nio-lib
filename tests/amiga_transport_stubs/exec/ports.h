#ifndef AMIGA_TRANSPORT_HOST_EXEC_PORTS_H
#define AMIGA_TRANSPORT_HOST_EXEC_PORTS_H

#include <exec/nodes.h>

struct MsgPort {
    struct Node mp_Node;
    UBYTE mp_Flags;
    UBYTE mp_SigBit;
    void *mp_SigTask;
};

struct Message {
    struct Node mn_Node;
    struct MsgPort *mn_ReplyPort;
    UWORD mn_Length;
};

#endif
