#pragma once

#include "types.h"
#include "memory.h"

#include <stdarg.h> // varargs

#define s8(s) (s8){(u8*)s, lengthof(s)}
typedef struct
{
    u8* data;
    size len;
} s8;

internal inline s8 s8_alloc(Arena* arena, size len)
{
    s8 result = {0};
    result.data = push_array(arena, len, u8);
    result.len = len;
    return result;
}

internal inline s8 s8_format(Arena* arena, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    size len = (size)vsnprintf(NULL, 0, format, args);
    va_end(args);

    s8 result = s8_alloc(arena, len);

    va_start(args, format);
    vsnprintf((char*)result.data, len+1, format, args);
    va_end(args);

    return result;
}
