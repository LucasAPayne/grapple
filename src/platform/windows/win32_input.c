#include "input.h"

#include <stdlib.h> // calloc

#include <windows.h>

void input_process(Window* window, Input* input)
{
    // Clear any char that was entered last frame
    input->current_char = 0;
    input->left_arrow = false;
    input->right_arrow = false;
    input->del = false;

    MSG msg = {0};
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
            case WM_QUIT:
            {
                window->open = false;
                DestroyWindow(window->ptr);
            } break;

            case WM_KEYDOWN:
            {
                UINT vk = LOWORD(msg.wParam);
                switch (vk)
                {
                    case VK_LEFT:   input->left_arrow = true;  break;
                    case VK_RIGHT:  input->right_arrow = true; break;
                    case VK_DELETE: input->del = true;         break;
                    default: break;
                }

                TranslateMessage(&msg);
            } break;

            case WM_CHAR:
            {
                char utf8[4] = {0};
                wchar_t wc = (wchar_t)msg.wParam;
                WideCharToMultiByte(CP_UTF8, 0, &wc, 1, utf8, sizeof(utf8), NULL, NULL);

                input->current_char = bytes_to_u32((u8*)utf8);

            } break;

            default:
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } break;
        }
    }
}

b32 clipboard_write_string(char* text)
{
    // Before clipboard can be written to, it first needs to be opened and emptied
    HWND window = GetActiveWindow();
    if (!OpenClipboard(window))
        return false;
    EmptyClipboard();

    // Allocate global memory for the text
    int string_length = lstrlenA(text);
    HGLOBAL string_handle = GlobalAlloc(GMEM_MOVEABLE, string_length+1);
    if (string_handle == NULL)
    {
        CloseClipboard();
        return false;
    }
    LPSTR string_copy = GlobalLock(string_handle);
    memcpy(string_copy, text, string_length+1);
    string_copy[string_length+1] = '\0';
    GlobalUnlock(string_handle);

    // Write to clipboard and close
    SetClipboardData(CF_TEXT, string_handle);
    CloseClipboard();
    return true;
}

char* clipboard_read_string(void)
{
    char* result = NULL;

    // Before clipboard can be written to, it first needs to be opened and emptied
    // Also, check that the desired format is available
    HWND window = GetActiveWindow();
    if (!IsClipboardFormatAvailable(CF_TEXT))
        return NULL;
    if (!OpenClipboard(window))
        return NULL;

    // Get clipboard data
    HGLOBAL clipboard_handle = GetClipboardData(CF_TEXT);
    LPCSTR clipboard_string = NULL;
    if (clipboard_handle != NULL)
    {
        // Get string from clipboard data
        clipboard_string = GlobalLock(clipboard_handle);
        if (clipboard_string != NULL)
        {
            // If string is not NULL, allocate a new one to return and copy
            usize string_length = lstrlenA(clipboard_string)+1;
            result = calloc(string_length, sizeof(TCHAR));
            memcpy(result, clipboard_string, string_length);
            result[string_length+1] = '\0';

            GlobalUnlock(clipboard_handle);
        }
    }

    CloseClipboard();
    return result;
}
