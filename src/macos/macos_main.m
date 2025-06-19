#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDLib.h>
#include <OpenGL/OpenGL.h>
#include <Security/Security.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#include "../game.h"
#include "macos.h"

#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_types/_ssize_t.h>
#include <sys/_types/_useconds_t.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

global const u8 BYTES_PER_PIXEL = 4;

global BOOL RUNNING;
global struct G_BackBuffer g_back_buffer;
global struct G_AudioBuffer g_audio_buffer;

@interface HandmadeWindowDelegate : NSObject <NSWindowDelegate>

@property(nonatomic, assign) struct lib_game *game;
@property(nonatomic, assign) struct G_Memory *memory;
@property(nonatomic, assign) struct G_BackBuffer *back_buffer;
@property(nonatomic, assign) struct G_AudioBuffer *audio_buffer;
@property(nonatomic, assign) struct G_Input *input;

@end

@implementation HandmadeWindowDelegate
- (void)windowWillClose:(NSNotification *)notification
{
    RUNNING = NO;
}

- (void)windowDidResize:(NSNotification *)notification
{
    NSWindow *window = (NSWindow *)notification.object;
    CGRect rect      = macos_get_window_rect(window);

    NSString *title =
        [NSString stringWithFormat:@"Handmade Hero (%d x %d)",
                                   (int)rect.size.width, (int)rect.size.height];
    [window setTitle:title];

    if (self.memory && self.back_buffer && self.audio_buffer && self.input)
    {
        macos_buffer_resize(self.back_buffer, rect.size.width,
                            rect.size.height);
        self.game->update_and_render(self.memory, self.back_buffer,
                                     self.audio_buffer, self.input);
        macos_window_display(self.back_buffer, window);
    }
}
@end // HandmadeWindowDelegate

#if HANDMADE_INTERNAL
DEBUG_PLATFORM_READ_FILE(debug_platform_read_file)
{
    struct debug_read_file_result result = {0};

    // open/create file
    int file_handle = open(path, O_RDONLY);
    if (file_handle == -1)
    {
        return result;
    }

    // get file size
    struct stat file_status;
    if (fstat(file_handle, &file_status) == -1)
    {
        close(file_handle);
        return result;
    }

    result.size = safe_truncate_uint64(file_status.st_size);

    // alloc file memory
    result.data = malloc(result.size);
    if (!result.data)
    {
        result.size = 0;
        close(file_handle);
        return result;
    }

    // read file
    u32 bytes_to_read       = result.size;
    u8 *next_bytes_location = (u8 *)result.data;
    while (bytes_to_read)
    {
        ssize_t bytes_read =
            read(file_handle, next_bytes_location, bytes_to_read);
        if (bytes_read == -1)
        {
            free(result.data);
            result.data = 0;
            result.size = 0;
            close(file_handle);
            return result;
        }

        bytes_to_read -= bytes_read;
        next_bytes_location += bytes_read;
    }

    // close handle
    close(file_handle);

    return result;
}

DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file)
{
    int file_handle =
        open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    u32 bytes_to_read       = size;
    u8 *next_bytes_location = (u8 *)data;
    while (bytes_to_read)
    {
        ssize_t bytes_read =
            write(file_handle, next_bytes_location, bytes_to_read);
        if (bytes_read == -1)
        {
            close(file_handle);
            return true;
        }

        bytes_to_read -= bytes_read;
        next_bytes_location += bytes_read;
    }

    return false;
}

DEBUG_PLATFORM_FREE_FILE(debug_platform_free_file)
{
    if (data)
    {
        free(data);
    }
}
#endif

internal void
macos_process_gamepad_button(struct G_GamepadButtonState *prev_state,
                             struct G_GamepadButtonState *curr_state,
                             b32 curr_is_pressed)
{
    curr_state->ended_pressed = curr_is_pressed;
    curr_state->half_transition_count +=
        prev_state->ended_pressed == curr_state->ended_pressed ? 0 : 1;
}

internal f32 macos_get_milliseconds_elapsed(u64 start, u64 end,
                                            mach_timebase_info_data_t *timebase)
{
    u64 time_ns = (end - start) * (timebase->numer / timebase->denom);
    f32 result  = (f32)time_ns * 1.0E-6;
    return result;
}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

void copy(char *src, char *dst)
{
    u8 buffer[512];

    FILE *fsrc = fopen(src, "rb");
    if (!fsrc)
    {
        printf("fopen src error: %s\n", src);
        exit(EXIT_FAILURE);
    }

    FILE *fdst = fopen(dst, "wb");
    if (!fdst)
    {
        printf("fopen dst error: %s", dst);
        exit(EXIT_FAILURE);
    }

    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), fsrc)) > 0)
    {
        fwrite(buffer, 1, bytes, fdst);
    }

    fclose(fsrc);
    fclose(fdst);

    printf("finished copy from %s to %s\n", src, dst);
}

