#include "game.h"
#include <stdint.h>

#define GAMEPAD_AXIS_DEADZONE 8000

static void game_update_and_render(struct game_memory *memory,
                                   struct game_back_buffer *buffer,
                                   struct game_input *input)
{
    ASSERT((&input->controllers[0].terminator -
            &input->controllers[0].buttons[0]) ==
           (ARRAY_SIZE(input->controllers[0].buttons)));

    ASSERT(sizeof(struct game_state) <= memory->permanent_size);

    struct game_state *state = (struct game_state *)memory->permanent;

    if (!memory->is_initialized)
    {
        state->x_offset        = 0;
        state->y_offset        = 0;
        state->tone_hz         = 256;
        memory->is_initialized = 1;
    }

    uint8_t *row = (uint8_t *)buffer->data;
    for (int y = 0; y < buffer->height; y++)
    {
        uint32_t *pixel = (uint32_t *)row;
        for (int x = 0; x < buffer->width; x++)
        {
            uint8_t alpha = 255;
            uint8_t red   = 0;
            uint8_t green = y + state->y_offset;
            uint8_t blue  = x + state->x_offset;
            *pixel++      = (alpha << 24 | red << 16 | green << 8 | blue);
        }

        row += buffer->pitch;
    }

    int controller_count = ARRAY_SIZE(input->controllers);
    for (int i = 0; i < controller_count; i++)
    {

        struct game_controller_input *controller = &input->controllers[i];
        if (controller->is_analog)
        {
            state->x_offset -= (int)(2.0f * controller->axis_leftx_average);
            // state->y_offset -= (int)(2.0f * controller->axis_lefty_average);
            state->tone_hz =
                256 + (int)(128.0f * controller->axis_lefty_average);
        }
        else
        {
            if (controller->move_down.ended_pressed)
            {
                state->y_offset -= 1;
            }
            if (controller->move_up.ended_pressed)
            {
                state->y_offset += 1;
            }
            if (controller->move_left.ended_pressed)
            {
                state->x_offset += 1;
            }
            if (controller->move_right.ended_pressed)
            {
                state->x_offset -= 1;
            }
        }
    }
}
