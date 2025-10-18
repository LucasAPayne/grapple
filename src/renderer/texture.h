#pragma once

#include "grapple_memory.h"
#include "types.h"

typedef struct Renderer Renderer;

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
    void* api_handle;
} Texture;

// TODO(lucas): Allow for atlas regions to be subdivided (e.g., put 4 smaller textures in the space of one normal size)?
typedef struct
{
    u32 total_textures;
    u32 textures_per_row;
    i32 tex_width;     // per sub-texture
    i32 tex_height;    // per sub-texture
    i32 gutter_width;  // blank pixels horizontally separating sub-textures
    i32 gutter_height; // blank pixels vertically separating sub-textures
    Texture tex;       // entire atlas texture
} TextureAtlas;

Texture load_bmp_from_memory(u8* data, size data_size);
Texture load_bmp_from_file(char* filename, Arena* arena);
TextureAtlas texture_atlas_load_from_file(char* filename, Renderer* renderer, Arena* arena, u32 total_textures,
    u32 textures_per_row, i32 tex_width, i32 tex_height, i32 gutter_width, i32 gutter_height);
rect texture_atlas_uv_from_index(TextureAtlas* atlas, u32 idx); // UV coordinates are given from texel centers
