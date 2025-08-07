#include "handmade.h"
#include <math.h>

internal int round_ftoi(float value) { return (int)(value + 0.5f); }
internal uint32_t round_ftou(float value) { return (uint32_t)(value + 0.5f); }

internal void render_rectangle(struct game_back_buffer *buffer,
                               f32 minx,
                               f32 miny,
                               f32 maxx,
                               f32 maxy,
                               f32 r,
                               f32 g,
                               f32 b)
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
    uint32_t color =
        (255 << 24 | round_ftou(b * 255.0f) << 16 |
         round_ftou(g * 255.0f) << 8 | round_ftou(r * 255.0f) << 0);

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
        game_state->player_x   = 60;
        game_state->player_y   = 60 * 5;
    }

    // Input
    int controller_count = ARRAY_SIZE(input->controllers);
    for (int i = 0; i < controller_count; i++)
    {
        struct game_controller_input *controller = &input->controllers[i];
        if (!controller->is_connected)
            continue;

        if (controller->is_analog_movement)
        {
        }
        else
        {
            f32 d_player_x = 0.0f;
            f32 d_player_y = 0.0f;
            if (controller->move_up.ended_pressed)
            {
                d_player_y += -1.0f;
            }
            if (controller->move_down.ended_pressed)
            {
                d_player_y += 1.0f;
            }
            if (controller->move_left.ended_pressed)
            {
                d_player_x += -1.0f;
            }
            if (controller->move_right.ended_pressed)
            {
                d_player_x += 1.0f;
            }
            d_player_x *= 64.0f;
            d_player_y *= 64.0f;
            game_state->player_x += d_player_x * input->delta_time_for_frame;
            game_state->player_y += d_player_y * input->delta_time_for_frame;
        }
    }

    render_rectangle(buffer,
                     0.0f,
                     0.0f,
                     (f32)buffer->width,
                     (f32)buffer->height,
                     1.0f,
                     0.0f,
                     1.0f);

    u32 tile_map[9][17] = {{1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
                           {1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                           {1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1},
                           {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
                           {0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                           {1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1},
                           {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
                           {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                           {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}};

    f32 padding_x   = 0.0f;
    f32 padding_y   = 0.0f;
    f32 tile_width  = 60.0f;
    f32 tile_height = 60.0f;

    for (int row = 0; row < 9; row++)
    {
        for (int col = 0; col < 17; col++)
        {
            u32 tile_id = tile_map[row][col];
            f32 gray    = 0.5f;
            if (tile_id == 1)
            {
                gray = 1.0f;
            }
            f32 min_x = padding_x + (f32)col * tile_width;
            f32 min_y = padding_y + (f32)row * tile_height;
            f32 max_x = min_x + tile_width;
            f32 max_y = min_y + tile_height;
            render_rectangle(
                buffer, min_x, min_y, max_x, max_y, gray, gray, gray);
        }
    }

    f32 player_width  = 0.75f * tile_width;
    f32 player_height = 0.75f * tile_height;
    f32 min_x         = game_state->player_x - (player_width * 0.5f);
    f32 min_y         = game_state->player_y - (player_height);
    f32 max_x         = min_x + (player_width);
    f32 max_y         = min_y + (player_height);
    render_rectangle(buffer, min_x, min_y, max_x, max_y, 0.0f, 1.0f, 1.0f);

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
