#pragma once

#include "grapple_math.h"
#include "grapple_memory.h"
#include "texture.h"
#include "types.h"
#include "window.h"

typedef struct Renderer Renderer;

internal Renderer* renderer_create(Window* window, Arena* arena);
internal void renderer_destroy(Renderer* renderer);

internal void renderer_set_projection(Renderer* renderer, m4 proj);
internal void renderer_upload_texture(Renderer* renderer, Texture* texture);
internal void renderer_draw_texture(Renderer* renderer, Texture* texture, v2 pos, v2 dim);

internal void renderer_clear(Renderer* renderer, v4 clear_color);
internal void renderer_begin_frame(Renderer* renderer, Window* window);
internal void renderer_end_frame(Renderer* renderer);

internal inline v4 color_red(void)    {return (v4){1.0f, 0.0f, 0.0f, 1.0f};}
internal inline v4 color_green(void)  {return (v4){0.0f, 1.0f, 0.0f, 1.0f};}
internal inline v4 color_blue(void)   {return (v4){0.0f, 0.0f, 1.0f, 1.0f};}
internal inline v4 color_black(void)  {return (v4){0.0f, 0.0f, 0.0f, 1.0f};}
internal inline v4 color_white(void)  {return (v4){1.0f, 1.0f, 1.0f, 1.0f};}
