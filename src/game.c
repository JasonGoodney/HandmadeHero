#include "game.h"
#include <math.h>

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
#if HANDMADE_MAC && HANDMADE_INTERNAL
        struct debug_read_file_result result =
            memory->debug_platform_read_file(__FILE__);
        if (result.data)
        {

            memory->debug_platform_write_file("test.out", result.size,
                                              result.data);
            memory->debug_platform_free_file(result.data);
        }
#endif
        game_state->tone_hz = 256;
        game_state->t_sine       = 0.0f;

        game_state->box.width  = 50;
        game_state->box.height = 50;
        game_state->box.x      = (RENDER_WIDTH / 2) - (50 / 2);
        game_state->box.y      = (RENDER_HEIGHT / 2) - (50 / 2);
        game_state->player_x   = (RENDER_WIDTH / 2) - (10 / 2);
        game_state->player_y   = (RENDER_HEIGHT / 2) - (10 / 2);
        game_state->t_jump     = 0.0f;

        memory->is_initialized = 1;
    }

    // Input
    int controller_count = ARRAY_SIZE(input->controllers);
    for (int i = 0; i < controller_count; i++)
    {
        struct game_controller_input controller = input->controllers[i];
        if (!controller.is_connected) continue;

        if (controller.is_analog_movement)
        {
            // game_state->box.x = gamepad.end_x;
            // game_state->box.y = gamepad.end_y;
            game_state->tone_hz =
                256 + (int)(128.0f * controller.axis_lefty_average);
        }
        else
        {
            s32 speed = 4;
            if (controller.move_up.ended_pressed)
            {
                game_state->box.y -= speed;
                game_state->player_y -= speed;
            }
            else if (controller.move_down.ended_pressed)
            {
                game_state->box.y += speed;
                game_state->player_y += speed;
            }
            else if (controller.move_left.ended_pressed)
            {
                game_state->box.x -= speed;
                game_state->player_x -= speed;
            }
            else if (controller.move_right.ended_pressed)
            {
                game_state->box.x += speed;
                game_state->player_x += speed;
            }
        }

        if (game_state->t_jump > 0)
        {
            game_state->player_y +=
                (s32)(10.0f * sinf(PI_F32 * game_state->t_jump));
        }

        if (controller.action_down.ended_pressed)
        {
            game_state->t_jump = 2.0f;
        }
        game_state->t_jump -= 0.033f;
    }

    // Pixels
    u8 *row = buffer->data;
    for (int y = 0; y < buffer->height; y += 1)
    {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer->width; x += 1)
        {
            u8 r   = 0;
            u8 g   = 0;
            u8 b   = 0;
            u8 a   = 255;
            *pixel = (a << 24 | b << 16 | g << 8 | r);
            pixel += 1;
        }
        row += buffer->pitch;
    }

    // Player
    u8 *end_buffer = (u8 *)buffer->data + (buffer->pitch * buffer->height);
    u32 color      = 0xFFFFFFFF;
    for (int x = game_state->player_x; x < game_state->player_x + 10; x++)
    {
        u8 *pixel = (u8 *)buffer->data + (x * buffer->bytes_per_pixel) +
                    (game_state->player_y * buffer->pitch);
        for (int y = game_state->player_y; y < game_state->player_y + 10; y++)
        {
            *(u32 *)pixel = color;
            if (pixel >= buffer->data && pixel < end_buffer)
            {
                pixel += buffer->pitch;
            }
        }
    }

    // Audio
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
}