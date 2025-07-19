#include "types.h"
#include "window.h"
#include "win32_base.h"

#include <windows.h>

global HICON global_window_icon;

internal inline i64 win32_get_ticks(void)
{
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    return ticks.QuadPart;
}

f32 get_frame_seconds(Window* window)
{
    i64 start_ticks = window->prev_frame_ticks;
    i64 end_ticks = win32_get_ticks();
    i64 microseconds_elapsed = (end_ticks - start_ticks);

	// We now have the elapsed number of ticks, along with the number of ticks-per-second. We use these values
	// to convert to the number of elapsed microseconds. To guard against loss-of-precision,
    // we convert to microseconds *before* dividing by ticks-per-second.
	microseconds_elapsed *= 1000000;
	microseconds_elapsed /= window->ticks_per_second;

    f32 seconds_elapsed = (f32)microseconds_elapsed / 1000000.0f;
    if (seconds_elapsed < 0.0f)
        seconds_elapsed = 0.0f;

    window->prev_frame_ticks = win32_get_ticks();
    return seconds_elapsed;
}

internal LRESULT CALLBACK win32_main_window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;

    switch(msg)
    {
        /*
        Sent when the window/application should close (e.g., user clicks X button).
        */
        case WM_CLOSE:
        {
            Window* window = (Window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
            if (window)
                window->open = false;

            DestroyWindow(hwnd);
        } break;

        /*
        Sent when a window is being destroyed.
        */
        case WM_DESTROY:
        {
            Window* window = (Window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
            if (window)
                window->open = false;

            PostQuitMessage(0);
        } break;

        case WM_MENUCHAR:
        {
            // Don't chime when Alt+Enter is pressed
            result = MAKELRESULT(0, MNC_CLOSE);
        } break;

        // case WM_SYSKEYDOWN:
        // case WM_SYSKEYUP:
        // case WM_KEYDOWN:
        // {
        //     ASSERT(0, "Keyboard input came in through a non-dispatch message!");
        // } break;

        /*
        All message types that are not explicitly handled will end up here. DefWindowProc just provides default
        processing. Having a default case ensures that every Windows message gets processed.
        */
        default:
        {
            result = DefWindowProcA(hwnd, msg, wparam, lparam);
        } break;
    }

    return result;
}

Window* window_create(const char* title, int width, int height)
{
    Window* window = (Window*)VirtualAllocEx(GetCurrentProcess(), NULL, sizeof(Window), MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

    window->width = width;
    window->height = height;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    window->ticks_per_second = frequency.QuadPart;

    // Open a window
    HINSTANCE instance = GetModuleHandleA(0);
    WNDCLASSEXA window_class = {0};
    window_class.cbSize = sizeof(WNDCLASSEXA);
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    window_class.lpfnWndProc = &win32_main_window_callback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "GrappleWindow";
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    window_class.hIcon = LoadIconA(0, IDI_APPLICATION);

    if (!RegisterClassExA(&window_class))
        win32_error_callback();

    RECT initial_window_rect = {0, 0, width, height};
    if (!AdjustWindowRectEx(&initial_window_rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW))
        win32_error_callback();

    LONG initial_window_width = initial_window_rect.right - initial_window_rect.left;
    LONG initial_window_height = initial_window_rect.bottom - initial_window_rect.top;

    HWND hwnd = CreateWindowExA(
        WS_EX_OVERLAPPEDWINDOW, window_class.lpszClassName, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, initial_window_width,  initial_window_height, 0, 0, instance, 0
    );

    if(!hwnd)
        win32_error_callback();

    window->ptr = hwnd;
    window->open = true;

    // Associate window data with the window ptr
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)window);

    return window;
}

void* window_icon_load_from_file(const char* filename)
{
    HICON icon = (HICON)LoadImageA(NULL, filename, IMAGE_ICON, 0, 0, LR_LOADFROMFILE|LR_DEFAULTSIZE);
    return icon;
}

void window_icon_set_from_memory(Window* window, void* icon)
{
    HWND hwnd = (HWND)window->ptr;
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
}

void window_icon_set_from_resource(int id)
{
    global_window_icon = (HICON)LoadImageA(GetModuleHandleA(0), MAKEINTRESOURCEA(id), IMAGE_ICON,
                                           0, 0, LR_DEFAULTSIZE|LR_SHARED);
}
