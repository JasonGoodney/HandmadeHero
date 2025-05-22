#ifndef PLATFORM_MACOS_H
#define PLATFORM_MACOS_H

#include "../core.h"

#include <AppKit/NSWindow.h>
#include <AudioToolbox/AudioToolbox.h>
#include <IOKit/IOReturn.h>
#include <IOKit/hid/IOHIDBase.h>
#include <limits.h>

struct Macos_AudioOutput
{
    s16 channels;
    s16 bits_per_sample;
    s16 bytes_per_sample;
    s32 sample_rate_khz;
    s32 volume;
    s32 frequency_hz;
    s32 period;
    u32 running_sample_index;
    f32 time_sine;
    AudioComponentInstance audio_unit;
};

struct audio_ring_buffer
{
    size_t size;
    s32 write_cursor;
    s32 play_cursor;
    void *data;
};

// Audio
internal void macos_audio_create(struct Macos_AudioOutput *audio_output);
internal void macos_audio_start(AudioUnit p_audio_unit);
internal void macos_audio_stop(AudioUnit p_audio_unit);

// Device
internal void macos_device_register(void *context);
internal void macos_device_input_callback(void *context, IOReturn result,
                                          void *sender, IOHIDValueRef value);
internal void macos_device_callback(void *context, IOReturn result,
                                    void *sender, IOHIDDeviceRef device);

// Render
void render_weird_gradient(const struct BackBuffer *buffer, int x_offset,
                           int y_offset);
void render_box(const struct Rectangle *box, const struct BackBuffer *buffer);
void macos_render(const struct BackBuffer *buffer);
internal CGRect macos_get_window_rect(const NSWindow *window);
internal void macos_buffer_resize(struct BackBuffer *buffer, int width,
                                  int height);
internal void macos_buffer_display(struct BackBuffer *buffer,
                                   const NSWindow *window);

#endif // PLATFORM_MACOS_H