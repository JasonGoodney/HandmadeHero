#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDLib.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#include "../core.h"
#include "macos.h"

#include <math.h>

global const u16 RENDER_WIDTH   = 64 * 12;
global const u16 RENDER_HEIGHT  = 64 * 8;
global const u8 BYTES_PER_PIXEL = 4;

global BOOL RUNNING;
global struct BackBuffer global_backbuffer;
global struct Rectangle box = {
    .width  = 50,
    .height = 50,
    .x      = (RENDER_WIDTH / 2) - (50 / 2),
    .y      = (RENDER_HEIGHT / 2) - (50 / 2),
};

@interface HandmadeWindowDelegate : NSObject <NSWindowDelegate>
;
@end

@implementation HandmadeWindowDelegate
;
- (void)windowWillClose:(NSNotification *)notification
{
    RUNNING = NO;
}

- (void)windowDidResize:(NSNotification *)notification
{
    NSWindow *window = (NSWindow *)notification.object;
    CGRect rect      = macos_get_window_rect(window);
    macos_buffer_resize(&global_backbuffer, rect.size.width, rect.size.height);
    // render_weird_gradient(&global_backbuffer, x_offset, y_offset);
    macos_render(&global_backbuffer);
    macos_buffer_display(&global_backbuffer, window);

    NSString *title =
        [NSString stringWithFormat:@"Handmade Hero (%x%d)",
                                   (int)rect.size.width, (int)rect.size.height];
    [window setTitle:title];
}
@end // HandmadeWindowDelegate

