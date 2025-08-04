#include "game.h"
#include <stdint.h>

#define GAMEPAD_AXIS_DEADZONE 8000

struct game_offscreen_buffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};

struct game_button_state
{
    int half_transition_count;
    int ended_down;
};

struct game_controller_input
{
    int is_connected;
    int is_analog;
    float axis_leftx_average;
    float axis_lefty_average;
    union
    {
        struct game_button_state buttons[12];
        struct
        {
            struct game_button_state move_up;
            struct game_button_state move_down;
            struct game_button_state move_left;
            struct game_button_state move_right;
            struct game_button_state action_up;
            struct game_button_state action_down;
            struct game_button_state action_left;
            struct game_button_state action_right;
            struct game_button_state left_shoulder;
            struct game_button_state right_shoulder;
            struct game_button_state back;
            struct game_button_state start;

            // Note: all buttons must be added above this line
            struct game_button_state terminator;
        };
    };
};

struct game_input
{
    struct game_controller_input controllers[5];
};

struct game_state
{
    int x_offset;
    int y_offset;
    int tone_hz;
};

struct game_memory
{
    uint64_t permanent_size;
    void *permanent;
    uint64_t transient_size;
    void *transient;
    int is_initialized;
};

static void game_update_and_render(struct game_memory *memory,
                                   struct game_offscreen_buffer *buffer,
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

    uint8_t *row = (uint8_t *)buffer->memory;
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
            if (controller->move_down.ended_down)
            {
                state->y_offset -= 1;
            }
            if (controller->move_up.ended_down)
            {
                state->y_offset += 1;
            }
            if (controller->move_left.ended_down)
            {
                state->x_offset += 1;
            }
            if (controller->move_right.ended_down)
            {
                state->x_offset -= 1;
            }
        }
    }
}
