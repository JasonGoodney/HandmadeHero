#ifndef HANDMADE_H
#define HANDMADE_H

//
// NOTE
// HANDMADE_INTERNAL
//  0 - Build for public release
//  1 - Build for developer only
//
// HANDMADE_SLOW
//  0 - No slow code allowed
//  1 - Slow code welcome
//

#include "handmade_platform.h"
#include <stdio.h>
#include <time.h>

global const u32 RENDER_WIDTH  = 960;
global const u32 RENDER_HEIGHT = 540;

struct game_state
{
    f32 player_tilemap_x;
    f32 player_tilemap_y;
    f32 player_x;
    f32 player_y;
};

struct lib_game
{
    b32 is_valid;
    void *lib_handle;
    time_t last_modification_time;
    game_update_and_render_f *update_and_render;
};

struct tilemap
{
    u32 *tiles;
};

struct world
{
    f32 upper_left_x;
    f32 upper_left_y;
    i32 width;          // TileMapCountX
    i32 height;         // TileMapCountY
    i32 tilemap_width;  // CountX
    i32 tilemap_height; // CountY
    f32 tile_width;
    f32 tile_height;
    struct tilemap *tilemaps;
};

struct canonical_position
{
    i32 tilemap_x;
    i32 tilemap_y;
    i32 tile_x;
    i32 tile_y;
    f32 tile_rel_x;
    f32 tile_rel_y;
};

struct raw_position
{
    i32 tilemap_x;
    i32 tilemap_y;
    f32 x;
    f32 y;
};

internal inline f32
normalize_gamepad_axis_input(i16 value, i32 range, i32 mid, i16 dead_zone)
{
    float result = 0;
    if (value < mid - dead_zone)
    {
        result =
            (float)((value + dead_zone - mid) / (float)(range - dead_zone));
    }
    else if (value > mid + dead_zone)
    {

        result = (float)((value - dead_zone - mid) /
                         (float)((range - 1) - dead_zone));
    }
    return result;
}

internal inline void
process_keyboard_key_input(struct game_button_state *new_state, int is_pressed)
{
    ASSERT(new_state->ended_pressed != is_pressed);
    new_state->ended_pressed = is_pressed;
    new_state->half_transition_count++;
}

internal inline void
process_gamepad_button_input(struct game_button_state *old_state,
                             struct game_button_state *new_state,
                             b32 is_pressed)
{
    new_state->ended_pressed = is_pressed;
    new_state->half_transition_count +=
        (new_state->ended_pressed == old_state->ended_pressed) ? 0 : 1;
}

internal inline i32
round_f32_to_i32(f32 value)
{
    return (i32)(value + 0.5f);
}
internal inline u32
round_f32_to_u32(f32 value)
{
    return (u32)(value + 0.5f);
}
internal inline i32
truncate_f32_to_i32(f32 value)
{
    return (i32)value;
}
internal inline u32
truncate_f32_to_u32(f32 value)
{
    return (u32)value;
}
#include <math.h>
internal inline i32
floor_f32_to_i32(f32 value)
{
    return (i32)floorf(value);
}

#endif // HANDMADE_H
