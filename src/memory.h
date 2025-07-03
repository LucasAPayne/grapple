#pragma once

#include "types.h"

#include <windows.h>

#define KILOBYTES(value) ((u64)(value)*1024)
#define MEGABYTES(value) ((u64)KILOBYTES(value)*1024)

typedef struct
{
    size bytes;
    size used;
    u8* data;
} Arena;

Arena arena_alloc(size bytes)
{
    Arena arena = {0};
    arena.used = 0;
    arena.data = (u8*)VirtualAllocEx(GetCurrentProcess(), NULL, bytes, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (arena.data)
        arena.bytes = bytes;
    return arena;
}

inline void arena_pop(Arena* arena, size bytes)
{
    arena->used -= bytes;
}

inline void arena_clear(Arena* arena)
{
    arena->used = 0;
}

inline void* push_size_(Arena* arena, size bytes)
{
    ASSERT(bytes <= (arena->bytes - arena->used), "Arena overflow");
    void* result = arena->data + arena->used;
    arena->used += bytes;
    return result;
}

inline void zero_size_(size bytes, void* ptr)
{
    u8* byte = (u8*)ptr;
    while (bytes--)
        *byte++ = 0;
}

// Define macro to cast to correct type and get correct size
#define push_struct(arena, type) (type*)push_size_(arena, sizeof(type))
#define zero_struct(instance) zero_size_(sizeof((instance)), &(instance))
#define push_array(arena, count, type) (type*)push_size_(arena, (count)*sizeof(type))
#define zero_array(first, count, type) zero_size_((count)*sizeof(type), first)
#define push_size(arena, bytes) push_size_(arena, bytes)
