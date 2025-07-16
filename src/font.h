#pragma once

#include "math.h"
#include "memory.h"
#include "str.h"

#include <d3d11.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TextRenderer TextRenderer;

TextRenderer* text_renderer_create(void* window_ptr, IDXGISwapChain* swap_chain, Arena* arena);
void text_renderer_destroy(TextRenderer* tr);
void text_draw(TextRenderer* tr, s8 text, rect bounds, v4 color);

#ifdef __cplusplus
}
#endif
