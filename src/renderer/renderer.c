#include "renderer.h"

#ifdef _WIN32
    #include "d3d11_renderer.c"
#else
    #error "No renderer backend defined!"
#endif
