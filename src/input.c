#include "input.h"

#ifdef _WIN32
    #include "platform/windows/win32_input.c"
#else
    #error "Unsupported platform!"
#endif
