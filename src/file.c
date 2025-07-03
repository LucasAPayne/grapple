#include "file.h"
#include "win32_base.h"

#include <windows.h>

#include <stdlib.h> // malloc, free

HANDLE file_open_normal_read(char* filename)
{
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return file;
}

size file_get_size(char* filename)
{
    usize file_size = 0;
    LARGE_INTEGER fsize;
    HANDLE file = file_open_normal_read(filename);
    if (file == INVALID_HANDLE_VALUE)
    {
        // TODO(lucas): Log/handle error
        return file_size;
    }

    GetFileSizeEx(file, &fsize);
    file_size = fsize.QuadPart;
    file_close(file);
    return file_size;
}

b32 file_exists(char* filename)
{
    DWORD attrib = GetFileAttributesA(filename);
    b32 result = (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
    return result;
}

void* file_open(char* filename, FileMode mode)
{
    DWORD file_access = 0;
    DWORD file_share = 0;
    DWORD creation_disposition = OPEN_EXISTING;
    if (mode & FileMode_Read)
    {
        file_access |= GENERIC_READ;
        file_share |= FILE_SHARE_READ;
    }
    if (mode & FileMode_Write || mode & FileMode_Append)
    {
        file_access |= GENERIC_WRITE;
        file_share |= FILE_SHARE_WRITE;
    }
    if (mode & FileMode_Append)
    {
        file_access |= FILE_APPEND_DATA;
        file_share |= FILE_SHARE_READ;
    }
    if ((mode & FileMode_Write) && !file_exists(filename))
        creation_disposition = CREATE_NEW;

    HANDLE file = CreateFileA(filename, file_access, file_share, NULL, creation_disposition, FILE_ATTRIBUTE_NORMAL, NULL);
    ASSERT(file != INVALID_HANDLE_VALUE, "CreateFileA failed");
    if (file == INVALID_HANDLE_VALUE)
    {
        win32_error_callback();
        return 0;
    }

    return file;
}

void file_close(void* file_handle)
{
    // TODO(lucas): For any failure to operate on a file, make sure to log the filename.
    BOOL closed = CloseHandle(file_handle);
    ASSERT(closed, "Failed to close file");
    if (closed == FALSE)
    {
        // TODO(lucas): Handle error
        win32_error_callback();
    }
}

char* get_filename(void* file_handle)
{
    ASSERT(file_handle != INVALID_HANDLE_VALUE, "Invalid file handle");
    if (file_handle == INVALID_HANDLE_VALUE)
    {
        // TODO(lucas): Handle error
        return 0;
    }

    DWORD len = GetFinalPathNameByHandleA(file_handle, NULL, 0, FILE_NAME_NORMALIZED);
    ASSERT(len, "Failed to get file name");
    if (len == 0)
    {
        return 0;
    }

    // TODO(lucas): Replace malloc
    char* filename = malloc(len+1);
    ASSERT(filename, "Failed to allocate memory");
    if (!filename)
        return 0;

    len = GetFinalPathNameByHandleA(file_handle, filename, MAX_PATH, FILE_NAME_NORMALIZED);
    ASSERT(len, "Failed to get file name");
    if (len == 0)
    {
        free(filename);
        return 0;
    }

    return filename;
}

// TODO(lucas): Consider reading from/writing to files >4GB
int file_read(void* file_handle, void* buffer, size num_bytes_to_read)
{
    ASSERT(file_handle, "Invalid file handle");
    DWORD num_bytes_read = 0;
    b32 success = ReadFile(file_handle, buffer, (u32)num_bytes_to_read, &num_bytes_read, NULL);
    if (success == FALSE)
    {
        char* filename = get_filename(file_handle);
        ASSERTF(0, "Failed to read from file %s", filename);
        free(filename);
        win32_error_callback();
    }
    if (num_bytes_read != num_bytes_to_read)
    {
        char* filename = get_filename(file_handle);
        ASSERTF(0, "Number of bytes read (%u) does not match expected number of bytes (%u) in file %s",
                 num_bytes_read, num_bytes_to_read, filename);
        free(filename);
        win32_error_callback();
    }

    return num_bytes_read;
}
