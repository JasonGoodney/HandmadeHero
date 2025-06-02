#ifndef PLATFORM_MACOS_H
#define PLATFORM_MACOS_H

#include "../game.h"

#include <AppKit/NSWindow.h>
#include <AudioToolbox/AudioToolbox.h>
#include <IOKit/IOReturn.h>
#include <IOKit/hid/IOHIDBase.h>
#include <limits.h>

struct Macos_AudioOutput
{
    s16 channels;
    s16 bytes_per_sample;
    s32 sample_rate_khz;
    s32 play_cursor;
    u32 running_sample_index;
    size_t buffer_size;
    void *data;
};

// Audio
internal void macos_audio_create(struct Macos_AudioOutput *audio_output,
                                 AudioComponentInstance audio_unit);
internal void macos_audio_fill_buffer(struct Macos_AudioOutput *audio_output,
                                      struct Game_AudioBuffer *audio_buffer,
                                      s32 byte_to_lock, s32 bytes_to_write);

// Device
internal void macos_device_register(void *context);
internal void macos_device_input_callback(void *context, IOReturn result,
                                          void *sender, IOHIDValueRef value);
internal void macos_device_callback(void *context, IOReturn result,
                                    void *sender, IOHIDDeviceRef device);

// Render
internal CGRect macos_get_window_rect(const NSWindow *window);
internal void macos_buffer_resize(struct BackBuffer *buffer, int width,
                                  int height);
internal void macos_buffer_display(struct BackBuffer *buffer,
                                   const NSWindow *window);

#endif // PLATFORM_MACOS_H
