#include "renderer.h"

#ifdef _WIN32
    #include "renderer/d3d11/d3d11_renderer.c"
#else
    #error "No renderer backend defined!"
#endif
