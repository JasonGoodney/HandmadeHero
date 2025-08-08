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

internal inline i32 round_f32_to_i32(f32 value) { return (i32)(value + 0.5f); }
internal inline u32 round_f32_to_u32(f32 value) { return (u32)(value + 0.5f); }
internal inline i32 truncate_f32_to_i32(f32 value) { return (i32)value; }
internal inline u32 truncate_f32_to_u32(f32 value) { return (u32)value; }

#endif // HANDMADE_H
