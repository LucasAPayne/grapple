#include "types.h"
#include "window.h"
#include "win32_base.h"

#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define TRAY_MENU_SHOW 1001
#define TRAY_MENU_EXIT 1002

#define HK_OPEN 1

global HICON global_window_icon;

global NOTIFYICONDATAA global_nid;
global HMENU global_tray_menu;

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

void window_show(Window* window)
{
    // Show the window, put it on top, and direct keyboard input to it
    HWND hwnd = window->ptr;
    ShowWindow(hwnd, SW_SHOW);
    SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(hwnd);
}

void window_hide(Window* window)
{
    HWND hwnd = window->ptr;
    ShowWindow(hwnd, SW_HIDE);
}

internal LRESULT CALLBACK win32_main_window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    Window* window = (Window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch(msg)
    {
        /*
        Sent when the window/application should close (e.g., user clicks X button).
        */
        case WM_CLOSE:
        {
            if (window)
                window->open = false;

            DestroyWindow(hwnd);
        } break;

        /*
        Sent when a window is being destroyed.
        */
        case WM_DESTROY:
        {
            if (window)
                window->open = false;

            // Clean up system tray resources
            UnregisterHotKey(hwnd, HK_OPEN);
            Shell_NotifyIconA(NIM_DELETE, &global_nid);
            if (global_tray_menu)
                DestroyMenu(global_tray_menu);

            PostQuitMessage(0);
        } break;

        case WM_MENUCHAR:
        {
            // Don't chime when Alt+Enter is pressed
            result = MAKELRESULT(0, MNC_CLOSE);
        } break;

        case WM_TRAYICON:
        {
            if (LOWORD(lparam) == WM_LBUTTONDBLCLK)
                window_show(window);
            else if (LOWORD(lparam) == WM_RBUTTONUP)
            {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(global_tray_menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                PostMessageA(hwnd, WM_NULL, 0, 0);
            }
        } break;

        case WM_COMMAND:
        {
            if (LOWORD(wparam) == TRAY_MENU_SHOW)
                window_show(window);
            else if (LOWORD(wparam) == TRAY_MENU_EXIT)
                DestroyWindow(hwnd);
        } break;

        case WM_HOTKEY:
        {
            if (wparam == HK_OPEN)
                window_show(window);
        }

        case WM_KILLFOCUS:
        case WM_ACTIVATE:
        {
            // Open the window hidden, and hide it when it loses focus.
            // NOTE(lucas): Calling window_hide here doesn't work, so call ShowWindow expclicitly
            if (LOWORD(lparam) == WA_INACTIVE)
                ShowWindow(hwnd, SW_HIDE);
        } break;

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

    int monitor_width = GetSystemMetrics(SM_CXSCREEN);
    int monitor_height = GetSystemMetrics(SM_CYSCREEN);
    int wnd_x = (monitor_width - width) / 2;
    int wnd_y = (monitor_height - height) / 4;

    // Make the window render on top of everything (topmost) and not appear in the taskbar (toolwindow)
    DWORD ex_style = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    HWND hwnd = CreateWindowExA(ex_style, window_class.lpszClassName, title, WS_VISIBLE | WS_POPUP,
        wnd_x, wnd_y, width, height, 0, 0, instance, 0);

    if(!hwnd)
        win32_error_callback();

    window->ptr = hwnd;
    window->open = true;

    // Associate window data with the window ptr
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)window);

    // Set up system tray icon and pop-up menu
    global_nid.cbSize = sizeof(NOTIFYICONDATAA);
    global_nid.hWnd = hwnd;
    global_nid.uID = 1;
    global_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    global_nid.uCallbackMessage = WM_TRAYICON;
    global_nid.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    lstrcpyA(global_nid.szTip, "Grapple");

    Shell_NotifyIconA(NIM_ADD, &global_nid);

    global_tray_menu =  CreatePopupMenu();
    AppendMenuA(global_tray_menu, MF_STRING, TRAY_MENU_SHOW, "Show");
    AppendMenuA(global_tray_menu, MF_STRING, TRAY_MENU_EXIT, "Exit");

    RegisterHotKey(hwnd, HK_OPEN, MOD_CONTROL | MOD_SHIFT, ' ');

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

void open_vs_code(char* proj_path)
{
    HINSTANCE result = ShellExecuteA(NULL, "open", "code", proj_path, proj_path, SW_HIDE);
    if ((INT_PTR)result <= 32)
        win32_error_callback();
}
