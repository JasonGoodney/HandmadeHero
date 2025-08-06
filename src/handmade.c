#include "handmade.h"
#include <math.h>

internal int round_ftoi(float value) { return (int)(value + 0.5f); }

internal void render_rectangle(struct game_back_buffer *buffer,
                               f32 minx,
                               f32 miny,
                               f32 maxx,
                               f32 maxy,
                               uint32_t color)
{
    int min_x = round_ftoi(minx);
    int min_y = round_ftoi(miny);
    int max_x = round_ftoi(maxx);
    int max_y = round_ftoi(maxy);

    if (min_x < 0)
    {
        min_x = 0;
    }
    if (min_y < 0)
    {
        min_y = 0;
    }
    if (max_x > buffer->width)
    {
        max_x = buffer->width;
    }
    if (max_y > buffer->height)
    {
        max_y = buffer->height;
    }

    u8 *row = (u8 *)buffer->data + (min_x * buffer->bytes_per_pixel) +
              (min_y * buffer->pitch);
    for (int y = min_y; y < max_y; y++)
    {
        u32 *pixel = (u32 *)row;
        for (int x = min_x; x < max_x; x++)
        {
            *(u32 *)pixel = color;
            pixel++;
        }
        row += buffer->pitch;
    }
}

// TODO: handle endianess for pixel buffer based on OS
GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    ASSERT(sizeof(struct game_state) <= memory->permanent_size);
    ASSERT((&input->controllers[0].terminator -
            &input->controllers[0].buttons[0]) ==
           (ARRAY_SIZE(input->controllers[0].buttons)));

    struct game_state *game_state = (struct game_state *)memory->permanent;

    if (!memory->is_initialized)
    {
        memory->is_initialized = 1;
    }

    // Input
    int controller_count = ARRAY_SIZE(input->controllers);
    for (int i = 0; i < controller_count; i++)
    {
        struct game_controller_input controller = input->controllers[i];
        if (!controller.is_connected)
            continue;

        if (controller.is_analog_movement)
        {
        }
        else
        {
        }
    }

    render_rectangle(buffer,
                     0.0f,
                     0.0f,
                     (float)buffer->width,
                     (float)buffer->height,
                     0xFF0000FF);
    render_rectangle(buffer, 10, -10, 30, 50, 0xFFFF0000);
    // Audio
#if 0
    if (audio_buffer)
    {
        s32 volume = 3000;
        f32 wave_period =
            (f32)audio_buffer->sample_rate_khz / game_state->tone_hz;
        s16 *sample_out = audio_buffer->samples;

        for (int i = 0; i < audio_buffer->sample_count; i++)
        {
#if 1
            s16 sample = sinf(game_state->t_sine) * volume;
#else
            s16 sample = 0;
#endif
            *sample_out = sample;
            sample_out++;
            *sample_out = sample;
            sample_out++;

            game_state->t_sine += (2.0f * PI_F32 * 1.0f) / (f32)wave_period;
        }
    }
#endif
}
