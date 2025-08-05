#ifndef PLATFORM_MACOS_H
#define PLATFORM_MACOS_H

#include "../game.h"

#include <AppKit/NSWindow.h>
#include <AudioToolbox/AudioToolbox.h>
#include <IOKit/IOReturn.h>
#include <IOKit/hid/IOHIDBase.h>
#include <limits.h>

struct macos_debug_time_marker
{
    s32 play_cursor;
    s32 write_cursor;
};

struct macos_audio_output
{
    s16 channels;
    s16 bytes_per_sample;
    s32 sample_rate_khz;
    s32 play_cursor;
    u32 running_sample_index;
    size_t buffer_size;
    void *data;
    AudioComponentInstance *audio_unit;
};

struct macos_recorded_input
{
    s32 input_count;
    struct g_input *input_stream;
};

struct macos_state
{
    uint64_t memory_block_size;
    void *memory_block;

    void *replay_memory_block;
    FILE *replay_file_handle;

    s32 input_recording_index;
    FILE *recording_handle;
    b32 is_recording;

    s32 input_playing_index;
    FILE *playback_handle;
    b32 is_playing_back;
};

// Audio
internal void macos_audio_create(struct macos_audio_output *audio_output);
internal void macos_audio_fill_buffer(struct macos_audio_output *audio_output,
                                      struct game_audio_buffer *audio_buffer,
                                      s32 byte_to_lock, s32 bytes_to_write);

// Device
internal void macos_device_register(void *context);
internal void macos_device_input_callback(void *context, IOReturn result,
                                          void *sender, IOHIDValueRef value);
internal void macos_device_callback(void *context, IOReturn result,
                                    void *sender, IOHIDDeviceRef device);

// Render
internal CGRect macos_get_window_rect(const NSWindow *window);
internal void macos_buffer_resize(struct game_back_buffer *buffer, int width,
                                  int height);
internal void macos_window_display(struct game_back_buffer *buffer,
                                   const NSWindow *window);

#endif // PLATFORM_MACOS_H
