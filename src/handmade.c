#include "handmade.h"
#include "handmade_math.h"

internal void
render_rectangle(struct game_back_buffer *buffer,
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

internal void
realign_coordinate(
    struct world *world, i32 tile_count, i32 *tilemap, i32 *tile, f32 *tile_rel)
{
    f32 offset = floor_f32_to_i32(*tile_rel / world->tile_side_meters);
    *tile += offset;
    *tile_rel -= offset * world->tile_side_meters;

    // check x/y within bounds of a tile
    ASSERT(*tile_rel >= 0);
    ASSERT(*tile_rel < world->tile_side_meters);

    if (*tile < 0)
    {
        *tile += tile_count;
        *tilemap -= 1;
    }
    if (*tile >= tile_count)
    {
        *tile -= tile_count;
        *tilemap += 1;
    }
}

internal struct canonical_position
realign_position(struct world *world, struct canonical_position pos)
{
    struct canonical_position result = pos;

    realign_coordinate(world,
                       world->tilemap_width,
                       &result.tilemap_x,
                       &result.tile_x,
                       &result.tile_rel_x);

    realign_coordinate(world,
                       world->tilemap_height,
                       &result.tilemap_y,
                       &result.tile_y,
                       &result.tile_rel_y);
    return result;
}

internal u32
get_tile_value_unsafe(struct world *world,
                      struct tilemap *tilemap,
                      i32 tile_x,
                      i32 tile_y)
{
    ASSERT(tilemap);
    ASSERT(tile_x >= 0 && tile_x < world->tilemap_width);
    ASSERT(tile_y >= 0 && tile_y < world->tilemap_height);

    return tilemap->tiles[tile_y * world->tilemap_width + tile_x];
}

internal struct tilemap *
get_tilemap(struct world *world, i32 tilemap_x, i32 tilemap_y)
{
    struct tilemap *tilemap = NULL;

    b32 x_in_bounds = tilemap_x >= 0 && tilemap_x < world->width;
    b32 y_in_bounds = tilemap_y >= 0 && tilemap_y < world->height;
    if (x_in_bounds && y_in_bounds)
    {
        tilemap = &world->tilemaps[tilemap_y * world->width + tilemap_x];
    }

    return tilemap;
}

internal b32
is_tilemap_point_empty(struct world *world,
                       struct tilemap *tilemap,
                       f32 test_tile_x,
                       f32 test_tile_y)
{
    b32 empty = 0;

    if (tilemap)
    {
        b32 x_in_bounds =
            test_tile_x >= 0 && test_tile_x < world->tilemap_width;
        b32 y_in_bounds =
            test_tile_y >= 0 && test_tile_y < world->tilemap_height;
        if (x_in_bounds && y_in_bounds)
        {
            empty = (get_tile_value_unsafe(
                         world, tilemap, test_tile_x, test_tile_y) == 0);
        }
    }
    return empty;
}

internal b32
is_world_point_empty(struct world *world, struct canonical_position canon_pos)
{
    b32 empty = 0;

    struct tilemap *tilemap =
        get_tilemap(world, canon_pos.tilemap_x, canon_pos.tilemap_y);
    empty = is_tilemap_point_empty(
        world, tilemap, canon_pos.tile_x, canon_pos.tile_y);

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

#define TILEMAP_HEIGHT 9
#define TILEMAP_WIDTH 17
    u32 tilemap00[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    u32 tilemap01[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
    u32 tilemap10[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}};
    u32 tilemap11[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

    struct world world     = {0};
    world.width            = 2;
    world.height           = 2;
    world.tile_side_meters = 1.4f;
    world.tile_side_pixels = 45;
    world.pixels_per_meter =
        (f32)world.tile_side_pixels / world.tile_side_meters;
    world.upper_left_x   = (f32)world.tile_side_pixels / 2;
    world.upper_left_y   = (f32)world.tile_side_pixels / 2;
    world.tilemap_width  = TILEMAP_WIDTH;
    world.tilemap_height = TILEMAP_HEIGHT;

    struct tilemap tilemaps[world.height][world.width];
    tilemaps[0][0].tiles = (u32 *)tilemap00;
    tilemaps[0][1].tiles = (u32 *)tilemap10;
    tilemaps[1][0].tiles = (u32 *)tilemap01;
    tilemaps[1][1].tiles = (u32 *)tilemap11;

    world.tilemaps = (struct tilemap *)tilemaps;

    f32 player_width  = 0.75f * (f32)world.tile_side_meters;
    f32 player_height = (f32)world.tile_side_meters;

    if (!memory->is_initialized)
    {
        memory->is_initialized            = 1;
        game_state->player_pos.tilemap_x  = 0;
        game_state->player_pos.tilemap_y  = 0;
        game_state->player_pos.tile_x     = 3;
        game_state->player_pos.tile_y     = 3;
        game_state->player_pos.tile_rel_x = 5.0f;
        game_state->player_pos.tile_rel_y = 5.0f;
    }

    struct tilemap *tilemap = get_tilemap(&world,
                                          game_state->player_pos.tilemap_x,
                                          game_state->player_pos.tilemap_y);
    ASSERT(tilemap);

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
            d_player_x *= 2.0f;
            d_player_y *= 2.0f;

            struct canonical_position new_player_pos = game_state->player_pos;
            new_player_pos.tile_rel_x +=
                d_player_x * input->delta_time_for_frame;
            new_player_pos.tile_rel_y +=
                d_player_y * input->delta_time_for_frame;
            new_player_pos = realign_position(&world, new_player_pos);

            struct canonical_position player_pos_left = new_player_pos;
            player_pos_left.tile_rel_x -= 0.5f * player_width;
            player_pos_left = realign_position(&world, player_pos_left);

            struct canonical_position player_pos_right = new_player_pos;
            player_pos_right.tile_rel_x += 0.5f * player_width;
            player_pos_right = realign_position(&world, player_pos_right);

            b32 empty_tile = is_world_point_empty(&world, new_player_pos);
            empty_tile &= is_world_point_empty(&world, player_pos_left);
            empty_tile &= is_world_point_empty(&world, player_pos_right);
            if (empty_tile)
            {
                game_state->player_pos = new_player_pos;
            }
        }
    }

    render_rectangle(buffer,
                     0.0f,
                     0.0f,
                     (f32)buffer->width,
                     (f32)buffer->height,
                     0.0f,
                     0.0f,
                     0.0f);

    for (int row = 0; row < world.tilemap_height; row++)
    {
        for (int col = 0; col < world.tilemap_width; col++)
        {
            u32 tile_id = get_tile_value_unsafe(&world, tilemap, col, row);
            f32 gray    = 0.5f;
            if (tile_id == 1)
            {
                gray = 1.0f;
            }
            if (col == game_state->player_pos.tile_x &&
                row == game_state->player_pos.tile_y)
            {
                gray = 0.0f;
            }
            f32 min_x = world.upper_left_x + (f32)col * world.tile_side_pixels;
            f32 min_y = world.upper_left_y + (f32)row * world.tile_side_pixels;
            f32 max_x = min_x + world.tile_side_pixels;
            f32 max_y = min_y + world.tile_side_pixels;
            render_rectangle(
                buffer, min_x, min_y, max_x, max_y, gray, gray, gray);
        }
    }

    f32 player_min_x =
        world.upper_left_x +
        world.tile_side_pixels * game_state->player_pos.tile_x +
        world.pixels_per_meter * game_state->player_pos.tile_rel_x -
        (world.pixels_per_meter * player_width * 0.5f);
    f32 player_min_y =
        world.upper_left_y +
        world.tile_side_pixels * game_state->player_pos.tile_y +
        world.pixels_per_meter * game_state->player_pos.tile_rel_y -
        (world.pixels_per_meter * player_height);
    f32 player_max_x = player_min_x + (world.pixels_per_meter * player_width);
    f32 player_max_y = player_min_y + (world.pixels_per_meter * player_height);
    render_rectangle(buffer,
                     player_min_x,
                     player_min_y,
                     player_max_x,
                     player_max_y,
                     0.0f,
                     1.0f,
                     1.0f);

    // render black border around world
    render_rectangle(buffer,
                     world.upper_left_x,
                     0.0f,
                     world.upper_left_x +
                         world.tilemap_width * world.tile_side_pixels,
                     world.upper_left_y,
                     0.0f,
                     0.0f,
                     0.0f);
    render_rectangle(buffer,
                     0.0f,
                     0.0f,
                     world.upper_left_x,
                     world.upper_left_y +
                         world.tilemap_height * world.tile_side_pixels,
                     0.0f,
                     0.0f,
                     0.0f);
    render_rectangle(buffer,
                     world.upper_left_x +
                         world.tilemap_width * world.tile_side_pixels,
                     0.0f,
                     buffer->width,
                     buffer->height,
                     0.0f,
                     0.0f,
                     0.0f);
    render_rectangle(
        buffer,
        0.0f,
        world.upper_left_y + world.tilemap_height * world.tile_side_pixels,
        world.upper_left_x + world.tilemap_width * world.tile_side_pixels,
        buffer->height,
        0.0f,
        0.0f,
        0.0f);
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
