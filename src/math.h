#include "types.h"

#define v2(x, y) (v2){(x), (y)}
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

#define v3(x, y, z) (v3){(x), (y), (z)}
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

#define v4(x, y, z, w) (v4){(x), (y), (z), (w)}
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
