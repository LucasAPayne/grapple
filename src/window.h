#pragma once

#include "types.h"

typedef struct
{
    int width;
    int height;
    b32 open;
    b32 woke_this_frame;

    // Timing information used to calculate delta seconds for each frame.
    // Not intended to be accessed
    i64 ticks_per_second;
    i64 prev_frame_ticks;

    void* ptr; // OS handle to window
} Window;

Window* window_create(const char* title, int width, int height);

void window_show(Window* window);
void window_hide(Window* window);

void* window_icon_load_from_file(const char* filename);
void window_icon_set_from_memory(Window* window, void* icon);
void window_icon_set_from_resource(int id);

void open_vs_code(char* proj_path, b32 vs_code);
