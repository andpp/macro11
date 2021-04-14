
#ifndef REPT_IRPC__H
#define REPT_IRPC__H

#include "stream2.h"


#ifndef REPT_IRPC__C
// extern STREAM_VTBL rept_stream_vtbl;
// extern STREAM_VTBL irp_stream_vtbl;
// extern STREAM_VTBL irpc_stream_vtbl;
#endif
struct REPT_STREAM;
struct IRP_STREAM;
struct IRPC_STREAM;

REPT_STREAM    *expand_rept(STACK *stack, char *cp);
IRP_STREAM     *expand_irp(STACK *stack, char *cp);
IRPC_STREAM    *expand_irpc(STACK *stack, char *cp);

#endif
