#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

#include "handmade_platform.h"
#include <math.h>

static inline i32
round_f32_to_i32(f32 value)
{
    i32 result = (i32)(value + 0.5f);
    return result;
}

static inline u32
round_f32_to_u32(f32 value)
{
    u32 result = (u32)(value + 0.5f);
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

#endif // HANDMADE_MATH_H
