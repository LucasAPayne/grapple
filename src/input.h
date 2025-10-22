#pragma once

#include "window.h"

typedef struct
{
    u64 current_char;
    b32 left_arrow;
    b32 right_arrow;
    b32 del;
} Input;

void input_process(Window* window, Input* input);
b32  clipboard_write_string(char* text);
char* clipboard_read_string(void);