int main(int argc, const char *argv[])
{
    NSApplication *app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
    [app activateIgnoringOtherApps:YES];

    struct Gamepad gamepad = {};
    macos_device_register(&gamepad);

    struct Macos_AudioOutput audio_output = {};
    AudioComponentInstance audio_unit     = NULL;
    macos_audio_create(&audio_output, audio_unit);

    NSRect screenRect = [[NSScreen mainScreen] frame];

    NSRect contentRect =
        NSMakeRect((screenRect.size.width - RENDER_WIDTH) * 0.5,
                   (screenRect.size.height - RENDER_HEIGHT) * 0.5, RENDER_WIDTH,
                   RENDER_HEIGHT);

    NSWindowStyleMask styleMask =
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

    NSWindow *window =
        [[NSWindow alloc] initWithContentRect:contentRect
                                    styleMask:styleMask
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    [window setBackgroundColor:NSColor.blackColor];
    [window makeKeyAndOrderFront:NULL];
    [window setAcceptsMouseMovedEvents:true];

    window.contentView.wantsLayer = YES;

    HandmadeWindowDelegate *windowDelegate =
        [[HandmadeWindowDelegate alloc] init];
    [window setDelegate:windowDelegate];

    CGRect rect = macos_get_window_rect(window);
    macos_buffer_resize(&global_backbuffer, rect.size.width, rect.size.height);
    NSString *title = [NSString stringWithFormat:@"Handmade Hero (%dx%d)",
                                                 global_backbuffer.width,
                                                 global_backbuffer.height];
    [window setTitle:title];

    RUNNING = true;
    struct mach_timebase_info timebase_info;
    mach_timebase_info(&timebase_info);
    u64 last_clock_tick = mach_absolute_time();

    while (RUNNING)
    {
        // Event handling
        NSEvent *event;
        do
        {
            event = [app nextEventMatchingMask:NSEventMaskAny
                                     untilDate:nil
                                        inMode:NSDefaultRunLoopMode
                                       dequeue:YES];

            switch ([event type])
            {
            case NSEventTypeKeyDown:
                printf("key down hex %x\n", event.keyCode);
                if (event.keyCode == 0x35) // Escape
                {
                    RUNNING = false;
                }
                if (event.keyCode == 0x31)
                {
                    // AudioOutputUnitStart(audio_output.audio_unit);
                }
                else if (event.keyCode == 0x7e)
                {
                    audio_output.frequency_hz += 20;
                    audio_output.period = audio_output.sample_rate_khz /
                                          audio_output.frequency_hz;
                }

                if (event.keyCode == 0x0d) // W
                {
                    gamepad.dpad_y.state = -1;
                }
                else if (event.keyCode == 0x00) // A
                {
                    gamepad.dpad_x.state = -1;
                }
                else if (event.keyCode == 0x01) // S
                {
                    gamepad.dpad_y.state = 1;
                }
                else if (event.keyCode == 0x02) // D
                {
                    gamepad.dpad_x.state = 1;
                }

                break;
            case NSEventTypeKeyUp:
                if (event.keyCode == 0x31)
                {
                    // AudioOutputUnitStop(audio_output.audio_unit);
                }
                else if (event.keyCode == 0x7d)
                {
                    audio_output.frequency_hz -= 20;
                    audio_output.period = audio_output.sample_rate_khz /
                                          audio_output.frequency_hz;
                }
                if (event.keyCode == 0x0d || event.keyCode == 0x01)
                {
                    gamepad.dpad_y.state = 0;
                }
                else if (event.keyCode == 0x00 || event.keyCode == 0x02)
                {
                    gamepad.dpad_x.state = 0;
                }

                break;
            default:
                [app sendEvent:event];
            }
        } while (event != nil);

        box.x += gamepad.dpad_x.state;
        box.y += gamepad.dpad_y.state;

        // Render
        macos_render(&global_backbuffer);
        macos_buffer_display(&global_backbuffer, window);

        // Sound output
        // write to circular buffer
        s32 latency_sample_count = audio_output.sample_rate_khz / 15;

        s32 target_queue_bytes =
            latency_sample_count * audio_output.bytes_per_sample;
        u32 target_cursor = (audio_output.play_cursor + target_queue_bytes) %
                            audio_output.buffer_size;

        s32 byte_to_lock = (audio_output.running_sample_index *
                            audio_output.bytes_per_sample) %
                           audio_output.buffer_size;
        s32 bytes_to_write;

        if (byte_to_lock > target_cursor)
        {
            // Play cursor wrapped
            // Bytes to end of the circular buffer
            bytes_to_write = audio_output.buffer_size - byte_to_lock;

            // Bytes up to target cursor
            bytes_to_write += target_cursor;
        }
        else
        {
            bytes_to_write = target_cursor - byte_to_lock;
        }

        macos_audio_fill_buffer(&audio_output, byte_to_lock, bytes_to_write);

        u64 end_clock_tick     = mach_absolute_time();
        u64 elapsed_clock_tick = end_clock_tick - last_clock_tick;
        u64 elapsed_time_ns =
            elapsed_clock_tick * timebase_info.numer / timebase_info.denom;
        f32 elapsed_time_s = (f32)elapsed_time_ns * 1.0E-9;
        f32 fps            = 1.f / elapsed_time_s;

        NSLog(@"frames/second %.02ffps", fps);
        last_clock_tick = end_clock_tick;
    }

    printf("Handmade Hero finished running.\n");

    return 0;
}

internal CGRect macos_get_window_rect(const NSWindow *window)
{
    return window.contentView.bounds;
}

internal void macos_buffer_display(struct BackBuffer *buffer,
                                   const NSWindow *window)
{

    @autoreleasepool
    {
        NSBitmapImageRep *imageRep = [[[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:&buffer->data
                          pixelsWide:buffer->width
                          pixelsHigh:buffer->height
                       bitsPerSample:8
                     samplesPerPixel:BYTES_PER_PIXEL
                            hasAlpha:YES
                            isPlanar:NO
                      colorSpaceName:NSDeviceRGBColorSpace
                         bytesPerRow:buffer->pitch
                        bitsPerPixel:32] autorelease];

        NSSize imageSize = NSMakeSize(buffer->width, buffer->height);
        NSImage *image = [[[NSImage alloc] initWithSize:imageSize] autorelease];
        [image addRepresentation:imageRep];

        window.contentView.layer.contents = image;
    }
}

internal void macos_buffer_resize(struct BackBuffer *buffer, int width,
                                  int height)
{
    if (buffer->data)
    {
        free(buffer->data);
        buffer->data = NULL;
    }

    buffer->width  = width;
    buffer->height = height;
    buffer->pitch  = width * BYTES_PER_PIXEL;
    buffer->data   = (u8 *)malloc(buffer->pitch * height);
}

void macos_render(const struct BackBuffer *buffer)
{
    // render_weird_gradient(buffer, x_offset, y_offset);
    render_box(&box, buffer);
}

void render_weird_gradient(const struct BackBuffer *buffer, int x_offset,
                           int y_offset)
{
    u8 *row = buffer->data;
    for (int y = 0; y < buffer->height; ++y)
    {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer->width; ++x)
        {
            u8 r   = 0;
            u8 g   = (u8)(y + y_offset);
            u8 b   = (u8)(x + x_offset);
            u8 a   = 255;
            *pixel = (r | g << 8 | b << 16 | a << 24);
            pixel += 1;
        }
        row += buffer->pitch;
    }
}

void render_box(const struct Rectangle *box, const struct BackBuffer *buffer)
{

    int size = box->width;

    u8 *row = buffer->data;
    for (int y = 0; y < buffer->height; y += 1)
    {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer->width; x += 1)
        {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            u8 a = 255;

            if (x >= box->x && x <= box->x + size)
            {
                if (y >= box->y && y <= box->y + size)
                {
                    r = 255;
                }
            }

            *pixel = (r | g << 8 | b << 16 | a << 24);
            pixel += 1;
        }
        row += buffer->pitch;
    }
}

