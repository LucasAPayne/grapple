#include "window.h"

#ifdef _WIN32
    #include "platform/windows/win32_window.c"
#else
    #error "Unsupported platform!"
#endif