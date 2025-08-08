#include "handmade.h"
#include <math.h>

internal void render_rectangle(struct game_back_buffer *buffer,
                               f32 minx,
                               f32 miny,
                               f32 maxx,
                               f32 maxy,
                               f32 r,
                               f32 g,
                               f32 b)
{
    int min_x = round_f32_to_i32(minx);
    int min_y = round_f32_to_i32(miny);
    int max_x = round_f32_to_i32(maxx);
    int max_y = round_f32_to_i32(maxy);

    if (min_x < 0)
    {
        min_x = 0;
    }
    if (min_y < 0)
    {
        min_y = 0;
    }
    if (max_x > buffer->width)
    {
        max_x = buffer->width;
    }
    if (max_y > buffer->height)
    {
        max_y = buffer->height;
    }

    u8 *row = (u8 *)buffer->data + (min_x * buffer->bytes_per_pixel) +
              (min_y * buffer->pitch);
    uint32_t color =
        (255 << 24 | round_f32_to_u32(b * 255.0f) << 16 |
         round_f32_to_u32(g * 255.0f) << 8 | round_f32_to_u32(r * 255.0f) << 0);

    for (int y = min_y; y < max_y; y++)
    {
        u32 *pixel = (u32 *)row;
        for (int x = min_x; x < max_x; x++)
        {
            *(u32 *)pixel = color;
            pixel++;
        }
        row += buffer->pitch;
    }
}

struct tilemap
{
    f32 upper_left_x;
    f32 upper_left_y;
    i32 width;
    i32 height;
    u32 *tiles;
    u32 tile_width;
    u32 tile_height;
};
struct world
{
    i32 width;
    i32 height;
    struct tilemap *tilemaps;
};

internal u32 get_tile_value_unsafe(struct tilemap *tilemap,
                                   i32 tile_x,
                                   i32 tile_y)
{
    return tilemap->tiles[tile_y * tilemap->width + tile_x];
}

internal struct tilemap *
get_tilemap(struct world *world, i32 tilemap_x, i32 tilemap_y)
{
    struct tilemap *tilemap = NULL;

    if ((tilemap_x >= 0 && tilemap_x < world->width) &&
        (tilemap_y >= 0 && tilemap_y < world->height))
    {
        tilemap = &world->tilemaps[tilemap_y * world->width + tilemap_x];
    }

    return tilemap;
}

internal b32 is_tilemap_point_empty(struct tilemap *tilemap,
                                    f32 test_x,
                                    f32 test_y)
{
    b32 empty = 0;

    i32 tile_x = truncate_f32_to_i32((test_x - tilemap->upper_left_x) /
                                     tilemap->tile_width);
    i32 tile_y = truncate_f32_to_i32((test_y - tilemap->upper_left_y) /
                                     tilemap->tile_height);

    if (tile_x >= 0 && tile_x < tilemap->width && tile_y >= 0 &&
        tile_y < tilemap->height)
    {
        empty = (get_tile_value_unsafe(tilemap, tile_x, tile_y) == 0);
    }
    return empty;
}

internal b32 is_world_point_empty(
    struct world *world, i32 tilemap_x, i32 tilemap_y, f32 test_x, f32 test_y)
{
    b32 empty               = 0;
    struct tilemap *tilemap = get_tilemap(world, tilemap_x, tilemap_y);
    if (tilemap)
    {
        empty = is_tilemap_point_empty(tilemap, test_x, test_y);
    }
    return empty;
}

// TODO: handle endianess for pixel buffer based on OS
GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    ASSERT(sizeof(struct game_state) <= memory->permanent_size);
    ASSERT((&input->controllers[0].terminator -
            &input->controllers[0].buttons[0]) ==
           (ARRAY_SIZE(input->controllers[0].buttons)));

    struct game_state *game_state = (struct game_state *)memory->permanent;