internal struct timespec macos_get_last_file_write(char *path)
{
    struct timespec result = {0};

    // open/create file
    int file_handle = open(path, O_RDONLY);
    if (file_handle == -1)
    {
        printf("unable to open %s\n", path);
        exit(EXIT_FAILURE);
    }

    // get file size
    struct stat file_status;
    if (fstat(file_handle, &file_status) == -1)
    {
        printf("unable to get file status %s\n", path);
        close(file_handle);
        return result;
    }

    return file_status.st_mtimespec;
}

internal struct lib_game macos_load_game_code()
{
    char cwd[256];
    getcwd(cwd, 265);
    printf("cwd %s\n", cwd);

    char *lib_path = "libgame.dylib";
    char *tmp_path = "tmp_libgame.dylib";

    struct lib_game game = {0};

    game.last_modification_time = macos_get_last_file_write(lib_path);
    copy(lib_path, tmp_path);
    game.lib_handle = dlopen(tmp_path, RTLD_NOW);

    if (game.lib_handle)
    {
        game.update_and_render = (game_update_and_render_f *)dlsym(
            game.lib_handle, "game_update_and_render");

        if (!game.update_and_render)
        {
            printf("failed to locate symbol %s\n", "game_update_and_render");
            exit(EXIT_FAILURE);
        }
        game.is_valid = game.update_and_render != NULL;
    }
    else
    {
        printf("failed to open dylib at %s\n", tmp_path);
        exit(EXIT_FAILURE);
    }

    if (!game.is_valid)
    {
        game.update_and_render = stub_game_update_and_render;
    }
    printf("finished loading libgame.dylib\n");
    return game;
}

