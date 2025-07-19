#include "grapple_memory.h"

#ifdef _WIN32
    #include "platform/windows/win32_memory.c"
#else
    #error "Unsupported platform!"
#endif
