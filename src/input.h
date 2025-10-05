#pragma once

#include "window.h"

typedef struct
{
    u64 current_char;
} Input;

void input_process(Window* window, Input* input);
b32  clipboard_write_string(char* text);
char* clipboard_read_string(void);
