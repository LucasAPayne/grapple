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

    Arena arena = arena_alloc(MEGABYTES(10));
    Arena scratch_arena = arena_alloc(KILOBYTES(4));

    Renderer* renderer = renderer_create(window, &arena);

    m4 proj = ortho_top_left((f32)window_width, (f32)window_height);
    renderer_set_projection(renderer, proj);
    Texture texture = texture_load_from_file("res/icons/magnifying_glass.bmp", renderer, &arena);

    while (window->open)
    {
        input_process(window);

        if (!window->open)
            break;

        arena_clear(&scratch_arena);
        f32 delta_time = get_frame_seconds(window);

        renderer_begin_frame(renderer, window);
        v4 clear_color = v4(0.125f, 0.125f, 0.125f, 1.0f);
        renderer_clear(renderer, clear_color);

        v2 tex_size = v2_full(32.0f);
        for (u32 i = 0; i < 2000; ++i)
        {
            renderer_draw_texture(renderer, &texture, v2(150.0f, 50.0f),   tex_size);
            renderer_draw_texture(renderer, &texture, v2(200.0f, 50.0f),  tex_size);
            renderer_draw_texture(renderer, &texture, v2(150.0f, 100.0f),  tex_size);
            renderer_draw_texture(renderer, &texture, v2(200.0f, 100.0f), tex_size);
        }

        s8 batch_size_str = s8_format(&scratch_arena, "Batch size: %d", renderer->quads_per_batch);
        s8 quad_count_str = s8_format(&scratch_arena, "Num quads: %d", renderer->total_quads);
        s8 batch_count_str = s8_format(&scratch_arena, "Num batches: %d", renderer->batch_count);
        s8 frame_ms_str = s8_format(&scratch_arena, "Frame time: %.2fms", delta_time*1000.0f);
        s8 fps_str = s8_format(&scratch_arena, "FPS: %u", (u32)(1.0f/delta_time));

        v4 text_color = color_white();
        v2 text_bounds = v2_full(200.0f);

        text_draw(renderer, frame_ms_str, v2_zero(), text_bounds, text_color);
        text_draw(renderer, fps_str, v2(0.0f, 20.0f), text_bounds, text_color);
        text_draw(renderer, batch_size_str, v2(0.0f, 40.0f), text_bounds, text_color);
        text_draw(renderer, quad_count_str, v2(0.0f, 60.0f), text_bounds, text_color);
        text_draw(renderer, batch_count_str, v2(0.0f, 80.0f), text_bounds, text_color);

        text_draw(renderer, s8("Hello, Direct2D! αβγδεζηθ"), v2_full(200.0f), text_bounds, text_color);

        renderer_end_frame(renderer);
    }

    renderer_destroy(renderer);
    return 0;
}
