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

    f32 font_size = text_renderer_get_font_size(renderer->text_renderer);

    size max_len = 64;
    size chars = 0;
    s8 buffer = s8_alloc(&arena, max_len*sizeof(u32));

    f32 caret_timer = 0.0f;
    f32 blink_rate = 0.5f;
    b32 show_caret = true;
    size caret_idx = 0;

    while (window->open)
    {
        f32 dt = get_frame_seconds(window);
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
                if (caret_idx > 0 && buffer.len > 0)
                {
                    // For UTF-8 text, keep walking back until the byte does not have the UTF-8 continuation byte.
                    // In other words, stop when a byte is found that does *not* begin with 0b10...
                    size i = caret_idx - 1;
                    while (buffer.len > 0 && ((buffer.data[i] & 0xC0) == 0x80))
                        --i;

                    size bytes_to_delete = caret_idx - i;
                    for (size b = 0; b < bytes_to_delete; ++b)
                        s8_delete(&buffer, i);

                    caret_idx = i;

                    if (chars > 0)
                        --chars;
                }
            }
            else
            {
                if (chars < max_len)
                {
                    u8 utf8[4] = {0};
                    u32_to_bytes((u32)input.current_char, utf8);
                    int num_bytes = utf8_get_num_bytes(utf8[0]);
                    if (input.current_char <= UINT32_MAX)
                    {
                        for (int i = 0; i < num_bytes; ++i)
                            s8_insert(&buffer, utf8[i], caret_idx+i, max_len);
                    }

                    caret_idx += num_bytes;
                    ++chars;
                }
            }
        }
        else if (input.del)
        {
            if (caret_idx >= 0 && buffer.len > 0)
            {
                // For UTF-8 text, keep walking back until the byte does not have the UTF-8 continuation byte.
                // In other words, stop when a byte is found that does *not* begin with 0b10...
                size i = caret_idx + 1;
                while (buffer.len > 0 && ((buffer.data[i] & 0xC0) == 0x80))
                    ++i;

                size bytes_to_delete = i - caret_idx;
                for (size b = 0; b < bytes_to_delete; ++b)
                    s8_delete(&buffer, caret_idx);

                if (chars > 0)
                    --chars;
            }
        }

        /* Update */
        caret_timer += dt;
        if (input.current_char || input.left_arrow || input.right_arrow)
        {
            show_caret = true;
            caret_timer = 0.0f;
        }
        if (caret_timer > blink_rate)
        {
            show_caret = !show_caret;
            caret_timer = 0.0f;
        }

        // TODO(lucas): The caret index needs to look at the next or previous character and potentially jump multiple bytes
        if (caret_idx > 0 && input.left_arrow)
        {
            size i = caret_idx - 1;
            while (buffer.len > 0 && ((buffer.data[i] & 0xC0) == 0x80))
                --i;
            caret_idx = i;
        }
        if (caret_idx < buffer.len && input.right_arrow)
        {
            int num_bytes = utf8_get_num_bytes(buffer.data[caret_idx]);
            caret_idx += num_bytes;
        }

        /* Draw */
        renderer_begin_frame(renderer, window);
        v4 clear_color = v4(0.125f, 0.125f, 0.125f, 1.0f);
        renderer_clear(renderer, clear_color);

        // TODO(lucas): If the text exceeds the horizontal bounds, start scrolling horizontally
        f32 padding = 2.0f;
        rect text_box = rect(200.0f, 200.0f, 200.0f, font_size+10.0f);
        rect text_box_border = rect(text_box.x-1.0f, text_box.y-1.0f, text_box.w+2.0f, text_box.h+2.0f);
        rect text_bounds = rect(text_box.x+padding, text_box.y, text_box.w-padding, text_box.h);
        v2 caret_pos = text_get_cursor_position(renderer->text_renderer, buffer, text_bounds, caret_idx);
        rect cursor = rect(caret_pos.x, caret_pos.y+2.0f, 2.0f, font_size+2.0f);
        renderer_draw_quad(renderer, text_box_border, color_white());
        renderer_draw_quad(renderer, text_box, clear_color);

        if (show_caret)
            renderer_draw_quad(renderer, cursor, color_white());

        // Quads must be flushed before drawing text because text is drawn immediately.
        renderer_flush_quads(renderer);
        text_draw_rect(renderer, buffer, text_bounds, color_white());

        renderer_end_frame(renderer);
    }

    renderer_destroy(renderer);
    return 0;
}