internal void macos_unload_game_code(struct lib_game *game)
{
    if (game->lib_handle)
    {
        dlclose(game->lib_handle);
        game->lib_handle = 0;
    }

    game->update_and_render = stub_game_update_and_render;
}
int main()
{
    NSApplication *app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
    [app activateIgnoringOtherApps:YES];

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
    macos_buffer_resize(&g_back_buffer, rect.size.width, rect.size.height);
    NSString *title =
        [NSString stringWithFormat:@"Handmade Hero (%d x %d)",
                                   g_back_buffer.width, g_back_buffer.height];
    [window setTitle:title];

    u32 monitor_refresh_rate_hz = 60;
    u32 game_refresh_rate_hz    = monitor_refresh_rate_hz / 2;
    f32 target_ms_per_frame     = 1.0E3f / game_refresh_rate_hz;

    struct Gamepad mac_gamepad = {0};
    macos_device_register(&mac_gamepad);

    struct G_Input inputs[2]   = {0};
    struct G_Input *curr_input = &inputs[0];
    struct G_Input *prev_input = &inputs[1];

    struct Macos_AudioOutput audio_output = {0};
    AudioComponentInstance audio_unit     = NULL;
    macos_audio_create(&audio_output, audio_unit);

    s16 *samples =
        calloc(audio_output.sample_rate_khz, audio_output.bytes_per_sample);
    g_audio_buffer.sample_rate_khz = audio_output.sample_rate_khz;

#if HANDMADE_INTERNAL
    void *base_address = (void *)TERABYTES(2);
#else
    void *base_address = (void *)0;
#endif

    struct G_Memory memory = {0};
    memory.permanent_size  = MEGABYTES(64);
    memory.transient_size  = GIGABYTES(2);
    memory.permanent =
        mmap(base_address, (u64)(memory.permanent_size + memory.transient_size),
             PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    memory.transient = ((u8 *)memory.permanent + memory.permanent_size);

    if (!samples && !memory.permanent && !memory.transient)
    {
        NSLog(@"Error setting up memory");
    }
#if HANDMADE_INTERNAL
    memory.debug_platform_free_file  = debug_platform_free_file;
    memory.debug_platform_read_file  = debug_platform_read_file;
    memory.debug_platform_write_file = debug_platform_write_file;
#endif

    struct lib_game game        = macos_load_game_code();
    windowDelegate.game         = &game;
    windowDelegate.memory       = &memory;
    windowDelegate.back_buffer  = &g_back_buffer;
    windowDelegate.audio_buffer = &g_audio_buffer;
    windowDelegate.input        = curr_input;

    RUNNING = true;
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    u64 last_clock_tick = mach_absolute_time();

    while (RUNNING)
    {
        struct timespec mtime = macos_get_last_file_write("libgame.dylib");
        if (mtime.tv_sec != 0 &&
            mtime.tv_sec != game.last_modification_time.tv_sec &&
            mtime.tv_nsec != game.last_modification_time.tv_nsec)
        {
            macos_unload_game_code(&game);
            game                        = macos_load_game_code();
            game.last_modification_time = mtime;
        }

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
                }

                if (event.keyCode == 0x0d) // W
                {
                    mac_gamepad.dpad_y.state = -1;
                }
                else if (event.keyCode == 0x00) // A
                {
                    mac_gamepad.dpad_x.state = -1;
                }
                else if (event.keyCode == 0x01) // S
                {
                    mac_gamepad.dpad_y.state = 1;
                }
                else if (event.keyCode == 0x02) // D
                {
                    mac_gamepad.dpad_x.state = 1;
                }

                break;
            case NSEventTypeKeyUp:
                if (event.keyCode == 0x31)
                {
                    // AudioOutputUnitStop(audio_output.audio_unit);
                }
                else if (event.keyCode == 0x7d)
                {
                }
                if (event.keyCode == 0x0d || event.keyCode == 0x01)
                {
                    mac_gamepad.dpad_y.state = 0;
                }
                else if (event.keyCode == 0x00 || event.keyCode == 0x02)
                {
                    mac_gamepad.dpad_x.state = 0;
                }

                break;
            default:
                [app sendEvent:event];
            }
        } while (event != nil);

        struct G_GamepadInput *prev_gamepad = &prev_input->gamepads[0];
        struct G_GamepadInput *curr_gamepad = &curr_input->gamepads[0];

        macos_process_gamepad_button(&prev_gamepad->dpad_top,
                                     &curr_gamepad->dpad_top,
                                     mac_gamepad.dpad_y.state == -1);

        macos_process_gamepad_button(&prev_gamepad->dpad_bottom,
                                     &curr_gamepad->dpad_bottom,
                                     mac_gamepad.dpad_y.state == 1);

        macos_process_gamepad_button(&prev_gamepad->dpad_left,
                                     &curr_gamepad->dpad_left,
                                     mac_gamepad.dpad_x.state == -1);

        macos_process_gamepad_button(&prev_gamepad->dpad_right,
                                     &curr_gamepad->dpad_right,
                                     mac_gamepad.dpad_x.state == 1);

        curr_gamepad->start_x = prev_gamepad->end_x;
        curr_gamepad->start_y = prev_gamepad->end_y;

        if (mac_gamepad.analog_stick_left_x.state < 128)
        {
            curr_gamepad->end_x =
                ((f32)mac_gamepad.analog_stick_left_x.state / 127) - 1;
        }
        else
        {
            curr_gamepad->end_x =
                (((f32)mac_gamepad.analog_stick_left_x.state - 128) / 127);
        }

        if (mac_gamepad.analog_stick_left_y.state < 128)
        {
            curr_gamepad->end_y =
                ((f32)mac_gamepad.analog_stick_left_y.state / 127) - 1;
        }
        else
        {
            curr_gamepad->end_y =
                (((f32)mac_gamepad.analog_stick_left_y.state - 128) / 127);
        }

        // TODO: min/max macros
        curr_gamepad->min_x = curr_gamepad->max_x = curr_gamepad->end_x;
        curr_gamepad->min_y = curr_gamepad->max_y = curr_gamepad->end_y;

#if 0 // my ps4 sticks are messed up and output feedback even while idle
        curr_gamepad->is_analog = true;
#endif
        struct G_Input *tmp = curr_input;
        curr_input          = prev_input;
        prev_input          = tmp;

        // Audio
        // write to circular buffer
        u32 latency_sample_count = audio_output.sample_rate_khz / 15;

        u32 target_queue_bytes =
            latency_sample_count * audio_output.bytes_per_sample;
        u32 target_cursor = (audio_output.play_cursor + target_queue_bytes) %
                            audio_output.buffer_size;

        u32 byte_to_lock = (audio_output.running_sample_index *
                            audio_output.bytes_per_sample) %
                           audio_output.buffer_size;
        u32 bytes_to_write;

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

        g_audio_buffer.samples = samples;
        g_audio_buffer.sample_count =
            bytes_to_write / audio_output.bytes_per_sample;

        game.update_and_render(&memory, &g_back_buffer, &g_audio_buffer,
                               curr_input);

        // call after game update
        macos_audio_fill_buffer(&audio_output, &g_audio_buffer, byte_to_lock,
                                bytes_to_write);

        // sleep until target ms per frame
        // TODO: change to nanoseconds
        f32 work_ms = macos_get_milliseconds_elapsed(
            last_clock_tick, mach_absolute_time(), &timebase);
        f32 work_ms_for_frame = work_ms;
        if (work_ms_for_frame < target_ms_per_frame)
        {
            f32 fudge_factor_ms = 3.0f;
            f32 sleep_ms =
                target_ms_per_frame - work_ms_for_frame - fudge_factor_ms;
            if (sleep_ms > 0)
            {
                usleep((useconds_t)(sleep_ms * 1000));
            }

            while (work_ms_for_frame < target_ms_per_frame)
            {
                work_ms_for_frame = macos_get_milliseconds_elapsed(
                    last_clock_tick, mach_absolute_time(), &timebase);
            }
        }
        else
        {
            // TODO: handle missing frame rate
            // TODO: Logging
        }

        // Render
        macos_window_display(&g_back_buffer, window);

