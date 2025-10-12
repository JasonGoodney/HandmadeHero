#ifndef HANDMADE_PLATFORM_H
#define HANDMADE_PLATFORM_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;
typedef float f32;
typedef double f64;
typedef size_t memory_index;

#define internal static
#define local static
#define global static

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#define MAX_GAMEPADS 4
#define PI_F32 3.14159265359f
#define UNUSED(x) (void)(x)
#define KILOBYTES(value) ((value) * 1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

#if HANDMADE_SLOW
#define ASSERT(Expression)                                                     \
    if (!(Expression))                                                         \
    {                                                                          \
        *(int *)0 = 0;                                                         \
    }
#else
#define ASSERT(Expression)
#endif

internal inline u32 safe_truncate_uint64(u64 value)
{
    ASSERT(value <= 0xFFFFFFFF);
    u32 result = (u32)value;
    return (result);
}

// TODO: Services that the platform layer provides to the game
#if HANDMADE_INTERNAL
struct debug_read_file_result
{
    u32 size;
    void *data;
};

#define DEBUG_PLATFORM_FREE_FILE(func_name) void func_name(void *data)
typedef DEBUG_PLATFORM_FREE_FILE(debug_platform_free_file_f);

#define DEBUG_PLATFORM_READ_FILE(func_name)                                    \
    struct debug_read_file_result func_name(char *path)
typedef DEBUG_PLATFORM_READ_FILE(debug_platform_read_file_f);

#define DEBUG_PLATFORM_WRITE_FILE(func_name)                                   \
    b32 func_name(char *path, u32 size, void *data)
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file_f);

#endif

struct game_back_buffer
{
    int width;
    int height;
    int pitch;
    u8 *data;
    u8 bytes_per_pixel;
};

struct game_audio_buffer
{
    i32 sample_rate_khz;
    i32 sample_count;
    i16 *samples;
};

struct game_button_state
{
    i32 half_transition_count;
    b32 ended_pressed;
};

struct game_controller_input
{
    int is_connected;
    int is_analog_movement;
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
            struct game_button_state shoulder_left;
            struct game_button_state shoulder_right;
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
    float delta_time_for_frame;
};

struct game_memory
{
    b32 is_initialized;

    u64 permanent_size;
    void *permanent;

    u64 transient_size;
    void *transient;

#if HANDMADE_INTERNAL
    debug_platform_free_file_f *debug_platform_free_file;
    debug_platform_read_file_f *debug_platform_read_file;
    debug_platform_write_file_f *debug_platform_write_file;
#endif
};

// Service that the game provides to the platform layer
#define GAME_UPDATE_AND_RENDER(func_name)                                      \
    void func_name(struct game_memory *memory,                                 \
                   struct game_back_buffer *buffer,                            \
                   struct game_audio_buffer *audio_buffer,                     \
                   struct game_input *input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render_f);
GAME_UPDATE_AND_RENDER(stub_game_update_and_render) {}

internal inline struct game_controller_input *
get_controller(struct game_input *input, uint8_t index)
{
    ASSERT(index < ARRAY_SIZE(input->controllers));
    struct game_controller_input *controller = &input->controllers[index];
    return controller;
}

#endif // HANDMADE_PLATFORM_H
