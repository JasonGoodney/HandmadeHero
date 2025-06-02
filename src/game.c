#include "game.h"
#include <math.h>

void render_box(const struct Rectangle *box,
                const struct Game_BackBuffer *buffer)
{
    int size = box->width;

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

            if (x >= box->x && x <= box->x + size)
            {
                if (y >= box->y && y <= box->y + size)
                {
                    r = 255;
                }
            }

            *pixel = (r | g << 8 | b << 16 | a << 24);
            pixel += 1;
        }
        row += buffer->pitch;
    }
}

void game_update_and_render(struct Game_BackBuffer *buffer,
                            struct Rectangle *box,
                            struct Game_AudioBuffer *audio_buffer,
                            s16 frequency)
{
    render_box(box, buffer);

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
}