internal void macos_device_input_callback(void *context, IOReturn result,
                                          void *sender, IOHIDValueRef value)
{
    if (result != kIOReturnSuccess)
    {
        printf("Error device input callback:\n");
        return;
    }

    IOHIDElementRef element = IOHIDValueGetElement(value);
    u32 usagePage           = IOHIDElementGetUsagePage(element);
    u32 usage               = IOHIDElementGetUsage(element);
    s32 state               = (s32)IOHIDValueGetIntegerValue(value);

    struct Gamepad *gamepad = (struct Gamepad *)context;

    if (usagePage == kHIDPage_Button)
    {
        if (usage == gamepad->face_bottom.usage_id)
        {
            gamepad->face_bottom.state = state;
        }
        else if (usage == gamepad->face_top.usage_id)
        {
            gamepad->face_top.state = state;
        }
        else if (usage == gamepad->face_right.usage_id)
        {
            gamepad->face_right.state = state;
        }
        else if (usage == gamepad->face_left.usage_id)
        {
            gamepad->face_left.state = state;
        }
    }
    else if (usagePage == kHIDPage_GenericDesktop)
    {
        // s64 analog =
        //     IOHIDValueGetScaledValue(value, kIOHIDValueScaleTypeCalibrated);
        // printf("analog, %ld\n", analog);
        if (usage == kHIDUsage_GD_Hatswitch)
        {
            if (state == 0)
            {
                gamepad->dpad_x.state = 0;
                gamepad->dpad_y.state = -1;
            }
            else if (state == 1)
            {
                gamepad->dpad_x.state = 1;
                gamepad->dpad_y.state = -1;
            }
            else if (state == 2)
            {
                gamepad->dpad_x.state = 1;
                gamepad->dpad_y.state = 0;
            }
            else if (state == 3)
            {
                gamepad->dpad_x.state = 1;
                gamepad->dpad_y.state = 1;
            }
            else if (state == 4)
            {
                gamepad->dpad_x.state = 0;
                gamepad->dpad_y.state = 1;
            }
            else if (state == 5)
            {
                gamepad->dpad_x.state = -1;
                gamepad->dpad_y.state = 1;
            }
            else if (state == 6)
            {
                gamepad->dpad_x.state = -1;
                gamepad->dpad_y.state = 0;
            }
            else if (state == 7)
            {
                gamepad->dpad_x.state = -1;
                gamepad->dpad_y.state = -1;
            }
            else
            {
                gamepad->dpad_x.state = 0;
                gamepad->dpad_y.state = 0;
            }
        }
    }
}

