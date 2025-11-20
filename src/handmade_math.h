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

internal inline vec2 new_vec2f32(f32 x, f32 y)    { vec2 v = {.x = x, .y = y};         return v; }
internal inline vec2 add_vec2f32(vec2 a, vec2 b)  { vec2 c = {{a.x + b.x, a.y + b.y}}; return c; }
internal inline vec2 sub_vec2f32(vec2 a, vec2 b)  { vec2 c = {{a.x - b.x, a.y - b.y}}; return c; }
internal inline vec2 mul_vec2f32(vec2 a, vec2 b)  { vec2 c = {{a.x * b.x, a.y * b.y}}; return c; }
internal inline  f32 dot_vec2f32(vec2 a, vec2 b)  {  f32 c = a.x * b.x + a.y * b.y;    return c; }
internal inline vec2 scale_vec2f32(vec2 v, f32 s) { vec2 c = {{v.x * s, v.y * s}};     return c; }

#endif // HANDMADE_MATH_H
