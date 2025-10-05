#include "grapple_math.h"
#include "types.h"
#include "str.h"

#include "grapple_memory.c"
#include "input.c"
#include "window.c"
#include "renderer/renderer.c"
#include "renderer/texture.c"

int main(void)
{
    int window_width = 800;
    int window_height = 600;
    Window* window = window_create("Grapple", window_width, window_height);
    Input input = {0};

    Arena arena = arena_alloc(MEGABYTES(10));
    Arena scratch_arena = arena_alloc(KILOBYTES(4));

    Renderer* renderer = renderer_create(window, &arena);

    m4 proj = ortho_top_left((f32)window_width, (f32)window_height);
    renderer_set_projection(renderer, proj);
    TextureAtlas atlas = texture_atlas_load_from_file("res/grapple_atlas.bmp", renderer, &arena, 2, 2, 32, 32);
    renderer->atlas = &atlas;

    size max_len = 64;
    size chars = 0;
    s8 buffer = s8_alloc(&arena, max_len*sizeof(u32));

    while (window->open)
    {
        input_process(window, &input);

        if (!window->open)
            break;

        arena_clear(&scratch_arena);

        /* Input */
        // TODO(lucas): Handle input from the Windows emoji picker
        if (input.current_char)
        {
            if (input.current_char == '\b') // backspace
            {
                // For UTF-8 text, keep walking back until the byte does not have the UTF-8 continuation byte.
                // In other words, stop when a byte is found that does *not* begin with 0b10...
                while (buffer.len > 0 && ((buffer.data[buffer.len-1] & 0xC0) == 0x80))
                {
                    --buffer.len;
                    buffer.data[buffer.len] = 0;
                }
                if (buffer.len > 0)
                {
                    --chars;
                    --buffer.len;
                    buffer.data[buffer.len] = 0;
                }
            }
            else
            {
                char utf8[4];
                wchar_t wc = (wchar_t)input.current_char;
                int num_bytes = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, utf8, sizeof(utf8), NULL, NULL);
                if (chars < max_len)
                {
                    ++chars;
                    if (input.current_char <= UINT32_MAX)
                    {
                        for (int i = 0; i < num_bytes; ++i)
                            buffer.data[buffer.len + i] = utf8[i];
                        buffer.len += num_bytes;
                    }
                }
            }
        }

        /* Draw */
        renderer_begin_frame(renderer, window);
        v4 clear_color = v4(0.125f, 0.125f, 0.125f, 1.0f);
        renderer_clear(renderer, clear_color);

        v2 text_bounds = v2_full(200.0f);
        text_draw(renderer, buffer, v2_full(200.0f), text_bounds, color_white());

        renderer_end_frame(renderer);
    }

    renderer_destroy(renderer);
    return 0;
}