internal void macos_device_callback(void *context, IOReturn result,
                                    void *sender, IOHIDDeviceRef device)
{
    if (result != kIOReturnSuccess)
    {
        return;
    }

    s32 vendorID  = 0;
    s32 productID = 0;

    {
        CFTypeRef ref =
            IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
        if (ref)
        {
            if (CFGetTypeID(ref) == CFNumberGetTypeID())
            {
                CFNumberGetValue((CFNumberRef)ref, kCFNumberSInt32Type,
                                 &vendorID);
            }
        }
    }

    {
        CFTypeRef ref =
            IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));
        if (ref)
        {
            if (CFGetTypeID(ref) == CFNumberGetTypeID())
            {
                CFNumberGetValue((CFNumberRef)ref, kCFNumberSInt32Type,
                                 &productID);
            }
        }
    }

    struct Gamepad *gamepad = (struct Gamepad *)context;

    if (vendorID == 0x054C && productID == 0x09CC)
    {
        printf("Sony Dualshock 4 detected\n");
        gamepad->face_left.usage_id   = 0x01;
        gamepad->face_bottom.usage_id = 0x02;
        gamepad->face_right.usage_id  = 0x03;
        gamepad->face_top.usage_id    = 0x04;
        gamepad->dpad_x.usage_id      = 0x39;
        gamepad->dpad_y.usage_id      = 0x39;
    }
    else if (vendorID == 0x57e && (productID == 0x2006 || productID == 0x2007))
    {
        printf("Nintendo Joy-Con (%c) detected\n",
               productID == 0x2006 ? 'L' : 'R');

        gamepad->face_bottom.usage_id = 0x01;
        gamepad->face_right.usage_id  = 0x02;
        gamepad->face_left.usage_id   = 0x03;
        gamepad->face_top.usage_id    = 0x04;
        gamepad->dpad_x.usage_id      = 0x39;
        gamepad->dpad_y.usage_id      = 0x39;
    }
    else
    {
        printf("vendorID: %x, productID: %x\n", vendorID, productID);
    }

    IOHIDDeviceSetInputValueMatchingMultiple(device, (__bridge CFArrayRef) @[
        @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_Button)},
        @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_GenericDesktop)},
        @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_KeyboardOrKeypad)}
    ]);

    IOHIDDeviceRegisterInputValueCallback(device, macos_device_input_callback,
                                          context);
}

internal void macos_device_register(void *context)
{
    IOHIDManagerRef manager =
        IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

    IOHIDManagerSetDeviceMatchingMultiple(manager, (__bridge CFArrayRef) @[
        @{
            @kIOHIDDeviceUsagePageKey : @(kHIDPage_GenericDesktop),
            @kIOHIDDeviceUsageKey : @(kHIDUsage_GD_Joystick)
        },
        @{
            @kIOHIDDeviceUsagePageKey : @(kHIDPage_GenericDesktop),
            @kIOHIDDeviceUsageKey : @(kHIDUsage_GD_GamePad)
        },
        @{
            @kIOHIDDeviceUsagePageKey : @(kHIDPage_GenericDesktop),
            @kIOHIDDeviceUsageKey : @(kHIDUsage_GD_MultiAxisController)
        },
        @{
            @kIOHIDDeviceUsagePageKey : @(kHIDPage_GenericDesktop),
            @kIOHIDDeviceUsageKey : @(kHIDUsage_GD_Keyboard)
        }
    ]);

    IOHIDManagerRegisterDeviceMatchingCallback(manager, macos_device_callback,
                                               context);

    IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(),
                                    kCFRunLoopDefaultMode);

    IOReturn ioReturn = IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
    if (ioReturn != kIOReturnSuccess)
    {
        printf("an error occurred opening IOHIDManagerOpen\n");
        return;
    }
}

internal OSStatus macos_audio_render_callback(
    void *int_ref_con, enum AudioUnitRenderActionFlags *io_action_flags,
    const struct AudioTimeStamp *in_timestamp, unsigned int in_bus_number,
    unsigned int in_number_frames, struct AudioBufferList *io_data)
{
    // Read from circular buffer
    struct Macos_AudioOutput *audio_output =
        (struct Macos_AudioOutput *)int_ref_con;

    u32 bytes_to_output = in_number_frames * audio_output->bytes_per_sample;

    // Region 1 is the number of bytes up to the end of the buffer. If the
    // frames to be rendered causes us to wrap, the remained goes into region 2.
    u32 region_1_size = bytes_to_output;
    u32 region_2_size = 0;

    if (audio_output->play_cursor + region_1_size > audio_output->buffer_size)
    {
        // When we wrap over the buffer size
        region_1_size = audio_output->buffer_size - audio_output->play_cursor;
        region_2_size = bytes_to_output - region_1_size;
    }

    // TODO: assert region_1_size and region_2_size are valid (multiple of 4
    // byte sample size)

    u8 *data = (u8 *)io_data->mBuffers[0].mData;
    memcpy(data, (u8 *)audio_output->data + audio_output->play_cursor,
           region_1_size);
    memcpy(&data[region_1_size], (u8 *)audio_output->data, region_2_size);

    audio_output->play_cursor = (audio_output->play_cursor + bytes_to_output) %
                                audio_output->buffer_size;

    return noErr;
}

