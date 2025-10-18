#pragma once

#include "types.h"
#include "grapple_memory.h"

#include <stdarg.h> // varargs
#include <string.h> // memcpy
#include <stdio.h>  // vsnprintf

#define s8(s) (s8){(u8*)s, lengthof(s)}
typedef struct
{
    u8* data;
    size len;
} s8;

typedef struct
{
    s8 left;
    s8 right;
} s8_pair;

internal inline b32 char_is_space(u8 c)
{
    b32 result = false;
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f')
        result = true;
    return result;
}

// Given the first byte of a UTF-8 encoded character, returns the total number of bytes for that character
internal inline int utf8_get_num_bytes(u8 c)
{
    persist const char lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
    };

    int num_bytes = lengths[c >> 3];
    return num_bytes;
}

// Given a byte offset into a UTF-8 string, returns the byte offset to the same location in the string
// if it were encoded by UTF-16.
size utf8_to_utf16_offset(s8 str, size utf8_byte_offset)
{
    size utf16_offset = 0;
    size i = 0;

    while (i < utf8_byte_offset && i < str.len)
    {
        u8 c = str.data[i];
        int n = utf8_get_num_bytes(c);

        // UTF-8 characters between 1 and 3 bytes contribute 1 byte in UTF-16
        // 4-byte UTF-8 characters contribute 2 bytes in UTF-16
        if (n == 4)
            utf16_offset += 2;
        else
            utf16_offset += 1;

        i += n;
    }

    return utf16_offset;
}

internal inline s8 s8_alloc(Arena* arena, size len)
{
    s8 result = {0};
    result.data = push_array(arena, len, u8);
    zero_array(result.data, len, u8);
    return result;
}

internal inline char* s8_get_char(Arena* arena, s8 src)
{
    char* dst = push_array(arena, src.len+1, char);
    memcpy(dst, src.data, src.len);
    dst[src.len] = 0;
    return dst;
}

internal inline s8 s8_format(Arena* arena, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    size len = (size)vsnprintf(NULL, 0, format, args);
    va_end(args);

    s8 result = s8_alloc(arena, len);
    result.len = len;

    va_start(args, format);
    vsnprintf((char*)result.data, len+1, format, args);
    va_end(args);

    return result;
}

// Returns a slice of a string s in the range [begin, end]
internal inline s8 s8_slice(s8 s, size begin, size end)
{
    s8 result = {0};
    result.data = s.data + begin;
    result.len = end == begin ? 0 : end - begin + 1;
    return result;
}

internal inline b32 s8_eq(s8 a, s8 b)
{
    if (a.len != b.len)
        return false;

    return memcmp(a.data, b.data, a.len) == 0;
}

internal inline b32 s8_begins_with(s8 s, u8 c)
{
    b32 result = s.data[0] == c;
    return result;
}

internal inline b32 s8_ends_with(s8 s, u8 c)
{
    b32 result = s.data[s.len-1] == c;
    return result;
}

// Returns a slice of a string starting from the beginning and ending just after the first occurrence of c
internal inline s8 s8_until(s8 s, u8 c)
{
    s8 result = s;
    for (size i = 0; i < s.len; ++i)
    {
        if (s.data[i] == c)
            result.len = i-1;
    }

    return result;
}

// IMPORTANT: Ensure that that s has at least max_len bytes allocated before calling this function
internal inline void s8_insert(s8* s, u8 c, size idx, size max_len)
{
    if (idx >= max_len || s->len >= max_len || idx < 0 || max_len <= 0 || idx > s->len) return;

    if (idx == s->len)
        s->data[idx] = c;
    else
    {
        size start = idx;
        size end = s->len;
        for (size i = end; i > start; --i)
            s->data[i] = s->data[i-1];

        s->data[idx] = c;
    }

    ++s->len;
}

internal inline void s8_delete(s8* s, size idx)
{
    if (idx < 0 || idx > s->len) return;

    if (idx == s->len)
        s->data[idx] = 0;
    else
    {
        s->data[idx] = 0;
        size end = s->len;
        size start = idx;
        for (size i = start; i < end; ++i)
            s->data[i] = s->data[i+1];
    }

    --s->len;
}

// Trim leading and trailing whitespace from a string
internal inline s8 s8_trim(s8 s)
{
    size start = 0;
    while (start < s.len && char_is_space(s.data[start]))
        ++start;

    size end = s.len-1;
    while (end > start && char_is_space(s.data[end]))
        --end;

    s8 result = {0};
    result.data = s.data + start;
    result.len = end-start+1;

    return result;
}

// IMPORTANT: Always check right result
internal inline s8_pair s8_split_first(s8 s, u8 delim)
{
    s8_pair result = {0};

    for (size i = 0; i < s.len; ++i)
    {
        if (s.data[i] == delim)
        {
            result.left = s8_slice(s, 0, i-1);
            result.right = s8_slice(s, i+1, s.len-1);
            return result;
        }
    }

    result.left = s;
    return result;
}
