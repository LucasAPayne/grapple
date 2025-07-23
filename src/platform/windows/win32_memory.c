#include "grapple_memory.h"

#include <windows.h>

Arena arena_alloc(size bytes)
{
    Arena arena = {0};
    arena.used = 0;
    arena.data = (u8*)VirtualAllocEx(GetCurrentProcess(), NULL, bytes, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (arena.data)
        arena.bytes = bytes;
    return arena;
}
