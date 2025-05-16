#define MACOS_H
#ifdef  MACOS_H

#include <AppKit/NSWindow.h>
#include <CoreFoundation/CFDictionary.h>
#include <IOKit/hid/IOHIDLib.h>
#include "../core.h"

internal struct Rectangle macos_get_window_rect(const NSWindow *window);
internal void macos_buffer_resize(struct BackBuffer *buffer, int width, int height);
internal void macos_buffer_display(struct BackBuffer *buffer, const NSWindow *window);
internal void macos_register_device(void *context);
internal void macos_device_input_callback(void *context, IOReturn result, void *sender, IOHIDValueRef value);
internal void macos_device_callback(void *context, IOReturn result, void *sender, IOHIDDeviceRef device);
internal CFDictionaryRef macos_device_matching_dictionary(u32 usagePage, u32 usage);

#endif // MACOS_H