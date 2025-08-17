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

struct lib_game
{
    b32 is_valid;
    void *lib_handle;
    time_t last_modification_time;
    game_update_and_render_f *update_and_render;
};

struct tile_chunk
{
    u32 *tiles;
};

struct world
{
    u32 chunk_shift;
    u32 chunk_mask;
    u32 chunk_dim;

    f32 tile_side_meters;
    i32 tile_side_pixels;
    f32 pixels_per_meter;

    i32 tile_chunk_count_x;
    i32 tile_chunk_count_y;

    struct tile_chunk *tile_chunks;
};

struct tile_chunk_position
{
    u32 tile_chunk_x;
    u32 tile_chunk_y;
    u32 rel_tile_x;
    u32 rel_tile_y;
};

struct world_position
{
    u32 abs_tile_x;
    u32 abs_tile_y;
    f32 tile_rel_x;
    f32 tile_rel_y;
};

struct game_state
{
    struct world_position player_pos;
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

#endif // HANDMADE_H
