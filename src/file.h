#pragma once

#include "types.h"

typedef enum FileMode
{
    FileMode_Read = (1 << 0),
    FileMode_Write = (1 << 1),
    FileMode_Append = (1 << 2)
} FileMode;

size file_get_size(char* filename);
b32 file_exists(char* filename);
void* file_open(char* filename, FileMode mode); // Opens file with given mode(s) and returns file handle
void file_close(void* file_handle);
int file_read(void* file_handle, void* buffer, size num_bytes_to_read);
