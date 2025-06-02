#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#define internal static
#define local static
#define global static

#define PI_F32 3.14159265359f

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

struct BackBuffer
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
void game_update_and_render(struct BackBuffer *buffer, struct Rectangle *box,
                            struct Game_AudioBuffer *audio_buffer, s16 frequency);

#endif // GAME_H
