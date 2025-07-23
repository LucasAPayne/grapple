#pragma once

#include "grapple_math.h"
#include "grapple_memory.h"
#include "str.h"

#include <d3d11.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Renderer Renderer;
typedef struct TextRenderer TextRenderer;

TextRenderer* text_renderer_create(void* window_ptr, IDXGISwapChain* swap_chain, Arena* arena);
void text_renderer_destroy(TextRenderer* tr);
void text_draw_rect(Renderer* renderer, s8 text, rect bounds, v4 color);
void text_draw(Renderer* renderer, s8 text, v2 pos, v2 dim, v4 color);

#ifdef __cplusplus
}
#endif
