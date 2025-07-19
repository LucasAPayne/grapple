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

        renderer_draw_texture(renderer, &texture, v2(50.0f, 50.0f), v2(32.0f, 32.0f));

        s8 frame_ms_str = s8_format(&scratch_arena, "Frame time: %.2fms", delta_time*1000.0f);
        s8 fps_str = s8_format(&scratch_arena, "FPS: %u", (u32)(1.0f/delta_time));
        text_draw(renderer, frame_ms_str, rect_min_dim(v2_zero(), v2_full(200.0f)), color_white());
        text_draw(renderer, fps_str, rect_min_dim(v2(0.0f, 20.0f), v2_full(200.0f)), color_white());
        text_draw(renderer, s8("Hello, Direct2D! αβγδεζηθ"), rect_min_dim(v2_full(200.0f), v2_full(200.0f)), color_white());

        renderer_end_frame(renderer);
    }

    renderer_destroy(renderer);
    return 0;
}