#if 1
        // Time Profiling
        f32 elapsed_time_ms = macos_get_milliseconds_elapsed(
            last_clock_tick, mach_absolute_time(), &timebase);

        NSLog(@"ms/f %.02f, fps %.02f", elapsed_time_ms,
              1000.f / elapsed_time_ms);
#endif
        last_clock_tick = mach_absolute_time();
    }

    printf("Handmade Hero finished running.\n");

    return 0;
}

internal CGRect macos_get_window_rect(const NSWindow *window)
{
    return window.contentView.bounds;
}

internal void macos_window_display(struct G_BackBuffer *buffer,
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

internal void macos_buffer_resize(struct G_BackBuffer *buffer, int width,
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

internal void macos_device_input_callback(void *context, IOReturn result,
                                          void *sender, IOHIDValueRef value)
{
    UNUSED(sender);

    if (result != kIOReturnSuccess)
    {
        printf("Error device input callback:\n");
        return;
    }

    IOHIDElementRef element = IOHIDValueGetElement(value);
    u32 usagePage           = IOHIDElementGetUsagePage(element);
    u32 usage               = IOHIDElementGetUsage(element);
    s32 state               = (s32)IOHIDValueGetIntegerValue(value);

    //  printf("usage: %d\n", usage);
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
        s32 analog =
            IOHIDValueGetScaledValue(value, kIOHIDValueScaleTypeCalibrated);
        if (analog < 120 || 140 < analog)
        {
            printf("analog: %d\n", analog);
        }
        if (usage == kHIDUsage_GD_X)
        {
            gamepad->analog_stick_left_x.state = analog;
        }
        if (usage == kHIDUsage_GD_Y)
        {
            gamepad->analog_stick_left_y.state = analog;
        }
        if (usage == kHIDUsage_GD_Rx)
        {
            gamepad->analog_stick_right_x.state = analog;
        }
        if (usage == kHIDUsage_GD_Ry)
        {
            gamepad->analog_stick_right_y.state = analog;
        }
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
    UNUSED(sender);

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

        gamepad->analog_stick_left_x.state  = 128.f;
        gamepad->analog_stick_left_y.state  = 128.f;
        gamepad->analog_stick_right_x.state = 128.f;
        gamepad->analog_stick_right_y.state = 128.f;
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
    UNUSED(io_action_flags);
    UNUSED(in_timestamp);
    UNUSED(in_bus_number);

    // Read from circular buffer
    struct Macos_AudioOutput *audio_output =
        (struct Macos_AudioOutput *)int_ref_con;

    u32 bytes_to_output = in_number_frames * audio_output->bytes_per_sample;

    // Region 1 is the number of bytes up to the end of the buffer. If the
    // frames to be rendered causes us to wrap, the remained goes into
    // region 2.
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
    audio_output->channels             = 2;
    audio_output->sample_rate_khz      = 48000;
    audio_output->bytes_per_sample     = sizeof(s16) * 2;
    audio_output->running_sample_index = 0;

    // Allocates a 2 second buffer
    audio_output->buffer_size =
        2 * audio_output->sample_rate_khz * audio_output->bytes_per_sample;
    audio_output->data        = malloc(audio_output->buffer_size);
    audio_output->play_cursor = 0;

    // Configure the search parameters to find the default playback output
    // unit
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
                                      struct G_AudioBuffer *audio_buffer,
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

    for (u32 i = 0; i < region_1_sample_count; i++)
    {
        *sample_out++ = *audio_buffer->samples++;
        *sample_out++ = *audio_buffer->samples++;

        audio_output->running_sample_index++;
    }

    u32 region_2_sample_count = region_2_size / audio_output->bytes_per_sample;
    sample_out                = (s16 *)region_2;

    for (u32 i = 0; i < region_2_sample_count; i++)
    {
        *sample_out++ = *audio_buffer->samples++;
        *sample_out++ = *audio_buffer->samples++;

        audio_output->running_sample_index++;
    }
}
