#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <math.h>
#include <stdlib.h>
#include <sys/_types/_ssize_t.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "handmade.h"

SDL_Gamepad *gamepad_handles[MAX_GAMEPADS];

struct sdl_offscreen_buffer
{
    SDL_Texture *texture;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct sdl_sound_output
{
    int samples_per_second;
    int tone_hz;
    float tone_volume;
    float t_sine;
    uint32_t running_sample_index;
};

struct sdl_state
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

static void sdl_resize_window(SDL_Renderer *renderer,
                              struct sdl_offscreen_buffer *buffer,
                              int width,
                              int height)
{
    int bytes_per_pixel = 4;

    if (buffer->memory)
    {
        munmap(buffer->memory, buffer->pitch * buffer->height);
    }
    if (buffer->texture)
    {
        SDL_DestroyTexture(buffer->texture);
    }

    buffer->texture =
        SDL_CreateTexture(renderer,
                          SDL_PIXELFORMAT_ABGR8888, // macos format
                          SDL_TEXTUREACCESS_STREAMING,
                          width,
                          height);

    buffer->memory = mmap(0,
                          width * height * bytes_per_pixel,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS,
                          -1,
                          0);
    buffer->width  = width;
    buffer->height = height;
    buffer->pitch  = width * bytes_per_pixel;
}

void sdl_audio_device_callback(void *userdata,
                               SDL_AudioStream *stream,
                               int additional_amount,
                               int total_amount)
{
    struct sdl_sound_output *sound = (struct sdl_sound_output *)userdata;
    additional_amount /= sizeof(float);
    while (additional_amount > 0)
    {
        float samples[128];
        int samples_size = ARRAY_SIZE(samples);
        int total =
            additional_amount < samples_size ? additional_amount : samples_size;

        for (int i = 0; i < total; i++)
        {
            samples[i] = sinf(sound->t_sine) * sound->tone_volume;
            sound->t_sine += 2.0f * PI_F32 * (float)sound->tone_hz /
                             (float)sound->samples_per_second;
            sound->running_sample_index++;
        }

        sound->running_sample_index %= sound->samples_per_second;

        SDL_PutAudioStreamData(stream, samples, total * sizeof(float));
        additional_amount -= total;
    }
}

static int sdl_get_window_refresh_rate(SDL_Window *window)
{
    int default_rate            = 60;
    int window_index            = SDL_GetDisplayForWindow(window);
    const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(window_index);
    if (!mode || mode->refresh_rate == 0.0f)
    {
        return default_rate;
    }
    return mode->refresh_rate;
}

static float sdl_get_seconds_elapsed(uint64_t old_counter,
                                     uint64_t current_counter)
{
    return (float)(current_counter - old_counter) /
           (float)SDL_GetPerformanceFrequency();
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

internal time_t sdl_get_last_file_write(char *path)
{
    time_t result           = 0;
    struct stat file_status = {0};
    if (stat(path, &file_status) == 0)
    {
        result = file_status.st_mtime;
    }

    return result;
}

internal struct lib_game sdl_load_game_code()
{
    char *lib_path = "libhandmade.dylib";
    char *tmp_path = "tmp_libhandmade.dylib";

    struct lib_game game = {0};

    game.last_modification_time = sdl_get_last_file_write(lib_path);
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
    printf("finished loading libhandmade.dylib\n");
    return game;
}

internal void sdl_unload_game_code(struct lib_game *game)
{
    if (game->lib_handle)
    {
        dlclose(game->lib_handle);
        game->lib_handle = 0;
    }

    game->is_valid          = 0;
    game->update_and_render = stub_game_update_and_render;
}

#if HANDMADE_INTERNAL
internal void sdl_record_input(struct sdl_state *state,
                               struct game_input *input)
{
    size_t bytes_written =
        fwrite(input, sizeof(char), sizeof(*input), state->recording_handle);
    if (bytes_written <= 0)
    {
        // Log record input failure
    }
}

internal void sdl_begin_recording_input(struct sdl_state *state, s32 index)
{
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

internal void sdl_end_recording_input(struct sdl_state *state)
{
    state->is_recording = 0;
}

internal void sdl_begin_input_playback(struct sdl_state *state, s32 index)
{
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

internal void sdl_end_input_playback(struct sdl_state *state)
{
    state->is_playing_back = 0;
}

internal void sdl_playback_input(struct sdl_state *state,
                                 struct game_input *input)
{
    size_t bytes_read =
        fread(input, sizeof(char), sizeof(*input), state->playback_handle);
    if (bytes_read <= 0)
    {
        sdl_end_input_playback(state);
        sdl_begin_input_playback(state, 1); // loop playback
    }
}
#endif

int main(void)
{
    int monitor_update_hz          = 60;
    int game_update_hz             = monitor_update_hz / 2;
    float target_seconds_per_frame = 1.0f / (float)game_update_hz;

    struct sdl_state platform_state         = {0};
    struct sdl_offscreen_buffer back_buffer = {0};

#if HANDMADE_INTERNAL
    void *base_address = (void *)TERABYTES(2);
#else
    void *base_address = (void *)0;
#endif
    struct game_memory game_memory = {0};
    game_memory.permanent_size     = MEGABYTES(64);
    game_memory.transient_size     = GIGABYTES(4);

    uint64_t total_mem_size =
        game_memory.permanent_size + game_memory.transient_size;
    game_memory.permanent = mmap(base_address,
                                 total_mem_size,
                                 PROT_READ | PROT_WRITE,
                                 MAP_ANONYMOUS | MAP_PRIVATE,
                                 -1,
                                 0);
    game_memory.transient =
        (uint8_t *)game_memory.permanent + game_memory.permanent_size;

    struct game_input input[2]   = {0};
    struct game_input *new_input = &input[0];
    struct game_input *old_input = &input[1];

    ASSERT(game_memory.permanent && game_memory.transient);
    if (!game_memory.permanent && !game_memory.transient)
    {
        printf("Error setting up game memory\n");
        exit(1);
    }

    platform_state.memory_block      = game_memory.permanent;
    platform_state.memory_block_size = game_memory.permanent_size;

    int file_descriptor;
    mode_t mode = S_IRUSR | S_IWUSR;
    char filename[256];
    sprintf(filename, "replay_buffer.hmi");
    file_descriptor = open(filename, O_CREAT | O_RDWR, mode);
    int result      = truncate(filename, (s64)game_memory.permanent_size);

    if (result < 0)
    {
        printf("Failed to open replay_buffer.hmi\n");
        exit(1);
    }

    platform_state.replay_memory_block = mmap(0,
                                              game_memory.permanent_size,
                                              PROT_READ | PROT_WRITE,
                                              MAP_PRIVATE,
                                              file_descriptor,
                                              0);
    platform_state.replay_file_handle  = fopen(filename, "r+");
    fseek(platform_state.replay_file_handle,
          (int)platform_state.memory_block_size,
          SEEK_SET);
    if (!platform_state.replay_memory_block)
    {
        printf("Failed to allocate replay_memory_block\n");
        exit(1);
    }

    // #if HANDMADE_INTERNAL
    //     memory.debug_platform_free_file  = debug_platform_free_file;
    //     memory.debug_platform_read_file  = debug_platform_read_file;
    //     memory.debug_platform_write_file = debug_platform_write_file;
    // #endif

    struct lib_game game = sdl_load_game_code();
    // end game setup

    // begin platform setup
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK |
             SDL_INIT_GAMEPAD);

    SDL_Window *window = SDL_CreateWindow(
        "Handmade Hero", RENDER_WIDTH, RENDER_HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, 0);

    sdl_resize_window(renderer, &back_buffer, RENDER_WIDTH, RENDER_HEIGHT);

    struct sdl_sound_output sound_output = {0};
    sound_output.samples_per_second      = 48000;
    sound_output.tone_hz                 = 256;
    sound_output.tone_volume             = 3.0f;

    SDL_AudioSpec audio_spec = {0};
    audio_spec.freq          = sound_output.samples_per_second;
    audio_spec.channels      = 1;
    audio_spec.format        = SDL_AUDIO_F32;
    SDL_AudioStream *audio_stream =
        SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                  &audio_spec,
                                  sdl_audio_device_callback,
                                  &sound_output);
    if (!audio_stream)
    {
        printf("failed to open audio stream: %s\n", SDL_GetError());
    }
#if 0
    SDL_ResumeAudioStreamDevice(audio_stream);
#endif
    // end platform

    // run loop
    int running              = 1;
    uint64_t perf_count_freq = SDL_GetPerformanceFrequency();
    uint64_t last_counter    = SDL_GetPerformanceCounter();
    while (running)
    {
        new_input->delta_time_for_frame = target_seconds_per_frame;

        time_t mtime = sdl_get_last_file_write("libhandmade.dylib");
        if (game.last_modification_time < mtime)
        {
            printf("loading updated game library\n");
            sdl_unload_game_code(&game);
            game                        = sdl_load_game_code();
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

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
#if HANDMADE_INTERNAL
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.key == SDLK_P)
                {
                    if (platform_state.is_playing_back)
                    {
                        sdl_end_input_playback(&platform_state);
                        break;
                    }

                    if (platform_state.is_recording)
                    {
                        sdl_end_recording_input(&platform_state);
                        sdl_begin_input_playback(&platform_state, 1);
                        break;
                    }
                    else
                    {
                        sdl_begin_recording_input(&platform_state, 1);
                    }
                }
                if (event.key.key == SDLK_ESCAPE)
                {
                    running = 0;
                }
            }
#endif
            if (event.type == SDL_EVENT_QUIT)
            {
                running = 0;
            }
            else if (event.type == SDL_EVENT_GAMEPAD_ADDED)
            {
                int count = 0;
                SDL_GetGamepads(&count);
                if (count == MAX_GAMEPADS)
                {
                    continue;
                }
                printf("gamepad added\n");
                SDL_JoystickID id          = event.gdevice.which;
                SDL_Gamepad *gamepad       = SDL_OpenGamepad(id);
                gamepad_handles[count - 1] = gamepad;
            }
            else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
            {
                printf("gamepad button down: %d\n", event.gbutton.button);
            }
            else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
            {
#if 0
                printf("gamepad axis motion: %d\n", event.gaxis.value);
#endif
            }
            if (event.type == SDL_EVENT_KEY_DOWN ||
                event.type == SDL_EVENT_KEY_UP)
            {
                SDL_Keycode key = event.key.key;
                int is_down     = event.key.down;
                if (event.key.repeat == 0)
                {
                    if (key == SDLK_W)
                    {
                        process_keyboard_key_input(&new_keyboard->move_up,
                                                   is_down);
                    }
                    else if (key == SDLK_S)
                    {
                        process_keyboard_key_input(&new_keyboard->move_down,
                                                   is_down);
                    }
                    else if (key == SDLK_A)
                    {
                        process_keyboard_key_input(&new_keyboard->move_left,
                                                   is_down);
                    }
                    else if (key == SDLK_D)
                    {
                        process_keyboard_key_input(&new_keyboard->move_right,
                                                   is_down);
                    }
                    if (key == SDLK_I)
                    {
                        process_keyboard_key_input(&new_keyboard->action_up,
                                                   is_down);
                    }
                    else if (key == SDLK_K)
                    {
                        process_keyboard_key_input(&new_keyboard->action_down,
                                                   is_down);
                    }
                    else if (key == SDLK_J)
                    {
                        process_keyboard_key_input(&new_keyboard->action_left,
                                                   is_down);
                    }
                    else if (key == SDLK_L)
                    {
                        process_keyboard_key_input(&new_keyboard->action_right,
                                                   is_down);
                    }
                    if (key == SDLK_Q)
                    {
                        process_keyboard_key_input(&new_keyboard->shoulder_left,
                                                   is_down);
                    }
                    else if (key == SDLK_E)
                    {
                        process_keyboard_key_input(
                            &new_keyboard->shoulder_right, is_down);
                    }
                    if (key == SDLK_BACKSPACE)
                    {
                        process_keyboard_key_input(&new_keyboard->back,
                                                   is_down);
                    }
                }
            }
        }

        struct game_input *tmp_input = new_input;
        new_input                    = old_input;
        old_input                    = tmp_input;

        for (int i = 0; i < MAX_GAMEPADS; i++)
        {
            if (gamepad_handles[i] == 0)
                continue;

            struct game_controller_input *old_controller =
                get_controller(old_input, i + 1);
            struct game_controller_input *new_controller =
                get_controller(old_input, i + 1);

            SDL_Gamepad *gamepad         = gamepad_handles[i];
            new_controller->is_connected = SDL_GamepadConnected(gamepad);
            if (!new_controller->is_connected)
                continue;

            process_gamepad_button_input(
                &old_controller->action_down,
                &new_controller->action_down,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH));
            process_gamepad_button_input(
                &old_controller->action_up,
                &new_controller->action_up,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH));
            process_gamepad_button_input(
                &old_controller->action_left,
                &new_controller->action_left,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST));
            process_gamepad_button_input(
                &old_controller->action_right,
                &new_controller->action_right,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST));
            process_gamepad_button_input(
                &old_controller->shoulder_left,
                &new_controller->shoulder_left,
                SDL_GetGamepadButton(gamepad,
                                     SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
            process_gamepad_button_input(
                &old_controller->shoulder_right,
                &new_controller->shoulder_right,
                SDL_GetGamepadButton(gamepad,
                                     SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
            process_gamepad_button_input(
                &old_controller->back,
                &new_controller->back,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK));
            process_gamepad_button_input(
                &old_controller->start,
                &new_controller->start,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START));

            int16_t axis_leftx =
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
            int16_t axis_lefty =
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
            new_controller->axis_leftx_average =
                normalize_gamepad_axis_input(axis_leftx, 32768, 0, 8000);
            new_controller->axis_lefty_average =
                normalize_gamepad_axis_input(axis_lefty, 32768, 0, 8000);

            new_controller->is_analog_movement =
                new_controller->axis_leftx_average != 0.0f ||
                new_controller->axis_lefty_average != 0.0f;
            if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP))
            {
                new_controller->axis_lefty_average = -1.0f;
                new_controller->is_analog_movement = 0;
            }
            else if (SDL_GetGamepadButton(gamepad,
                                          SDL_GAMEPAD_BUTTON_DPAD_DOWN))
            {
                new_controller->axis_lefty_average = 1.0f;
                new_controller->is_analog_movement = 0;
            }
            else if (SDL_GetGamepadButton(gamepad,
                                          SDL_GAMEPAD_BUTTON_DPAD_LEFT))
            {
                new_controller->axis_leftx_average = -1.0f;
                new_controller->is_analog_movement = 0;
            }
            else if (SDL_GetGamepadButton(gamepad,
                                          SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
            {
                new_controller->axis_leftx_average = 1.0f;
                new_controller->is_analog_movement = 0;
            }

            float axis_threshold = 0.5f;
            process_gamepad_button_input(&old_controller->move_down,
                                         &new_controller->move_down,
                                         new_controller->axis_lefty_average >
                                             axis_threshold);
            process_gamepad_button_input(&old_controller->move_right,
                                         &new_controller->move_right,
                                         new_controller->axis_leftx_average >
                                             axis_threshold);

            process_gamepad_button_input(&old_controller->move_up,
                                         &new_controller->move_up,
                                         new_controller->axis_lefty_average <
                                             -axis_threshold);
            process_gamepad_button_input(&old_controller->move_left,
                                         &new_controller->move_left,
                                         new_controller->axis_leftx_average <
                                             -axis_threshold);
        }

#if HANDMADE_INTERNAL
        if (platform_state.is_recording)
        {
            sdl_record_input(&platform_state, new_input);
        }
        else if (platform_state.is_playing_back)
        {
            sdl_playback_input(&platform_state, new_input);
        }
#endif

        struct game_back_buffer buffer = {0};
        buffer.data                    = back_buffer.memory;
        buffer.width                   = back_buffer.width;
        buffer.height                  = back_buffer.height;
        buffer.pitch                   = back_buffer.pitch;
        buffer.bytes_per_pixel         = 4;
        game.update_and_render(&game_memory, &buffer, NULL, new_input);

        // enforce frame rate
        float seconds_elapsed =
            sdl_get_seconds_elapsed(last_counter, SDL_GetPerformanceCounter());
        if (seconds_elapsed < target_seconds_per_frame)
        {
            int32_t time_to_sleep =
                (target_seconds_per_frame - seconds_elapsed) * 1000 - 1;
            if (time_to_sleep > 0)
            {
                SDL_Delay(time_to_sleep);
            }
            // ASSERT(sdl_get_seconds_elapsed(last_counter,
            //                                SDL_GetPerformanceCounter()) <
            //        target_seconds_per_frame)
            while (sdl_get_seconds_elapsed(last_counter,
                                           SDL_GetPerformanceCounter()) <
                   target_seconds_per_frame)
            {
                // wait...
            }
        }
        else
        {
            // TODO: handle missing frame rate
            // TODO: Logging
        }

        uint64_t end_counter = SDL_GetPerformanceCounter();

        SDL_UpdateTexture(
            back_buffer.texture, 0, back_buffer.memory, back_buffer.pitch);
        SDL_RenderTexture(renderer, back_buffer.texture, 0, 0);
        SDL_RenderPresent(renderer);

#if 0 // log fps
        uint64_t counter_elapsed = end_counter - last_counter;
        double ms_per_frame =
            (1000.0f * (double)counter_elapsed) / (double)perf_count_freq;
        double fps = (double)perf_count_freq / (double)counter_elapsed;
        printf("%.02f ms/f, %.02ff/s\n", ms_per_frame, fps);
#endif

        last_counter = end_counter;
    }
    // end run loop

    // exit cleanup
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (gamepad_handles[i])
        {
            SDL_CloseGamepad(gamepad_handles[i]);
        }
    }
    SDL_DestroyAudioStream(audio_stream);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
