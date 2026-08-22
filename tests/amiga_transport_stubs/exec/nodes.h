#ifndef AMIGA_TRANSPORT_HOST_EXEC_NODES_H
#define AMIGA_TRANSPORT_HOST_EXEC_NODES_H

#include <exec/types.h>

struct Node {
    struct Node *ln_Succ;
    struct Node *ln_Pred;
    UBYTE ln_Type;
    BYTE ln_Pri;
    char *ln_Name;
};

#define NT_UNKNOWN 0
#define NT_MESSAGE 5

#endif
