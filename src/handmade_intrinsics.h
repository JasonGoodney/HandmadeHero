#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

#include "handmade_platform.h"
#include <math.h>

// TODO: Change to using our own implementation
// indstead of the C runtime libaries.

static inline i32
round_f32_to_i32(f32 value)
{
    i32 result = (i32)roundf(value);
    return result;
}

static inline u32
round_f32_to_u32(f32 value)
{
    u32 result = (u32)roundf(value);
    return result;
}

static inline i32
truncate_f32_to_i32(f32 value)
{
    i32 result = (i32)value;
    return result;
}

static inline u32
truncate_f32_to_u32(f32 value)
{
    u32 result = (u32)value;
    return result;
}

static inline i32
floor_f32_to_i32(f32 value)
{
    i32 result = (i32)floorf(value);
    return result;
}

static inline f32
sin_f32(f32 value)
{
    f32 result = sinf(value);
    return result;
}

static inline f32
cos_f32(f32 value)
{
    f32 result = cosf(value);
    return result;
}

static inline f32
atan2_f32(f32 y, f32 x)
{
    f32 result = atan2f(y, x);
    return result;
}

typedef struct bit_scan_result
{
    b32 found;
    u32 index;
} BitScanResult;
static inline BitScanResult
bit_scan_least_signficant_bit(u32 value)
{
    BitScanResult result = {0};

#if COMPILER_MSVC
    result.found = _BitMaskForward((unsigned long *)&result.index);
#else
    for (u32 test = 0; test < 32; test++)
    {
        if (value & (1 << test))
        {
            result.index = test;
            result.found = 1;
            break;
        }
    }
#endif

    return result;
}

#endif // HANDMADE_INTRINSICS_H
