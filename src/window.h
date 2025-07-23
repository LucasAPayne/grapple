#pragma once

#include "types.h"

typedef struct
{
    int width;
    int height;
    b32 open;

    // Timing information used to calculate delta seconds for each frame.
    // Not intended to be accessed
    i64 ticks_per_second;
    i64 prev_frame_ticks;

    void* ptr; // OS handle to window
} Window;
