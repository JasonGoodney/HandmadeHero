#include "game.h"
#include <math.h>

// TODO: handle endianesss for pixel buffer based on OS
void game_update_and_render(struct Game_BackBuffer *buffer,
                            struct Game_AudioBuffer *audio_buffer,
                            struct G_Input *input)
{
    local struct Rectangle box = {
        .width  = 50,
        .height = 50,
        .x      = (RENDER_WIDTH / 2) - (50 / 2),
        .y      = (RENDER_HEIGHT / 2) - (50 / 2),
    };

    // Input
    struct G_GamepadInput gamepad = input->gamepads[0];

    if (gamepad.is_analog)
    {
        box.x = gamepad.end_x;
        box.y = gamepad.end_y;
    }

    if (gamepad.dpad_top.ended_pressed)
    {
        box.y -= 1;
    }
    else if (gamepad.dpad_bottom.ended_pressed)
    {
        box.y += 1;
    }
    else if (gamepad.dpad_left.ended_pressed)
    {
        box.x -= 1;
    }
    else if (gamepad.dpad_right.ended_pressed)
    {
        box.x += 1;
    }

    // Pixels
    int size = box.width;

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

            if (x >= box.x && x <= box.x + size)
            {
                if (y >= box.y && y <= box.y + size)
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
    local s16 frequency = 256;
    local f32 time_sine = 0.0f;
    s32 volume          = 3000;
    f32 wave_period     = (f32)audio_buffer->sample_rate_khz / frequency;
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
