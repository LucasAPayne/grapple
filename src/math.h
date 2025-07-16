#pragma once

#include "types.h"

// TODO(lucas): Replace standard math functions
#include <math.h>

#ifdef __cplusplus
    #define v2(x, y) {(x), (y)}
    #define v3(x, y, z) {(x), (y), (z)}
    #define v4(x, y, z, w) {(x), (y), (z), (w)}
#else
    #define v2(x, y) (v2){(x), (y)}
    #define v3(x, y, z) (v3){(x), (y), (z)}
    #define v4(x, y, z, w) (v4){(x), (y), (z), (w)}
#endif
typedef union
{
    struct
    {
        f32 x, y;
    };
    struct
    {
        f32 u, v;
    };
    f32 e[2];
} v2;

typedef union
{
    struct
    {
        union
        {
            v2 xy;
            struct
            {
                f32 x, y;
            };
        };
        f32 z;
    };
    struct
    {
        f32 r, g, b;
    };
    f32 e[3];
} v3;

typedef union
{
    struct
    {
        union
        {
            v3 xyz;
            struct
            {
                f32 x, y, z;
            };
        };
        f32 w;
    };
    struct
    {
        union
        {
            v3 rgb;
            struct
            {
                f32 r, g, b;
            };
        };
        f32 a;
    };
    f32 e[4];
} v4;

// IMPORTANT: This math library works with matrices in ROW MAJOR order
typedef struct
{
    f32 m[4][4];
} m4;

typedef struct
{
    v2 min;
    v2 max;
} rect;

//
// NOTE(lucas): Scalar operations
//

internal inline f32 sq_f32(f32 x)
{
    f32 result = x*x;
    return result;
}

internal inline f32 sqrt_f32(f32 x)
{
    f32 result = sqrtf(x);
    return result;
}

internal inline f32 lerp_f32(f32 a, f32 b, f32 t)
{
    f32 result = (1.0f - t)*a + t*b;
    return result;
}

//
// NOTE(lucas): v2 operations
//

internal inline v2 v2_full(f32 fill_value)
{
    v2 result = {fill_value, fill_value};
    return result;
}

internal inline v2 v2_zero(void)
{
    v2 result = v2(0.0f, 0.0f);
    return result;
}

internal inline v2 v2_scale(v2 v, f32 c)
{
    v2 result = v2(c*v.x, c*v.y);
    return result;
}

internal inline v2 v2_add(v2 a, v2 b)
{
    v2 result = v2(a.x + b.x, a.y + b.y);
    return result;
}

//
// NOTE(lucas): v3 operations
//

internal inline v3 v3_zero(void)
{
    v3 result = v3(0.0f, 0.0f, 0.0f);
    return result;
}

internal inline v3 v3_scale(v3 v, f32 c)
{
    v3 result = v3(c*v.x, c*v.y, c*v.z);
    return result;
}

//
// NOTE(lucas): v4 operations
//

internal inline v4 v4_zero(void)
{
    v4 result = v4(0.0f, 0.0f, 0.0f, 0.0f);
    return result;
}

internal inline v4 v4_scale(v4 v, f32 c)
{
    v4 result = v4(c*v.x, c*v.y, c*v.z, c*v.w);
    return result;
}

//
// NOTE(lucas): m4 operations
//

internal inline m4 ortho_top_left(f32 width, f32 height)
{
    m4 result = {0};
    result.m[0][0] =  2.0f / width;
    result.m[1][1] = -2.0f / height;
    result.m[2][2] = 1.0f;
    result.m[3][0] = -1.0f;
    result.m[3][1] = 1.0f;
    result.m[3][3] =  1.0f;
    return result;
}

internal m4 m4_transpose(m4 mat)
{
    m4 result = {0};
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
            result.m[row][col] = mat.m[col][row];
    }
    return result;
}

//
// NOTE(lucas): rect operations
//

internal inline rect rect_min_max(v2 min, v2 max)
{
    rect result = {0};

    result.min = min;
    result.max = max;

    return result;
}

internal inline rect rect_min_dim(v2 min, v2 dim)
{
    rect result = {0};

    result.min = min;
    result.max = v2_add(min, dim);

    return result;
}
