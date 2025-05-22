#ifndef CORE_H
#define CORE_H

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

enum HatSwitchDirection
{
    HatSwitch_Top         = 0,
    HatSwitch_TopRight    = 1,
    HatSwitch_Right       = 2,
    HatSwitch_BottomRight = 3,
    HatSwitch_Bottom      = 4,
    HatSwitch_BottomLeft  = 5,
    HatSwitch_Left        = 6,
    HatSwitch_TopLeft     = 7,
    HatSwitch_None        = 8
};

struct BackBuffer
{
    u8 *data;
    int width;
    int height;
    int pitch;
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

#endif // CORE_H