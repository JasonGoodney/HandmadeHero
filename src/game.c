#include "game.h"

internal u32 safe_truncate_uint64(u64 value)
{
    ASSERT(value <= 0xFFFFFFFF);
    u32 result = (u32)value;
    return (result);
}

// TODO: handle endianesss for pixel buffer based on OS
void game_update_and_render(struct G_Memory *memory,
                            struct G_BackBuffer *buffer,
                            struct G_AudioBuffer *audio_buffer,
                            struct G_Input *input)
{
    ASSERT(sizeof(struct G_State) <= memory->permenant_size);
#if HANDMADE_INTERNAL
    struct DEBUG_read_file_result result = DEBUG_platform_read_file(__FILE__);
    if (result.data)
    {

        DEBUG_platform_write_file("test.out", result.size, result.data);
        DEBUG_platform_free_file_data(result.data);
    }
#endif
    struct G_State *game_state = (struct G_State *)memory;

    if (!memory->is_initialized)
    {
        game_state->frequency_hz = 256;
        game_state->box.width    = 50;
        game_state->box.height   = 50;
        game_state->box.x        = (RENDER_WIDTH / 2) - (50 / 2);
        game_state->box.y        = (RENDER_HEIGHT / 2) - (50 / 2);

        memory->is_initialized = 1;
    }

    // Input
    struct G_GamepadInput gamepad = input->gamepads[0];

    if (gamepad.is_analog)
    {
        game_state->box.x = gamepad.end_x;
        game_state->box.y = gamepad.end_y;
    }

    if (gamepad.dpad_top.ended_pressed)
    {
        game_state->box.y -= 1;
    }
    else if (gamepad.dpad_bottom.ended_pressed)
    {
        game_state->box.y += 1;
    }
    else if (gamepad.dpad_left.ended_pressed)
    {
        game_state->box.x -= 1;
    }
    else if (gamepad.dpad_right.ended_pressed)
    {
        game_state->box.x += 1;
    }

    // Pixels
    int size = game_state->box.width;

    u8 *row = buffer->data;
    for (int y = 0; y < buffer->height; y += 1)
    {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer->width; x += 1)
        {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            u8 a = 255;

            if (x >= game_state->box.x && x <= game_state->box.x + size)
            {
                if (y >= game_state->box.y && y <= game_state->box.y + size)
                {
                    r = 255;
                }
            }

            *pixel = (r | g << 8 | b << 16 | a << 24);
            pixel += 1;
        }
        row += buffer->pitch;
    }

#if 0
    // Audio
    local f32 time_sine = 0.0f;
    s32 volume          = 3000;
    f32 wave_period     = (f32)audio_buffer->sample_rate_khz / game_state->frequency_hz;
    s16 *sample_out     = audio_buffer->samples;

    for (int i = 0; i < audio_buffer->sample_count; i++)
    {
        s16 sample = sinf(time_sine) * volume;

        *sample_out = sample;
        sample_out++;
        *sample_out = sample;
        sample_out++;

        time_sine += (2.0f * PI_F32 * 1.0f) / (f32)wave_period;
    }
#endif
}
