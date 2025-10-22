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

typedef struct
{
    v2 caret_pos;
    f32 text_width;
} TextMetrics;

TextRenderer* text_renderer_create(void* window_ptr, IDXGISwapChain* swap_chain, Arena* arena);
void text_renderer_destroy(TextRenderer* tr);

f32 text_renderer_get_font_size(TextRenderer* tr);

// caret_idx is the byte index into the UTF-8 string
TextMetrics text_get_metrics(TextRenderer* tr, s8 text, rect bounds, size caret_idx);

void draw_text_rect(Renderer* renderer, s8 text, rect bounds, v4 color, f32 scroll);
void draw_text(Renderer* renderer, s8 text, v2 pos, v2 dim, v4 color, f32 scroll);

#ifdef __cplusplus
}
#endif
