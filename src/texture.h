#pragma once

#include "memory.h"
#include "types.h"

typedef struct
{
    b32 found;
    DWORD index;
} BitScanResult;

typedef struct
{
    i32 channels;
    i32 width;
    i32 height;
    u8* data;
} Texture;

Texture load_bmp_from_memory(u8* data, size data_size);
Texture load_bmp_from_file(char* filename, Arena* arena);
