#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#define internal static
#define local static
#define global static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 b32;
typedef float f32;
typedef double f64;

global const u32 RENDER_WIDTH  = 64 * 12;
global const u32 RENDER_HEIGHT = 64 * 8;

#define PI_F32 3.14159265359f

struct Game_BackBuffer
{
    int width;
    int height;
    int pitch;
    u8 *data;
};

struct Game_AudioBuffer
{
    s32 sample_rate_khz;
    s32 sample_count;
    s16 *samples;
};

struct G_GamepadButtonState
{
    s32 half_transition_count;
    b32 ended_pressed;
};

struct G_GamepadInput
{
    b32 is_analog;

    f32 start_x;
    f32 start_y;
    f32 end_x;
    f32 end_y;
    f32 min_x;
    f32 min_y;
    f32 max_x;
    f32 max_y;

    union
    {
        struct G_GamepadButtonState buttons[10];

        struct
        {
            struct G_GamepadButtonState face_top;
            struct G_GamepadButtonState face_bottom;
            struct G_GamepadButtonState face_left;
            struct G_GamepadButtonState face_right;
            struct G_GamepadButtonState dpad_top;
            struct G_GamepadButtonState dpad_bottom;
            struct G_GamepadButtonState dpad_left;
            struct G_GamepadButtonState dpad_right;
            struct G_GamepadButtonState left_shoulder;
            struct G_GamepadButtonState right_shoulder;
        };
    };
};

struct G_Input
{
    struct G_GamepadInput gamepads[4];
};

struct DeviceUsage
{
    u32 usage_id;
    u32 state;
};

struct Gamepad
{
    struct DeviceUsage face_top, face_bottom, face_left, face_right;
    struct DeviceUsage dpad_x, dpad_y;
    struct DeviceUsage should_left, should_right;
    struct DeviceUsage trigger_left, trigger_right;
    struct DeviceUsage analog_stick_left, analog_stick_right;
};

struct Rectangle
{
    int x;
    int y;
    int width;
    int height;
};

// TODO: Services that the platform layer provides to the game

// Service that the game provides to the platform layer
void game_topdate_and_render(struct Game_BackBuffer *buffer,
                             struct Game_AudioBuffer *audio_buffer,
                             struct G_Input *input);

#endif // GAME_H
