#include "file.h"

#ifdef _WIN32
    #include "platform/windows/win32_file.c"
#else
    #error "Unsupported platform!"
#endif