internal void macos_audio_create(struct Macos_AudioOutput *audio_output,
                                 AudioComponentInstance audio_unit)
{
    audio_output->channels         = 2;
    audio_output->sample_rate_khz  = 48000;
    audio_output->volume           = 3000;
    audio_output->frequency_hz     = 256;
    audio_output->bytes_per_sample = sizeof(s16) * 2;
    audio_output->period =
        audio_output->sample_rate_khz / audio_output->frequency_hz;
    audio_output->running_sample_index = 0;

    // Allocates a 2 second buffer
    audio_output->buffer_size =
        2 * audio_output->sample_rate_khz * audio_output->bytes_per_sample;
    audio_output->data        = malloc(audio_output->buffer_size);
    audio_output->play_cursor = 0;

    // Configure the search parameters to find the default playback output unit
    AudioComponentDescription output_desc;
    output_desc.componentType         = kAudioUnitType_Output;
    output_desc.componentSubType      = kAudioUnitSubType_DefaultOutput;
    output_desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    output_desc.componentFlags        = 0;
    output_desc.componentFlagsMask    = 0;

    // Get the default playback output unit
    AudioComponent output = AudioComponentFindNext(NULL, &output_desc);
    assert(output);

    // Create a new unit
    s32 status = AudioComponentInstanceNew(output, &audio_unit);
    assert(audio_unit);

    // Set the format to 16 bit, dual channel, signed integer, linear PCM
    AudioStreamBasicDescription stream_format;
    stream_format.mFormatID         = kAudioFormatLinearPCM;
    stream_format.mSampleRate       = audio_output->sample_rate_khz;
    stream_format.mFramesPerPacket  = 1;
    stream_format.mBytesPerFrame    = audio_output->bytes_per_sample;
    stream_format.mBytesPerPacket   = audio_output->bytes_per_sample;
    stream_format.mChannelsPerFrame = audio_output->channels;
    stream_format.mBitsPerChannel   = sizeof(s16) * 8;
    stream_format.mFormatFlags =
        kAudioFormatFlagIsPacked | kAudioFormatFlagIsSignedInteger;

    status = AudioUnitSetProperty(audio_unit, kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 0, &stream_format,
                                  sizeof(AudioStreamBasicDescription));
    assert(status == 0);

    // Set out one rendering function on the unit
    AURenderCallbackStruct input;
    input.inputProc       = macos_audio_render_callback;
    input.inputProcRefCon = audio_output;

    status = AudioUnitSetProperty(
        audio_unit, kAudioUnitProperty_SetRenderCallback,
        kAudioUnitScope_Global, 0, &input, sizeof(AURenderCallbackStruct));
    assert(status == 0);

    status = AudioUnitInitialize(audio_unit);
    assert(status == 0);

    status = AudioOutputUnitStart(audio_unit);
    assert(status == 0);
}

internal void macos_audio_fill_buffer(struct Macos_AudioOutput *audio_output,
                                      s32 byte_to_lock, s32 bytes_to_write)
{
    void *region_1    = (u8 *)audio_output->data + byte_to_lock;
    u32 region_1_size = bytes_to_write;
    if (region_1_size + byte_to_lock > audio_output->buffer_size)
    {
        region_1_size = audio_output->buffer_size - byte_to_lock;
    }

    void *region_2    = audio_output->data;
    u32 region_2_size = bytes_to_write - region_1_size;

    u32 region_1_sample_count = region_1_size / audio_output->bytes_per_sample;
    s16 *sample_out           = (s16 *)region_1;

    for (int i = 0; i < region_1_sample_count; i++)
    {
        s16 sample = sinf(audio_output->time_sine) * audio_output->volume;

        *sample_out = sample;
        sample_out++;
        *sample_out = sample;
        sample_out++;

        audio_output->time_sine +=
            (2.0f * PI_F32 * 1.0f) / (f32)audio_output->period;
        audio_output->running_sample_index++;
    }

    u32 region_2_sample_count = region_2_size / audio_output->bytes_per_sample;
    sample_out                = (s16 *)region_2;

    for (int i = 0; i < region_2_sample_count; i++)
    {
        s16 sample = sinf(audio_output->time_sine) * audio_output->volume;

        *sample_out = sample;
        sample_out++;
        *sample_out = sample;
        sample_out++;

        audio_output->time_sine +=
            (2.0f * PI_F32 * 1.0f) / (f32)audio_output->period;
        audio_output->running_sample_index++;
    }
}
