#ifndef GAME_H
#define GAME_H

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

#define internal static
#define local static
#define global static

#include <stdint.h>

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

#if HANDMADE_SLOW
#define ASSERT(Expression)                                                     \
    if (!(Expression))                                                         \
    {                                                                          \
        abort();                                                               \
    }
#else
#define ASSERT(Expression)
#endif

#define UNUSED(x) (void)(x)

internal inline u32 safe_truncate_uint64(u64 value)
{
    ASSERT(value <= 0xFFFFFFFF);
    u32 result = (u32)value;
    return (result);
}

#define KILOBYTES(value) ((value) * 1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

global const u32 RENDER_WIDTH  = 64 * 12;
global const u32 RENDER_HEIGHT = 64 * 8;

#define PI_F32 3.14159265359f

struct DeviceUsage
{
    u32 usage_id;
    s32 state;
};

struct Gamepad
{
    struct DeviceUsage face_top, face_bottom, face_left, face_right;
    struct DeviceUsage dpad_x, dpad_y;
    struct DeviceUsage should_left, should_right;
    struct DeviceUsage trigger_left, trigger_right;
    struct DeviceUsage analog_stick_left_x, analog_stick_left_y;
    struct DeviceUsage analog_stick_right_x, analog_stick_right_y;
};

struct Rectangle
{
    int x;
    int y;
    int width;
    int height;
};

struct G_BackBuffer
{
    int width;
    int height;
    int pitch;
    u8 *data;
};

struct G_AudioBuffer
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

struct G_Memory
{
    b32 is_initialized;

    u64 permenant_size;
    void *permenant;

    u64 transient_size;
    void *transient;
};

struct G_State
{
    s16 frequency_hz;
    struct Rectangle box;
};

struct DEBUG_read_file_result
{
    u32 size;
    void *data;
};

// TODO: Services that the platform layer provides to the game
#if HANDMADE_INTERNAL
internal struct DEBUG_read_file_result DEBUG_platform_read_file(char *path);
internal b32 DEBUG_platform_write_file(char *filename, u32 size, void *data);
internal void DEBUG_platform_free_file_data(void *data);
#endif

// Service that the game provides to the platform layer
void game_update_and_render(struct G_Memory *memory,
                            struct G_BackBuffer *buffer,
                            struct G_AudioBuffer *audio_buffer,
                            struct G_Input *input);

#endif // GAME_H
