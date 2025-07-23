#include "file.c"
#include "grapple_math.h"
#include "grapple_memory.h"
#include "texture.h"

// TODO(lucas): Full bitmap support should separate the BMP header from the DIB header
// and allow using different versions of the DIB header.
#pragma pack(push, 1)
typedef struct BitmapHeader
{
    // BMP Header
    u16 file_type;
    u32 file_size;
    u16 reserved1;
    u16 reserved2;
    u32 pixel_array_offset;

    // DIB Header
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 bitmap_size;
    i32 horiz_resolution;
    i32 vert_resolution;
    u32 colors_used;
    u32 colors_important;
    /*
     * NOTE(lucas): When compression is set to 3, it indicates the use of bitfield encoding.
     * In this case, there are masks for the RGB channels, indicating their location.
     * The masks and locations of these channels may be different in any given bitmap.
     */
    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
} BitmapHeader;
#pragma pack(pop)

BitScanResult find_least_significant_bit(u32 value)
{
    BitScanResult result = {0};
    result.found = _BitScanForward(&result.index, value);
    return result;
}

inline v4 srgb255_to_linear1(v4 c)
{
    v4 result = v4_zero();

    f32 inv_255 = 1.0f/255.0f;

    result.r = sq_f32(inv_255*c.r);
    result.g = sq_f32(inv_255*c.g);
    result.b = sq_f32(inv_255*c.b);
    result.a = inv_255*c.a;

    return result;
}

inline v4 linear1_to_srgb255(v4 c)
{
    v4 result = v4_zero();

    result.r = 255.0f*sqrt_f32(c.r);
    result.g = 255.0f*sqrt_f32(c.g);
    result.b = 255.0f*sqrt_f32(c.b);
    result.a = 255.0f*c.a;

    return result;
}

Texture load_bmp_from_memory(u8* data, size data_size)
{
    Texture tex = {0};
    if (data_size)
    {
        BitmapHeader* header = (BitmapHeader*)data;

        u32 bytes_per_pixel = header->bits_per_pixel / 8;

        tex.width = header->width;
        tex.height = header->height;
        tex.channels = bytes_per_pixel;

        switch(header->compression)
        {
            case 0: // BI_RGB: no compression
            {
                // BGR -> RGB
                u8* pixels = data + header->pixel_array_offset;
                tex.data = pixels;
                u32 row_size = ((header->width * bytes_per_pixel + 3) & ~3); // Each row is padded to multiple of 4 bytes
                for (i32 y = 0; y < header->height; ++y)
                {
                    u8* row = pixels + y * row_size; // BMP is bottom-up
                    for (i32 x = 0; x < header->width; ++x)
                    {
                        u8* pixel = row + x * bytes_per_pixel;

                        u8 temp = pixel[0];
                        pixel[0] = pixel[2];
                        pixel[2] = temp;
                    }
                }
            } break;

            case 3: // BI_BITFIELDS
            {
                u32* pixels = (u32*)(data + header->pixel_array_offset);
                tex.data = (u8*)pixels;

                u32 red_mask = header->red_mask;
                u32 green_mask = header->green_mask;
                u32 blue_mask = header->blue_mask;
                u32 alpha_mask = ~(red_mask | green_mask | blue_mask);

                BitScanResult red_scan   = find_least_significant_bit(red_mask);
                BitScanResult green_scan = find_least_significant_bit(green_mask);
                BitScanResult blue_scan  = find_least_significant_bit(blue_mask);
                BitScanResult alpha_scan = find_least_significant_bit(alpha_mask);

                ASSERT(red_scan.found, "Scan for red mask failed");
                ASSERT(green_scan.found, "Scan for green mask failed");
                ASSERT(blue_scan.found, "Scan for blue mask failed");
                ASSERT(alpha_scan.found, "Scan for alpha mask failed");

                i32 red_shift_down   = (i32)red_scan.index;
                i32 green_shift_down = (i32)green_scan.index;
                i32 blue_shift_down  = (i32)blue_scan.index;
                i32 alpha_shift_down = (i32)alpha_scan.index;

                u32* pixel = pixels;
                for (i32 y = 0; y < header->height; ++y)
                {
                    for (i32 x = 0; x < header->width; ++x)
                    {
                        u32 c = *pixel;

                        v4 texel = {(f32)((c & red_mask)   >> red_shift_down),
                            (f32)((c & green_mask) >> green_shift_down),
                            (f32)((c & blue_mask)  >> blue_shift_down),
                            (f32)((c & alpha_mask) >> alpha_shift_down)};
                        texel = srgb255_to_linear1(texel);

                        // BGR -> RGB
                        v3 temp = v3_scale(v3(texel.b, texel.g, texel.r), texel.a);
                        texel.r = temp.r;
                        texel.g = temp.g;
                        texel.b = temp.b;

                        texel = linear1_to_srgb255(texel);
                        *pixel++ = ((u32)(texel.a + 0.5f) << 24) |
                                   ((u32)(texel.r + 0.5f) << 16) |
                                   ((u32)(texel.g + 0.5f) << 8)  |
                                   ((u32)(texel.b + 0.5f) << 0);
                    }
                }
            } break;

            default: ASSERTF(0, "Unsupported/invalid compression type (%u)", header->compression); break;
        }
    }

    ASSERT(tex.data, "Failed to load texture");
    return tex;
}

Texture load_bmp_from_file(char* filename, Arena* arena)
{
    size file_size = file_get_size(filename);
    void* file = file_open(filename, FileMode_Read);
    u8* data = push_size(arena, file_size);
    file_read(file, data, file_size);
    Texture result = load_bmp_from_memory(data, file_size);

    return result;
}

Texture texture_load_from_file(char* filename, Renderer* renderer, Arena* arena)
{
    size file_size = file_get_size(filename);
    void* file = file_open(filename, FileMode_Read);
    u16 signature = 0;
    file_read(file, &signature, sizeof(signature));
    b32 is_bmp = (signature == 0x4D42);

    ASSERT(is_bmp, "Only BMP textures are currently supported");

    Texture tex = {0};
    if (is_bmp)
    {
        file_seek_begin(file);
        u8* data = push_size(arena, file_size);
        file_read(file, data, file_size);
        tex = load_bmp_from_memory(data, file_size);
        file_close(file);
    }
    else
        return tex;

    ASSERT(tex.data, "Failed to laod texture");

    renderer_upload_texture(renderer, &tex);
    return tex;
}
