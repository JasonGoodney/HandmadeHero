#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDLib.h>
#include <OpenGL/OpenGL.h>
#include <Security/Security.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_types/_ssize_t.h>
#include <sys/_types/_useconds_t.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#include "game.h"
#include "macos.h"

global BOOL RUNNING;

internal struct macos_gamepad *gamepad_handles[MAX_GAMEPADS];

@interface HandmadeWindowDelegate : NSObject <NSWindowDelegate>

@property(nonatomic, assign) struct lib_game *game;
@property(nonatomic, assign) struct game_memory *memory;
@property(nonatomic, assign) struct game_back_buffer *back_buffer;
@property(nonatomic, assign) struct game_audio_buffer *audio_buffer;
@property(nonatomic, assign) struct game_input *input;

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

    NSString *title = [NSString stringWithFormat:@"Handmade Hero (%d x %d)",
                                                 (int)rect.size.width,
                                                 (int)rect.size.height];
    [window setTitle:title];

    if (self.memory && self.back_buffer && self.audio_buffer && self.input)
    {
        macos_buffer_resize(
            self.back_buffer, rect.size.width, rect.size.height);
        self.game->update_and_render(
            self.memory, self.back_buffer, self.audio_buffer, self.input);
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
    u32 bytes_to_write      = size;
    u8 *next_bytes_location = (u8 *)data;
    while (bytes_to_write)
    {
        ssize_t bytes_written =
            write(file_handle, next_bytes_location, bytes_to_write);
        if (bytes_written == -1)
        {
            close(file_handle);
            return true;
        }

        bytes_to_write -= bytes_written;
        next_bytes_location += bytes_written;
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

internal void macos_record_input(struct macos_state *state,
                                 struct game_input *input)
{
    // write(state->recording_handle, (uint8_t *)input, sizeof(*input));
    size_t bytes_written =
        fwrite(input, sizeof(char), sizeof(*input), state->recording_handle);
    if (bytes_written <= 0)
    {
        // Log record input failure
    }
}

internal void macos_begin_recording_input(struct macos_state *state, s32 index)
{
    // state->input_recording_index = index;
    // char *filename               = "foo.hmi";
    // state->recording_handle =
    //     open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP |
    //     S_IROTH);

    if (state->replay_memory_block)
    {
        state->recording_handle = state->replay_file_handle;
        fseek(state->recording_handle, (s64)state->memory_block_size, SEEK_SET);
        memcpy(state->replay_memory_block,
               state->memory_block,
               state->memory_block_size);
        state->is_recording = 1;
    }
}

internal void macos_end_recording_input(struct macos_state *state)
{
    // close(state->recording_handle);
    // state->input_recording_index = 0;
    state->is_recording = 0;
}

internal void macos_begin_input_playback(struct macos_state *state, s32 index)
{
    // state->input_playing_index = index;
    // char *filename             = "foo.hmi";
    // state->recording_handle    = open(filename, O_RDONLY | O_CREAT,
    //                                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (state->replay_memory_block)
    {
        state->playback_handle = state->replay_file_handle;
        fseek(state->playback_handle, (s64)state->memory_block_size, SEEK_SET);
        memcpy(state->memory_block,
               state->replay_memory_block,
               state->memory_block_size);
        state->is_playing_back = 1;
    }
}

internal void macos_end_input_playback(struct macos_state *state)
{
    // close(state->playback_handle);
    // state->input_playing_index = 0;
    state->is_playing_back = 0;
}

internal void macos_playback_input(struct macos_state *state,
                                   struct game_input *input)
{
    // ssize_t bytes_read =
    //     read(state->playback_handle, (uint8_t *)input, sizeof(*input));
    // if (bytes_read == 0) {
    //     // reached the end of the stream
    //     s32 index = state->input_playing_index;
    //     macos_end_input_playback(state);
    //     macos_begin_input_playback(state, index);
    //     bytes_read =
    //         read(state->playback_handle, (uint8_t *)input, sizeof(*input));
    // }
    size_t bytes_read =
        fread(input, sizeof(char), sizeof(*input), state->playback_handle);
    if (bytes_read <= 0)
    {
        macos_end_input_playback(state);
        macos_begin_input_playback(state, 1); // loop playback
    }
}

internal void macos_debug_draw_vertical_line(
    struct game_back_buffer *back_buffer, int x, int top, int bottom, u32 color)
{
    u8 *pixel = (u8 *)back_buffer->data + (top * back_buffer->pitch) +
                (x * back_buffer->bytes_per_pixel);
    for (int y = top; y < bottom; y++)
    {
        *(u32 *)pixel = color;
        pixel += back_buffer->pitch;
    }
}

internal void
macos_draw_audio_buffer_time_marker(struct game_back_buffer *back_buffer,
                                    struct macos_audio_output *audio_output,
                                    f32 coeff,
                                    int pad_x,
                                    int top,
                                    int bottom,
                                    int value,
                                    u32 color)
{

    ASSERT(value < audio_output->buffer_size);

    f32 x_float = coeff * (f32)value;
    int x       = pad_x + (int)x_float;
    macos_debug_draw_vertical_line(back_buffer, x, top, bottom, color);
}

internal void
macos_debug_sync_display(struct game_back_buffer *back_buffer,
                         struct macos_debug_time_marker *time_markers,
                         size_t time_markers_size,
                         struct macos_audio_output *audio_output,
                         f32 target_ms_per_frame)
{
    int pad_x = 16;
    int pad_y = 16;

    int top    = pad_y;
    int bottom = back_buffer->height - pad_y;

    f32 coeff =
        (f32)(back_buffer->width - 2 * pad_x) / (f32)audio_output->buffer_size;

    for (size_t marker_index = 0; marker_index < time_markers_size;
         marker_index++)
    {
        struct macos_debug_time_marker *time_marker =
            &time_markers[marker_index];
        macos_draw_audio_buffer_time_marker(back_buffer,
                                            audio_output,
                                            coeff,
                                            pad_x,
                                            top,
                                            bottom,
                                            time_marker->play_cursor,
                                            0xFFFFFFFF);
        macos_draw_audio_buffer_time_marker(back_buffer,
                                            audio_output,
                                            coeff,
                                            pad_x,
                                            top,
                                            bottom,
                                            time_marker->write_cursor,
                                            0xFF0000FF);
    }
}
#endif



internal f32 macos_get_milliseconds_elapsed(u64 start,
                                            u64 end,
                                            mach_timebase_info_data_t *timebase)
{
    u64 time_ns = (end - start) * (timebase->numer / timebase->denom);
    f32 result  = (f32)time_ns * 1.0E-6;
    return result;
}

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

internal time_t macos_get_last_file_write(char *path)
{
    time_t result           = 0;
    struct stat file_status = {0};
    if (stat(path, &file_status) == 0)
    {
        result = file_status.st_mtime;
    }

    return result;
}

internal struct lib_game macos_load_game_code()
{
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

    game->is_valid          = 0;
    game->update_and_render = stub_game_update_and_render;
}

int main(void)
{
    // begin game setup
    struct game_back_buffer back_buffer = {0};
    back_buffer.bytes_per_pixel     = 4;

    const u32 monitor_refresh_rate_hz = 60;
    const u32 game_refresh_rate_hz    = monitor_refresh_rate_hz / 2;
    f32 target_ms_per_frame           = 1.0E3f / game_refresh_rate_hz;

    struct macos_gamepad mac_gamepad = {0};
    mac_gamepad.analog_stick_left_x.state = 128;
    mac_gamepad.analog_stick_left_y.state = 128;
    mac_gamepad.analog_stick_right_x.state = 128;
    mac_gamepad.analog_stick_right_y.state = 128;
    macos_device_register(&mac_gamepad);
    gamepad_handles[0] = &mac_gamepad;

    struct game_input input[2]   = {0};
    struct game_input *new_input = &input[0];
    struct game_input *old_input = &input[1];

    struct game_audio_buffer audio_buffer   = {0};
    struct macos_audio_output audio_output = {0};
    macos_audio_create(&audio_output);

    s16 *samples =
        calloc(audio_output.sample_rate_khz, audio_output.bytes_per_sample);
    audio_buffer.sample_rate_khz = audio_output.sample_rate_khz;

#if HANDMADE_INTERNAL
    void *base_address = (void *)TERABYTES(2);
#else
    void *base_address = (void *)0;
#endif

    struct macos_state macos_state = {0};
    struct game_memory memory         = {0};
    memory.permanent_size          = MEGABYTES(64);
    memory.transient_size          = GIGABYTES(2);
    memory.permanent =
        mmap(base_address,
             (u64)(memory.permanent_size + memory.transient_size),
             PROT_READ | PROT_WRITE,
             MAP_ANON | MAP_PRIVATE,
             -1,
             0);
    memory.transient = ((u8 *)memory.permanent + memory.permanent_size);

    ASSERT(samples && memory.permanent && memory.transient);
    if (!samples && !memory.permanent && !memory.transient)
    {
        NSLog(@"Error setting up memory");
    }

    macos_state.memory_block      = memory.permanent;
    macos_state.memory_block_size = memory.permanent_size;

    int file_descriptor;
    mode_t mode = S_IRUSR | S_IWUSR;
    char filename[256];
    sprintf(filename, "replay_buffer.hmi");
    file_descriptor = open(filename, O_CREAT | O_RDWR, mode);
    int result      = truncate(filename, (s64)memory.permanent_size);

    if (result < 0)
    {
        printf("Failed to open replay_buffer.hmi\n");
        exit(1);
    }

    macos_state.replay_memory_block = mmap(0,
                                           memory.permanent_size,
                                           PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE,
                                           file_descriptor,
                                           0);
    macos_state.replay_file_handle  = fopen(filename, "r+");
    fseek(macos_state.replay_file_handle,
          (int)macos_state.memory_block_size,
          SEEK_SET);
    if (!macos_state.replay_memory_block)
    {
        printf("Failed to allocate replay_memory_block\n");
        exit(1);
    }

#if HANDMADE_INTERNAL
    memory.debug_platform_free_file  = debug_platform_free_file;
    memory.debug_platform_read_file  = debug_platform_read_file;
    memory.debug_platform_write_file = debug_platform_write_file;
#endif

    struct lib_game game = macos_load_game_code();
    // end game setup

    // begin platform setup
    NSApplication *app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
    [app activateIgnoringOtherApps:YES];

    NSRect screenRect = [[NSScreen mainScreen] frame];

    NSRect contentRect =
        NSMakeRect((screenRect.size.width - RENDER_WIDTH) * 0.5,
                   (screenRect.size.height - RENDER_HEIGHT) * 0.5,
                   RENDER_WIDTH,
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

    windowDelegate.game         = &game;
    windowDelegate.memory       = &memory;
    windowDelegate.back_buffer  = &back_buffer;
    windowDelegate.audio_buffer = &audio_buffer;
    windowDelegate.input        = new_input;

    CGRect rect = macos_get_window_rect(window);
    macos_buffer_resize(&back_buffer, rect.size.width, rect.size.height);
    NSString *title = [NSString stringWithFormat:@"Handmade Hero (%d x %d)",
                                                 back_buffer.width,
                                                 back_buffer.height];
    [window setTitle:title];
    // end platform

    // run loop
    size_t debug_time_marker_index = 0;
    struct macos_debug_time_marker
        debug_time_markers[game_refresh_rate_hz / 2] = {0};

    RUNNING = true;
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    u64 last_clock_tick = mach_absolute_time();

    while (RUNNING)
    {
        time_t mtime = macos_get_last_file_write("libgame.dylib");
        if (game.last_modification_time < mtime)
        {
            printf("loading updated game library\n");
            macos_unload_game_code(&game);
            game                        = macos_load_game_code();
            game.last_modification_time = mtime;
        }

        struct game_controller_input *old_keyboard =
            get_controller(old_input, 0);
        struct game_controller_input *new_keyboard =
            get_controller(new_input, 0);
        struct game_controller_input zero_controller = {0};
        *new_keyboard                                = zero_controller;
        new_keyboard->is_connected                   = 1;
        int button_count = ARRAY_SIZE(new_keyboard->buttons);
        for (int i = 0; i < button_count; i++)
        {
            new_keyboard->buttons[i].ended_pressed =
                old_keyboard->buttons[i].ended_pressed;
        }

        // Event handling
        NSEvent *event;
        do
        {
            event = [app nextEventMatchingMask:NSEventMaskAny
                                     untilDate:nil
                                        inMode:NSDefaultRunLoopMode
                                       dequeue:YES];

            NSEventType type = event.type;
            if (type == NSEventTypeKeyDown || type == NSEventTypeKeyUp)
            {
#if 0
                if ([event type] == NSEventTypeKeyDown) 
                {
                    printf("key down hex %x\n", event.keyCode);
                }
#endif
                int is_down = type == NSEventTypeKeyDown;
                if (event.keyCode == kVK_Escape)
                {
                    RUNNING = false;
                }
#if HANDMADE_INTERNAL
                else if (event.keyCode == kVK_ANSI_L)
                {
                    if (macos_state.is_playing_back)
                    {
                        macos_end_input_playback(&macos_state);
                        break;
                    }

                    if (macos_state.is_recording)
                    {
                        macos_end_recording_input(&macos_state);
                        macos_begin_input_playback(&macos_state, 1);
                        break;
                    }
                    else
                    {
                        macos_begin_recording_input(&macos_state, 1);
                    }
                }
#endif
                if (event.ARepeat == 0) 
                {
                    if (event.keyCode == kVK_ANSI_W)
                    {
                        process_keyboard_key_input(&new_keyboard->move_up, is_down);
                    }
                    else if (event.keyCode == kVK_ANSI_A)
                    {
                        process_keyboard_key_input(&new_keyboard->move_left, is_down);
                    }
                    else if (event.keyCode == kVK_ANSI_S)
                    {
                        process_keyboard_key_input(&new_keyboard->move_down, is_down);
                    }
                    else if (event.keyCode == kVK_ANSI_D)
                    {
                        process_keyboard_key_input(&new_keyboard->move_right, is_down);
                    }
                    if (event.keyCode == kVK_ANSI_K)
                    {
                        process_keyboard_key_input(&new_keyboard->action_down, is_down);
                    }
                }
            }
            else
            {
                [app sendEvent:event];
            }
        } while (event != nil);

        for (int i = 0; i < MAX_GAMEPADS; i++)
        {
            if (!gamepad_handles[i]) continue;
            if (!gamepad_handles[i]->is_connected) continue;

            struct game_controller_input *old_controller = get_controller(old_input, i + 1);
            struct game_controller_input *new_controller = get_controller(new_input, i + 1);
            new_controller->is_connected = gamepad_handles[i]->is_connected;

            process_gamepad_button_input(&old_controller->action_up,
                                        &new_controller->action_up,
                                        mac_gamepad.face_top.state == -1);
            process_gamepad_button_input(&old_controller->action_down,
                                        &new_controller->action_down,
                                        mac_gamepad.face_bottom.state == 1);
            process_gamepad_button_input(&old_controller->action_left,
                                        &new_controller->action_left,
                                        mac_gamepad.face_left.state == -1);
            process_gamepad_button_input(&old_controller->action_right,
                                        &new_controller->action_right,
                                        mac_gamepad.face_right.state == 1);

            // TODO: set should/back/start button usages
            process_gamepad_button_input(&old_controller->shoulder_left,
                                        &new_controller->shoulder_left,
                                        mac_gamepad.shoulder_left.state == -1);
            process_gamepad_button_input(&old_controller->shoulder_right,
                                        &new_controller->shoulder_right,
                                        mac_gamepad.shoulder_right.state == 1);
            process_gamepad_button_input(&old_controller->back,
                                        &new_controller->back,
                                        mac_gamepad.back.state == 1);
            process_gamepad_button_input(&old_controller->start,
                                        &new_controller->start,
                                        mac_gamepad.start.state == 1); 

            s32 axis_leftx = mac_gamepad.analog_stick_left_x.state;
            s32 axis_lefty = mac_gamepad.analog_stick_left_y.state;
 
            new_controller->axis_leftx_average = normalize_gamepad_axis_input(axis_leftx, 128, 128, 32);
            new_controller->axis_lefty_average = normalize_gamepad_axis_input(axis_lefty, 128, 128, 32);
  
            new_controller->is_analog_movement =
                    new_controller->axis_leftx_average != 0.0f ||
                    new_controller->axis_lefty_average != 0.0f;

             
            if (!new_controller->is_analog_movement)
            {
                new_controller->axis_leftx_average = mac_gamepad.dpad_x.state;
                new_controller->axis_lefty_average = mac_gamepad.dpad_y.state;
            }
            
            float axis_threshold = 0.5f;
            process_gamepad_button_input(
                &old_controller->move_down,
                &new_controller->move_down,
                new_controller->axis_lefty_average > axis_threshold);
            process_gamepad_button_input(
                &old_controller->move_right,
                &new_controller->move_right,
                new_controller->axis_leftx_average > axis_threshold);
            process_gamepad_button_input(
                &old_controller->move_up,
                &new_controller->move_up,
                new_controller->axis_lefty_average < -axis_threshold);
            process_gamepad_button_input(
                &old_controller->move_left,
                &new_controller->move_left,
                new_controller->axis_leftx_average < -axis_threshold);
        }

        struct game_input *tmp = new_input;
        new_input               = old_input;
        old_input               = tmp;

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

        audio_buffer.samples = samples;
        audio_buffer.sample_count =
            bytes_to_write / audio_output.bytes_per_sample;

#if HANDMADE_INTERNAL
        if (macos_state.is_recording)
        {
            macos_record_input(&macos_state, new_input);
        }
        else if (macos_state.is_playing_back)
        {
            macos_playback_input(&macos_state, new_input);
        }
#endif

        game.update_and_render(
            &memory, &back_buffer, &audio_buffer, new_input);

        // call after game update
        macos_audio_fill_buffer(
            &audio_output, &audio_buffer, byte_to_lock, bytes_to_write);

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

#if HANDMADE_INTERNAL
        {
            ASSERT(debug_time_marker_index < ARRAY_SIZE(debug_time_markers));
            struct macos_debug_time_marker *time_marker =
                &debug_time_markers[debug_time_marker_index++];
            if (debug_time_marker_index == ARRAY_SIZE(debug_time_markers))
            {
                debug_time_marker_index = 0;
            }
            time_marker->play_cursor  = audio_output.play_cursor;
            time_marker->write_cursor = target_cursor;
        }

        macos_debug_sync_display(&back_buffer,
                                 debug_time_markers,
                                 ARRAY_SIZE(debug_time_markers),
                                 &audio_output,
                                 target_ms_per_frame);
#endif
        // Render
        macos_window_display(&back_buffer, window);

#if 0
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

internal void macos_window_display(struct game_back_buffer *buffer,
                                   const NSWindow *window)
{

    @autoreleasepool
    {
        NSBitmapImageRep *imageRep = [[[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:&buffer->data
                          pixelsWide:buffer->width
                          pixelsHigh:buffer->height
                       bitsPerSample:8
                     samplesPerPixel:buffer->bytes_per_pixel
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

internal void
macos_buffer_resize(struct game_back_buffer *buffer, int width, int height)
{
    if (buffer->data)
    {
        free(buffer->data);
        buffer->data = NULL;
    }

    buffer->width  = width;
    buffer->height = height;
    buffer->pitch  = width * buffer->bytes_per_pixel;
    buffer->data   = (u8 *)malloc(buffer->pitch * height);
}

internal void macos_device_input_callback(void *context,
                                          IOReturn result,
                                          void *sender,
                                          IOHIDValueRef value)
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
    struct macos_gamepad *gamepad = (struct macos_gamepad *)context;

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

internal void macos_device_callback(void *context,
                                    IOReturn result,
                                    void *sender,
                                    IOHIDDeviceRef device)
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
                CFNumberGetValue(
                    (CFNumberRef)ref, kCFNumberSInt32Type, &vendorID);
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
                CFNumberGetValue(
                    (CFNumberRef)ref, kCFNumberSInt32Type, &productID);
            }
        }
    }

    struct macos_gamepad *gamepad = (struct macos_gamepad *)context;

    if (vendorID == 0x054C && productID == 0x09CC)
    {
        printf("Sony Dualshock 4 detected\n");
        gamepad->is_connected = 1;
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
        gamepad->is_connected = 1;
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
        gamepad->is_connected = 0;
    }

    IOHIDDeviceSetInputValueMatchingMultiple(device, (__bridge CFArrayRef) @[
        @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_Button)},
        @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_GenericDesktop)},
        @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_KeyboardOrKeypad)}
    ]);

    IOHIDDeviceRegisterInputValueCallback(
        device, macos_device_input_callback, context);
}

