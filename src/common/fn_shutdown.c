#include "fn_internal.h"
#include "fn_platform.h"

#include <string.h>

void fn_shutdown(void)
{
    fn_transport_close();
    memset(_fn_sessions, 0, sizeof(_fn_sessions));
    _fn_initialized = 0;
}
