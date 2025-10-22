#include "grapple_math.h"
#include "types.h"
#include "str.h"

#include "grapple_memory.c"
#include "input.c"
#include "window.c"
#include "renderer/renderer.c"
#include "renderer/texture.c"

typedef struct
{
    s8 name;
    s8 path;
} Project;

internal inline void open_project(s8 proj_path, Arena* arena)
{
    char* dir = s8_get_char(arena, proj_path);
    open_vs_code(dir);
    arena_pop(arena, proj_path.len+1);
}

// TODO(lucas): Fuzz testing on ini parsing
internal inline u32 get_num_projects(s8 settings_str)
{
    u32 result = 0;

    s8 line = {0};
    b32 parsing_projects = false;
    b32 process_line = false;
    b32 comment = false;
    size line_start = 0;
    size line_end = 0;
    for (size i = 0; i < settings_str.len; ++i)
    {
        u8 c = settings_str.data[i];
        if (c == '\n' && !comment)
        {
            process_line = true;
            line_end = i;
            line = s8_trim(s8_slice(settings_str, line_start, line_end));
        }
        else if (c == ';' || c == '#')
        {
            comment = true;
            process_line = true;
            line_end = max(i-1, 0);
            line = s8_trim(s8_slice(settings_str, line_start, line_end));

            while (i < settings_str.len && settings_str.data[i] != '\n')
                ++i;
        }

        if (process_line)
        {
            if (parsing_projects)
            {
                if (s8_begins_with(line, '[') && s8_ends_with(line, ']'))
                {
                    parsing_projects = false;
                    break;
                }

                s8_pair project = s8_split_first(line, '=');
                if (project.right.data)
                    ++result;
            }
            else
            {
                if (s8_eq(line, s8("[Projects]")))
                    parsing_projects = true;
            }
            line_start = i+1;

            comment = false;
            process_line = false;
        }
    }

    return result;
}