internal void macos_device_register(void *context)
{
    IOHIDManagerRef manager =
        IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

    IOHIDManagerSetDeviceMatchingMultiple(manager, (__bridge CFArrayRef) @[
        // NOTE: commented out, caused multiple callbacks for a single device
        // Need a better way to determine connected devices

        // @{
        //     @kIOHIDDeviceUsagePageKey : @(kHIDPage_GenericDesktop),
        //     @kIOHIDDeviceUsageKey : @(kHIDUsage_GD_Joystick)
        // },
        @{
            @kIOHIDDeviceUsagePageKey : @(kHIDPage_GenericDesktop),
            @kIOHIDDeviceUsageKey : @(kHIDUsage_GD_GamePad)
        },
        // @{
        //     @kIOHIDDeviceUsagePageKey : @(kHIDPage_GenericDesktop),
        //     @kIOHIDDeviceUsageKey : @(kHIDUsage_GD_MultiAxisController)
        // },
        // @{
        //     @kIOHIDDeviceUsagePageKey : @(kHIDPage_GenericDesktop),
        //     @kIOHIDDeviceUsageKey : @(kHIDUsage_GD_Keyboard)
        // }
    ]);

    IOHIDManagerRegisterDeviceMatchingCallback(
        manager, macos_device_callback, context);

    IOHIDManagerScheduleWithRunLoop(
        manager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

    IOReturn ioReturn = IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
    if (ioReturn != kIOReturnSuccess)
    {
        printf("an error occurred opening IOHIDManagerOpen\n");
        return;
    }
}

internal OSStatus
macos_audio_render_callback(void *int_ref_con,
                            enum AudioUnitRenderActionFlags *io_action_flags,
                            const struct AudioTimeStamp *in_timestamp,
                            unsigned int in_bus_number,
                            unsigned int in_number_frames,
                            struct AudioBufferList *io_data)
{
    UNUSED(io_action_flags);
    UNUSED(in_timestamp);
    UNUSED(in_bus_number);

    // Read from circular buffer
    struct macos_audio_output *audio_output =
        (struct macos_audio_output *)int_ref_con;

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
    memcpy(data,
           (u8 *)audio_output->data + audio_output->play_cursor,
           region_1_size);
    memcpy(&data[region_1_size], (u8 *)audio_output->data, region_2_size);

    audio_output->play_cursor = (audio_output->play_cursor + bytes_to_output) %
                                audio_output->buffer_size;

    return noErr;
}

internal void macos_audio_create(struct macos_audio_output *audio_output)
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
    AudioComponentInstance audio_unit;
    audio_output->audio_unit = &audio_unit;
    s32 status = AudioComponentInstanceNew(output, audio_output->audio_unit);
    assert(audio_output->audio_unit);

    uint32_t theDataSize          = sizeof(uint32_t);
    uint32_t outIOBufferFrameSize = 0;
    uint32_t inIOBufferFrameSize  = 900; // TODO: make dependent on frame rate

    status = AudioUnitSetProperty(audio_unit,
                                  kAudioDevicePropertyBufferFrameSize,
                                  kAudioUnitScope_Global,
                                  0,
                                  &inIOBufferFrameSize,
                                  sizeof(uint32_t));
    if (status != noErr)
    {
        NSLog(@"Failed to set the IO buffer frame size");
        return;
    }

#if HANDMADE_INTERNAL
    status = AudioUnitGetProperty(audio_unit,
                                  kAudioDevicePropertyBufferFrameSize,
                                  kAudioUnitScope_Global,
                                  0,
                                  &outIOBufferFrameSize,
                                  &theDataSize);

    if (status != noErr)
    {
        NSLog(@"Failed to get the IO buffer frame size");
        return;
    }
    else
    {
        NSLog(@"IO buffer frame size: %u", outIOBufferFrameSize);
    }
#endif

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

    status = AudioUnitSetProperty(*audio_output->audio_unit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input,
                                  0,
                                  &stream_format,
                                  sizeof(AudioStreamBasicDescription));
    assert(status == 0);

    // Set out one rendering function on the unit
    AURenderCallbackStruct input;
    input.inputProc       = macos_audio_render_callback;
    input.inputProcRefCon = audio_output;

    status = AudioUnitSetProperty(*audio_output->audio_unit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Global,
                                  0,
                                  &input,
                                  sizeof(AURenderCallbackStruct));
    assert(status == 0);

    status = AudioUnitInitialize(*audio_output->audio_unit);
    assert(status == 0);

    status = AudioOutputUnitStart(*audio_output->audio_unit);
    assert(status == 0);
}

internal void macos_audio_fill_buffer(struct macos_audio_output *audio_output,
                                      struct game_audio_buffer *audio_buffer,
                                      s32 byte_to_lock,
                                      s32 bytes_to_write)
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
