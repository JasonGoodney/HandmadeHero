#include <SDL3/SDL.h>
#include <math.h>
#include <sys/mman.h>

#include "game.h"
#include "handmade.c"

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

    buffer->texture = SDL_CreateTexture(renderer,
                                        SDL_PIXELFORMAT_ARGB8888,
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

static void
sdl_process_game_controller_button(struct game_button_state *old_state,
                                   struct game_button_state *new_state,
                                   int value)
{
    new_state->ended_pressed = value;
    new_state->half_transition_count +=
        (new_state->ended_pressed == old_state->ended_pressed) ? 0 : 1;
}

static float sdl_process_game_controller_axis(int16_t value, int16_t dead_zone)
{
    float result = 0;
    if (value < -dead_zone)
    {
        result = (float)((value + dead_zone) / (32768.0f - dead_zone));
    }
    else if (value > dead_zone)
    {

        result = (float)((value - dead_zone) / (32767.0f - dead_zone));
    }
    return result;
}

static void sdl_process_key_press(struct game_button_state *new_state,
                                  int is_down)
{
    ASSERT(new_state->ended_pressed != is_down);
    new_state->ended_pressed = is_down;
    new_state->half_transition_count++;
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

int main(void)
{
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

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK |
             SDL_INIT_GAMEPAD);

    SDL_Window *window = SDL_CreateWindow(
        "Handmade Hero", RENDER_WIDTH, RENDER_HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, 0);

    struct sdl_offscreen_buffer back_buffer = {0};
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

    int game_update_hz             = 30;
    float target_seconds_per_frame = 1.0f / (float)game_update_hz;

    int running              = 1;
    uint64_t perf_count_freq = SDL_GetPerformanceFrequency();
    uint64_t last_counter    = SDL_GetPerformanceCounter();
    while (running)
    {
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
            else if (event.type == SDL_EVENT_KEY_DOWN ||
                     event.type == SDL_EVENT_KEY_UP)
            {
                SDL_Keycode key = event.key.key;
                int is_down     = event.key.down;

                if (event.key.repeat == 0)
                {
                    if (key == SDLK_W)
                    {
                        sdl_process_key_press(&new_keyboard->move_up, is_down);
                    }
                    else if (key == SDLK_S)
                    {
                        sdl_process_key_press(&new_keyboard->move_down,
                                              is_down);
                    }
                    else if (key == SDLK_A)
                    {
                        sdl_process_key_press(&new_keyboard->move_left,
                                              is_down);
                    }
                    else if (key == SDLK_D)
                    {
                        sdl_process_key_press(&new_keyboard->move_right,
                                              is_down);
                    }
                    else if (key == SDLK_I)
                    {
                        sdl_process_key_press(&new_keyboard->action_up,
                                              is_down);
                    }
                    else if (key == SDLK_K)
                    {
                        sdl_process_key_press(&new_keyboard->action_down,
                                              is_down);
                    }
                    else if (key == SDLK_J)
                    {
                        sdl_process_key_press(&new_keyboard->action_left,
                                              is_down);
                    }
                    else if (key == SDLK_L)
                    {
                        sdl_process_key_press(&new_keyboard->action_right,
                                              is_down);
                    }
                    else if (key == SDLK_Q)
                    {
                        sdl_process_key_press(&new_keyboard->shoulder_left,
                                              is_down);
                    }
                    else if (key == SDLK_E)
                    {
                        sdl_process_key_press(&new_keyboard->shoulder_right,
                                              is_down);
                    }
                    else if (key == SDLK_BACKSPACE)
                    {
                        sdl_process_key_press(&new_keyboard->back, is_down);
                    }
                }
            }
        }

        for (int i = 0; i < MAX_GAMEPADS; i++)
        {
            if (gamepad_handles[i] == 0)
            {
                continue;
            }
            struct game_controller_input *old_controller =
                get_controller(old_input, i + 1);
            struct game_controller_input *new_controller =
                get_controller(old_input, i + 1);

            SDL_Gamepad *gamepad         = gamepad_handles[i];
            new_controller->is_connected = SDL_GamepadConnected(gamepad);
            if (!new_controller->is_connected)
            {
                continue;
            }

            sdl_process_game_controller_button(
                &old_controller->action_down,
                &new_controller->action_down,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH));
            sdl_process_game_controller_button(
                &old_controller->action_up,
                &new_controller->action_up,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH));
            sdl_process_game_controller_button(
                &old_controller->action_left,
                &new_controller->action_left,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST));
            sdl_process_game_controller_button(
                &old_controller->action_right,
                &new_controller->action_right,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST));
            sdl_process_game_controller_button(
                &old_controller->shoulder_left,
                &new_controller->shoulder_left,
                SDL_GetGamepadButton(gamepad,
                                     SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
            sdl_process_game_controller_button(
                &old_controller->shoulder_right,
                &new_controller->shoulder_right,
                SDL_GetGamepadButton(gamepad,
                                     SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
            sdl_process_game_controller_button(
                &old_controller->back,
                &new_controller->back,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK));
            sdl_process_game_controller_button(
                &old_controller->start,
                &new_controller->start,
                SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START));

            int16_t axis_leftx =
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
            int16_t axis_lefty =
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
            new_controller->axis_leftx_average =
                sdl_process_game_controller_axis(axis_leftx,
                                                 GAMEPAD_AXIS_DEADZONE);
            new_controller->axis_lefty_average =
                sdl_process_game_controller_axis(axis_lefty,
                                                 GAMEPAD_AXIS_DEADZONE);

            new_controller->is_analog_movement =
                new_controller->axis_leftx_average != 0.0f ||
                new_controller->axis_lefty_average != 0.0f;
            if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP))
            {
                new_controller->axis_lefty_average = -1.0f;
                new_controller->is_analog_movement          = 0;
            }
            if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
            {
                new_controller->axis_lefty_average = 1.0f;
                new_controller->is_analog_movement          = 0;
            }
            if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
            {
                new_controller->axis_leftx_average = -1.0f;
                new_controller->is_analog_movement          = 0;
            }
            if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
            {
                new_controller->axis_leftx_average = 1.0f;
                new_controller->is_analog_movement          = 0;
            }

            float axis_threshold = 0.5f;
            sdl_process_game_controller_button(
                &old_controller->move_down,
                &new_controller->move_down,
                new_controller->axis_lefty_average > axis_threshold);
            sdl_process_game_controller_button(
                &old_controller->move_right,
                &new_controller->move_right,
                new_controller->axis_leftx_average > axis_threshold);

            sdl_process_game_controller_button(
                &old_controller->move_up,
                &new_controller->move_up,
                new_controller->axis_lefty_average < -axis_threshold);
            sdl_process_game_controller_button(
                &old_controller->move_left,
                &new_controller->move_left,
                new_controller->axis_leftx_average < -axis_threshold);
        }

        struct game_back_buffer buffer = {0};
        buffer.data                       = back_buffer.memory;
        buffer.width                        = back_buffer.width;
        buffer.height                       = back_buffer.height;
        buffer.pitch                        = back_buffer.pitch;
        game_update_and_render(&game_memory, &buffer, new_input);

        struct game_input *tmp_input = new_input;
        new_input                    = old_input;
        old_input                    = tmp_input;

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

        uint64_t end_counter = SDL_GetPerformanceCounter();
        SDL_UpdateTexture(
            back_buffer.texture, 0, back_buffer.memory, back_buffer.pitch);
        SDL_RenderTexture(renderer, back_buffer.texture, 0, 0);
        SDL_RenderPresent(renderer);
#if 1
        uint64_t counter_elapsed = end_counter - last_counter;
        double ms_per_frame =
            (1000.0f * (double)counter_elapsed) / (double)perf_count_freq;
        double fps = (double)perf_count_freq / (double)counter_elapsed;
        printf("%.02f ms/f, %.02ff/s\n", ms_per_frame, fps);
#endif

        last_counter = end_counter;
    } // end run loop

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
