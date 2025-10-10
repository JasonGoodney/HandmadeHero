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
realign_coordinate(struct world *world, u32 *tile, f32 *tile_rel)
{
    // TODO: Need to do something that doesn't use the divide/multiply method
    // for recanonicalizing because this this can end up rounding back on to
    // the tile you just came from

    // NOTE: World is assumed to be toroidal topology.
    // If you step off one end you come back on the other.

    f32 offset = round_f32_to_i32(*tile_rel / world->tile_side_meters);
    *tile += offset;
    *tile_rel -= offset * world->tile_side_meters;

    // check x/y within bounds of a tile
    ASSERT(*tile_rel >= -0.5f * world->tile_side_meters);
    ASSERT(*tile_rel <= 0.5f * world->tile_side_meters);
}

internal struct world_position
realign_position(struct world *world, struct world_position pos)
{
    struct world_position result = pos;

    realign_coordinate(world, &result.abs_tile_x, &result.tile_rel_x);
    realign_coordinate(world, &result.abs_tile_y, &result.tile_rel_y);

    return result;
}

internal u32
get_tile_value_unchecked(struct world *world,
                         struct tile_chunk *tile_chunk,
                         u32 tile_x,
                         u32 tile_y)
{
    ASSERT(tile_chunk);
    ASSERT(tile_x < world->chunk_dim);
    ASSERT(tile_y < world->chunk_dim);

    u32 tile_value = tile_chunk->tiles[tile_y * world->chunk_dim + tile_x];
    return tile_value;
}

internal u32
get_tile_value_checked(struct world *world,
                       struct tile_chunk *tile_chunk,
                       u32 test_tile_x,
                       u32 test_tile_y)
{
    u32 tile_value = 0;

    if (tile_chunk)
    {
        tile_value = get_tile_value_unchecked(
            world, tile_chunk, test_tile_x, test_tile_y);
    }
    return tile_value;
}

internal struct tile_chunk *
get_tile_chunk(struct world *world, i32 tile_chunk_x, i32 tile_chunk_y)
{
    struct tile_chunk *tile_chunk = NULL;

    b32 x_in_bounds =
        tile_chunk_x >= 0 && tile_chunk_x < world->tile_chunk_count_x;
    b32 y_in_bounds =
        tile_chunk_y >= 0 && tile_chunk_y < world->tile_chunk_count_y;
    if (x_in_bounds && y_in_bounds)
    {
        tile_chunk =
            &world->tile_chunks[tile_chunk_y * world->tile_chunk_count_x +
                                tile_chunk_x];
    }

    return tile_chunk;
}

internal struct tile_chunk_position
get_chunk_position_for(struct world *world, u32 abs_tile_x, u32 abs_tile_y)
{
    struct tile_chunk_position result;

    result.tile_chunk_x = abs_tile_x >> world->chunk_shift;
    result.tile_chunk_y = abs_tile_y >> world->chunk_shift;
    result.rel_tile_x   = abs_tile_x & world->chunk_mask;
    result.rel_tile_y   = abs_tile_y & world->chunk_mask;

    return result;
}

internal u32
get_tile_value(struct world *world, u32 abs_tile_x, u32 abs_tile_y)
{
    struct tile_chunk_position chunk_pos =
        get_chunk_position_for(world, abs_tile_x, abs_tile_y);
    struct tile_chunk *tile_chunk =
        get_tile_chunk(world, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y);

    u32 tile_value = get_tile_value_checked(
        world, tile_chunk, chunk_pos.rel_tile_x, chunk_pos.rel_tile_y);

    return tile_value;
}

