#pragma once

#include <windows.h>
#include <strsafe.h>

#ifdef GRAPPLE_DEBUG
    #if defined(GRAPPLE_WIN32)
        #define ASSERT(expr, msg) \
            if(!(expr)) \
            { \
                char assert_buf__[512]; \
                if (FAILED(StringCchPrintfA(assert_buf__, sizeof(assert_buf__), msg))) \
                    assert_buf__[0] = '\0'; \
                MessageBoxA(NULL, assert_buf__, "Assertion Failed", MB_OK | MB_ICONERROR); \
                ExitProcess(1); \
            }
        #define ASSERTF(expr, msg, ...) \
            if(!(expr)) \
            { \
                char assert_buf__[512]; \
                if (FAILED(StringCchPrintfA(assert_buf__, sizeof(assert_buf__), msg, ##__VA_ARGS__))) \
                    assert_buf__[0] = '\0'; \
                MessageBoxA(NULL, assert_buf__, "Assertion Failed", MB_OK | MB_ICONERROR); \
                ExitProcess(1); \
            }
    #else
        #define ASSERT(expr, msg) if(!(expr)) {*(int *)0 = 0;}
        #define ASSERTF(expr, msg, ...) if(!(expr)) {*(int *)0 = 0;}
    #endif
#else
    #define ASSERT(expr, msg)
    #define ASSERTF(expr, msg, ...)
#endif

// NOTE(lucas): Only call after functions whose errors can be retrieved via GetLastError, not those that return HRESULT.
#ifdef GRAPPLE_DEBUG
    #define win32_error_callback() do                                                                           \
    {                                                                                                           \
        DWORD win32_err = GetLastError();                                                                       \
        char win32_msg[512] = "Unknown error";                                                                  \
        char win32_full_buf[1024];                                                                              \
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, win32_err,             \
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), win32_msg, 0, NULL);                             \
        StringCchPrintfA(win32_full_buf, sizeof(win32_full_buf), "Win32 call failed near %s:%d\n\nMessage: %s", \
                        __FILE__, __LINE__, win32_msg);                                                         \
        MessageBoxA(NULL, win32_full_buf, TEXT("Error"), MB_ICONERROR);                                         \
    } while(0)
#else
    #define win32_error_callback() ((void)0);
#endif

#ifdef GRAPPLE_DEBUG
	#define HR(x) do                                                                                             \
	{                                                                                                            \
		HRESULT hr_hr = (x);                                                                                     \
		if(FAILED(hr_hr))                                                                                        \
		{                                                                                                        \
            char hr_msg[512] = "Unknown error";                                                                  \
            char hr_buf[1024];                                                                                   \
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr_hr, 0, hr_msg,   \
                           sizeof(hr_msg), NULL);                                                                \
            StringCchPrintfA(hr_buf, sizeof(hr_buf), "D3D call failed at %s:%d, HRESULT: 0x%08X\n\nMessage: %s", \
                             __FILE__, __LINE__, hr_hr, hr_msg);                                                 \
			MessageBoxA(NULL, hr_buf, "Direct3D Error", MB_OK | MB_ICONERROR);                                   \
            ExitProcess(1);                                                                                      \
		}                                                                                                        \
	} while(0)
#else
	#define HR(x) (x)
#endif

#define com_release(obj) do                      \
{                                                \
    if(obj){obj->lpVtbl->Release(obj); obj = 0;} \
} while(0)
