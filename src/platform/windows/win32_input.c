#include "input.h"

#include <windows.h>

void input_process(Window* window)
{
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

            default:
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } break;
        }
    }
}
