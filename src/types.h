#pragma once

#include <stdint.h>

#define countof(array) (sizeof((array)) / sizeof((array)[0]))
#define lengthof(array) (countof(array) - 1)

#define u64_high_low(hi, lo) ((u64)(hi) << 32) | (lo)

#define INVALID_CODE_PATH() ASSERT(0, "Invalid Code Path")
#define INVALID_DEFAULT_CASE() default: {INVALID_CODE_PATH();} break

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