internal inline b32 has_valid_drive_colon(s8 s)
{
    b32 result = true;

    u8 first = s.data[0];
    u8 sec = s.data[1];
    b32 has_drive = false;

    if ((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z'))
    {
        if (sec == ':')
            has_drive = true;
    }

    s8 after_drive = {s.data + 2, s.len - 2};
    if (has_drive)
    {
        if (s8_contains(after_drive, ':'))
            result = false;
    }
    else if (s8_contains(s, ':'))
        result = false;

    return result;
}

internal inline Project* load_projects(s8 settings_str, u32 num_projects, Arena* arena)
{
    Project* projects = push_array(arena, num_projects, Project);

    s8 line = {0};
    u32 proj_idx = 0;
    b32 parsing_projects = false;
    b32 process_line = false;
    b32 comment = false;
    size line_start = 0;
    size line_end = 0;
    for (size i = 0; i < settings_str.len; ++i)
    {
        u8 c = settings_str.data[i];
        if (c == '\n' && !comment)
        {
            process_line = true;
            line_end = i;
            line = s8_trim(s8_slice(settings_str, line_start, line_end));
        }
        else if (c == ';' || c == '#')
        {
            comment = true;
            process_line = true;
            line_end = max(i-1, 0);
            line = s8_trim(s8_slice(settings_str, line_start, line_end));

            while (i < settings_str.len && settings_str.data[i] != '\n')
                ++i;
        }

        if (process_line)
        {
            if (parsing_projects)
            {
                if (s8_begins_with(line, '[') && s8_ends_with(line, ']'))
                {
                    parsing_projects = false;
                    break;
                }

                s8_pair project = s8_split_first(line, '=');
                if (project.right.data)
                {
                    s8 proj_name = s8_trim(project.left);
                    s8 proj_path = s8_trim(project.right);

                    // TODO(lucas): Support UNC paths
                    b32 valid_path = !(s8_contains_any(proj_path, s8("<>\"|?*")) || !has_valid_drive_colon(proj_path) ||
                        s8_contains_ctrl(proj_path));

                    if (!valid_path)
                    {
                        // TODO(lucas): Make platform-independent
                        char buf[1024];
                        if (proj_name.len + proj_path.len < sizeof(buf))
                        {
                            StringCchPrintfA(buf, countof(buf), "Invalid path for project %.*s: %.*s. Paths may not "
                            "contain '<', '>', ':', '\"', '?', '*', or any ASCII control characters.",
                                (int)proj_name.len, (char*)proj_name.data, (int)proj_path.len, (char*)proj_path.data);
                            MessageBoxA(0, buf, "Invalid Path", MB_OK |MB_ICONERROR);
                        }
                    }
                    else
                        projects[proj_idx++] = (Project){.name=proj_name, .path=proj_path};
                }
            }
            else
            {
                if (s8_eq(line, s8("[Projects]")))
                    parsing_projects = true;
            }
            line_start = i+1;

            comment = false;
            process_line = false;
        }
    }

    return projects;
}

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
    TextureAtlas atlas = texture_atlas_load_from_file("res/grapple_atlas.bmp", renderer, &arena, 2, 2, 32, 32, 4, 4);
    renderer->atlas = &atlas;

    f32 font_size = text_renderer_get_font_size(renderer->text_renderer);

    size max_len = 64;
    size chars = 0;
    s8 buffer = s8_alloc(&arena, max_len*sizeof(u32));

    f32 caret_timer = 0.0f;
    f32 blink_rate = 0.5f;
    b32 show_caret = true;
    size caret_idx = 0;

    char* settings_path = "config/grapple.ini";
    size settings_size = file_get_size(settings_path);
    s8 settings_str = s8_alloc(&arena, settings_size);
    settings_str.len = settings_size;
    void* settings_file = file_open(settings_path, FileMode_Read);
    if (!settings_file)
    {
        // TODO(lucas): Message box
        return 1;
    }
    file_read(settings_file, settings_str.data, settings_size);
    file_close(settings_file);

    u32 num_projects = get_num_projects(settings_str);
    Project* projects = load_projects(settings_str, num_projects, &arena);

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
            else if (input.current_char == '\r') // enter
            {
                for (u32 proj_idx = 0; proj_idx < num_projects; ++proj_idx)
                {
                    Project project = projects[proj_idx];

                    if (s8_eq(buffer, project.name) || s8_eq(buffer, project.path))
                    {
                        open_project(project.path, &scratch_arena);

                        // TODO(lucas): Return to system tray
                        return 0;
                    }
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
        // TODO(lucas): Mouse input (focusing, text selection)
        // TODO(lucas): Only show caret when text box is focused
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

        v2 window_center = v2((f32)window->width/2.0f, (f32)window->height/2.0f);
        v2 text_box_size = v2(200.0f, font_size+10.0f);
        v2 icon_size = v2_full(text_box_size.y);

        v2 text_box_pos = v2(window_center.x - (text_box_size.x - icon_size.x)/2.0f, window_center.y - text_box_size.y/2.0f);
        v2 icon_pos = v2(text_box_pos.x - icon_size.x - 5.0f, text_box_pos.y);

        f32 padding = 4.0f;
        rect text_box = rect_min_dim(text_box_pos, text_box_size);
        rect text_box_border = rect(text_box.x-1.0f, text_box.y-1.0f, text_box.w+2.0f, text_box.h+2.0f);
        rect text_bounds = rect(text_box.x+padding, text_box.y, text_box.w-padding, text_box.h);
        TextMetrics metrics =  text_get_metrics(renderer->text_renderer, buffer, text_bounds, caret_idx);

        f32 scroll = 0.0f;
        if (metrics.text_width < text_bounds.w - padding)
            scroll = 0.0f;
        else
            scroll = metrics.text_width - text_bounds.w + padding;

        metrics.caret_pos.x -= scroll;
        rect cursor = rect(metrics.caret_pos.x, metrics.caret_pos.y+4.0f, 2.0f, font_size+2.0f);

        draw_texture(renderer, &atlas, 1, icon_pos, icon_size);
        draw_quad(renderer, text_box_border, color_white());
        draw_quad(renderer, text_box, clear_color);

        if (show_caret)
            draw_quad(renderer, cursor, color_white());

        // Quads must be flushed before drawing text because text is drawn immediately.
        flush_quads(renderer);
        draw_text_rect(renderer, buffer, text_bounds, color_white(), scroll);

        renderer_end_frame(renderer);
    }

    renderer_destroy(renderer);
    return 0;
}
