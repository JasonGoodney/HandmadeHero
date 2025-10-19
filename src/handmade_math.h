#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

#include "handmade_platform.h"

typedef union vec2
{
    struct
    {
        f32 x;
        f32 y;
    };
    f32 e[2];
} vec2;

internal inline vec2 vec2_new(f32 x, f32 y)             { vec2 v = {{x, y}}; return v; }
internal inline vec2 vec2_add(vec2 v, f32 s)            { vec2 c = {{v.x + s, v.y + s}}; return c; }
internal inline vec2 vec2_sub(vec2 v, f32 s)            { vec2 c = {{v.x - s, v.y - s}}; return c; }
internal inline vec2 vec2_mul(vec2 v, f32 s)            { vec2 c = {{v.x * s, v.y * s}}; return c; }
internal inline vec2 vec2_add_vec2(vec2 a, vec2 b)      { vec2 c = {{a.x + b.x, a.y + b.y}}; return c; }
internal inline vec2 vec2_sub_vec2(vec2 a, vec2 b)      { vec2 c = {{a.x - b.x, a.y - b.y}}; return c; }
internal inline vec2 vec2_mul_vec2(vec2 a, vec2 b)      { vec2 c = {{a.x * b.x, a.y * b.y}}; return c; }
internal inline  f32 vec2_dot_vec2(vec2 a, vec2 b)      { f32 c = a.x * b.x + a.y * b.y; return c; }

#endif // HANDMADE_MATH_H
