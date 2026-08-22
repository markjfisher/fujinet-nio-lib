#ifndef AMIGA_TRANSPORT_HOST_ALIB_PROTOS_H
#define AMIGA_TRANSPORT_HOST_ALIB_PROTOS_H

#include <exec/ports.h>
#include <exec/types.h>

struct MsgPort *CreatePort(CONST_STRPTR name, LONG pri);
void DeletePort(struct MsgPort *port);

#endif