internal b32
is_world_point_empty(struct world *world, struct world_position canon_pos)
{
    u32 tile_chunk_value =
        get_tile_value(world, canon_pos.abs_tile_x, canon_pos.abs_tile_y);
    b32 empty = (tile_chunk_value == 0);

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

#define TILEMAP_HEIGHT 256
#define TILEMAP_WIDTH 256
    u32 temp_tiles[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
         1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

    struct world world       = {0};
    world.chunk_shift        = 8;
    world.chunk_mask         = (1 << world.chunk_shift) - 1;
    world.chunk_dim          = 256;
    world.tile_chunk_count_x = 1;
    world.tile_chunk_count_y = 1;
    world.tile_side_meters   = 1.4f;
    world.tile_side_pixels   = 60;
    world.pixels_per_meter =
        (f32)world.tile_side_pixels / world.tile_side_meters;

    f32 lower_left_x = (f32)world.tile_side_pixels / 2;
    f32 lower_left_y = (f32)(world.tile_side_pixels * TILEMAP_HEIGHT) +
                       (f32)world.tile_side_pixels / 2;

    struct tile_chunk tile_chunk;
    tile_chunk.tiles  = (u32 *)temp_tiles;
    world.tile_chunks = &tile_chunk;

    f32 player_width  = 0.75f * (f32)world.tile_side_meters;
    f32 player_height = (f32)world.tile_side_meters;

    if (!memory->is_initialized)
    {
        game_state->player_speed = 2.0f;

        game_state->player_pos.abs_tile_x = 3;
        game_state->player_pos.abs_tile_y = 3;
        game_state->player_pos.tile_rel_x = 5.0f;
        game_state->player_pos.tile_rel_y = 5.0f;

        memory->is_initialized = 1;
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
                d_player_y = 1.0f;
            }
            if (controller->move_down.ended_pressed)
            {
                d_player_y = -1.0f;
            }
            if (controller->move_left.ended_pressed)
            {
                d_player_x = -1.0f;
            }
            if (controller->move_right.ended_pressed)
            {
                d_player_x = 1.0f;
            }

            if (controller->action_up.ended_pressed) {
                game_state->player_speed = 10.0f;
            }
            if (controller->action_down.ended_pressed) {
                game_state->player_speed = 2.0f;
            }
            d_player_x *= game_state->player_speed;
            d_player_y *= game_state->player_speed;

            // TODO: Strafing is fast. Will fix once we have vectors.
            struct world_position new_player_pos = game_state->player_pos;
            new_player_pos.tile_rel_x +=
                d_player_x * input->delta_time_for_frame;
            new_player_pos.tile_rel_y +=
                d_player_y * input->delta_time_for_frame;
            new_player_pos = realign_position(&world, new_player_pos);
            // TODO: Delta function that auto-recanonicalizes

            struct world_position player_pos_left = new_player_pos;
            player_pos_left.tile_rel_x -= 0.5f * player_width;
            player_pos_left = realign_position(&world, player_pos_left);

            struct world_position player_pos_right = new_player_pos;
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

    f32 screen_center_x = 0.5f * (f32)buffer->width;
    f32 screen_center_y = 0.5f * (f32)buffer->height;

    for (i32 rel_row = -10; rel_row < 10; rel_row++)
    {
        for (i32 rel_col = -20; rel_col < 20; rel_col++)
        {
            u32 col     = game_state->player_pos.abs_tile_x + rel_col;
            u32 row     = game_state->player_pos.abs_tile_y + rel_row;
            u32 tile_id = get_tile_value(&world, col, row);
            f32 gray    = 0.5f;
            if (tile_id == 1)
            {
                gray = 1.0f;
            }
            if (col == game_state->player_pos.abs_tile_x &&
                row == game_state->player_pos.abs_tile_y)
            {
                gray = 0.0f;
            }

            f32 cen_x = screen_center_x - (world.pixels_per_meter * game_state->player_pos.tile_rel_x) + (f32)rel_col * world.tile_side_pixels;
            f32 cen_y = screen_center_y + (world.pixels_per_meter * game_state->player_pos.tile_rel_y) - (f32)rel_row * world.tile_side_pixels;
            f32 min_x = cen_x - 0.5f * world.tile_side_pixels;
            f32 min_y = cen_y - 0.5f * world.tile_side_pixels;
            f32 max_x = cen_x + 0.5f * world.tile_side_pixels;
            f32 max_y = cen_y + 0.5f * world.tile_side_pixels;
            render_rectangle(
                buffer, min_x, min_y, max_x, max_y, gray, gray, gray);
        }
    }

    f32 player_left =
        screen_center_x -
        (world.pixels_per_meter * player_width * 0.5f);
    f32 player_top =
        screen_center_y -
        (world.pixels_per_meter * player_height);
    f32 player_right = player_left + (world.pixels_per_meter * player_width);
    f32 player_bottom = player_top + (world.pixels_per_meter * player_height);
    render_rectangle(buffer,
                     player_left,
                     player_top,
                     player_right,
                     player_bottom,
                     1.0f,
                     1.0f,
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
