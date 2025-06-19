#pragma once

#include <stdint.h>

#define countof(array) (sizeof((array)) / sizeof((array)[0]))
#define lengthof(array) (countof(array) - 1)

#define u64_high_low(hi, lo) ((u64)(hi) << 32) | (lo)

#define INVALID_CODE_PATH() ASSERT(0, "Invalid Code Path")
#define INVALID_DEFAULT_CASE() default: {INVALID_CODE_PATH();} break

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

#define internal static
#define persist  static
#define global   static

#define true  1
#define false 0

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef ptrdiff_t size;
typedef size_t usize;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef int32_t b32;

typedef float f32;
typedef double f64;