#define tile_map_height 9
#define tile_map_width 17
    u32 tilemap00[tile_map_height][tile_map_width] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    u32 tilemap01[tile_map_height][tile_map_width] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
    u32 tilemap10[tile_map_height][tile_map_width] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    u32 tilemap11[tile_map_height][tile_map_width] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

    struct tilemap tilemaps[2][2];
    tilemaps[0][0].upper_left_x = 0.0f;
    tilemaps[0][0].upper_left_y = 0.0f;
    tilemaps[0][0].tile_width   = 60.0f;
    tilemaps[0][0].tile_height  = 60.0f;
    tilemaps[0][0].height       = tile_map_height;
    tilemaps[0][0].width        = tile_map_width;
    tilemaps[0][0].tiles        = (u32 *)tilemap00;

    tilemaps[0][1]       = tilemaps[0][0];
    tilemaps[0][1].tiles = (u32 *)tilemap01;
    tilemaps[1][0]       = tilemaps[0][0];
    tilemaps[1][0].tiles = (u32 *)tilemap01;
    tilemaps[1][1]       = tilemaps[0][0];
    tilemaps[1][1].tiles = (u32 *)tilemap01;

    struct tilemap *tilemap = &tilemaps[0][0];

    struct world world = {0};
    world.width        = 2;
    world.height       = 2;
    world.tilemaps     = (struct tilemap *)tilemaps;

    f32 player_width  = 0.75f * tilemap->tile_width;
    f32 player_height = 0.75f * tilemap->tile_height;

    if (!memory->is_initialized)
    {
        memory->is_initialized = 1;
        game_state->player_x   = tilemap->tile_width * 3;
        game_state->player_y   = tilemap->tile_height * 5;
    }

    // Input
    int controller_count = ARRAY_SIZE(input->controllers);
    for (int i = 0; i < controller_count; i++)
    {
        struct game_controller_input *controller = &input->controllers[i];
        if (!controller->is_connected)
            continue;

        if (controller->is_analog_movement)
        {
        }
        else
        {
            f32 d_player_x = 0.0f;
            f32 d_player_y = 0.0f;
            if (controller->move_up.ended_pressed)
            {
                d_player_y += -1.0f;
            }
            if (controller->move_down.ended_pressed)
            {
                d_player_y += 1.0f;
            }
            if (controller->move_left.ended_pressed)
            {
                d_player_x += -1.0f;
            }
            if (controller->move_right.ended_pressed)
            {
                d_player_x += 1.0f;
            }
            d_player_x *= 128.0f;
            d_player_y *= 128.0f;
            f32 new_player_x =
                game_state->player_x + d_player_x * input->delta_time_for_frame;
            f32 new_player_y =
                game_state->player_y + d_player_y * input->delta_time_for_frame;

            b32 empty_tile =
                is_tilemap_point_empty(tilemap, new_player_x, new_player_y);
            empty_tile &= is_tilemap_point_empty(
                tilemap, new_player_x + 0.5f * player_width, new_player_y);
            empty_tile &= is_tilemap_point_empty(
                tilemap, new_player_x - 0.5f * player_width, new_player_y);
            if (empty_tile)
            {
                game_state->player_x = new_player_x;
                game_state->player_y = new_player_y;
            }
        }
    }

    render_rectangle(buffer,
                     0.0f,
                     0.0f,
                     (f32)buffer->width,
                     (f32)buffer->height,
                     1.0f,
                     0.0f,
                     1.0f);

    for (int row = 0; row < tile_map_height; row++)
    {
        for (int col = 0; col < tile_map_width; col++)
        {
            u32 tile_id = get_tile_value_unsafe(tilemap, col, row);
            f32 gray    = 0.5f;
            if (tile_id == 1)
            {
                gray = 1.0f;
            }
            f32 min_x = tilemap->upper_left_x + (f32)col * tilemap->tile_width;
            f32 min_y = tilemap->upper_left_y + (f32)row * tilemap->tile_height;
            f32 max_x = min_x + tilemap->tile_width;
            f32 max_y = min_y + tilemap->tile_height;
            render_rectangle(
                buffer, min_x, min_y, max_x, max_y, gray, gray, gray);
        }
    }

    f32 min_x = game_state->player_x - (player_width * 0.5f);
    f32 min_y = game_state->player_y - (player_height);
    f32 max_x = min_x + (player_width);
    f32 max_y = min_y + (player_height);
    render_rectangle(buffer, min_x, min_y, max_x, max_y, 0.0f, 1.0f, 1.0f);

    // Audio
#if 0
    if (audio_buffer)
    {
        i32 volume = 3000;
        f32 wave_period =
            (f32)audio_buffer->sample_rate_khz / game_state->tone_hz;
        i16 *sample_out = audio_buffer->samples;

        for (int i = 0; i < audio_buffer->sample_count; i++)
        {
#if 1
            i16 sample = sinf(game_state->t_sine) * volume;
#else
            i16 sample = 0;
#endif
            *sample_out = sample;
            sample_out++;
            *sample_out = sample;
            sample_out++;

            game_state->t_sine += (2.0f * PI_F32 * 1.0f) / (f32)wave_period;
        }
    }
#endif
}
