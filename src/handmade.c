#include "handmade.h"
#include "handmade_math.h"
#include "handmade_tile.c"

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

    TileMap tile_map       = {0};
    tile_map.chunk_shift        = 8;
    tile_map.chunk_mask         = (1 << tile_map.chunk_shift) - 1;
    tile_map.chunk_dim          = 256;
    tile_map.tile_chunk_count_x = 1;
    tile_map.tile_chunk_count_y = 1;
    tile_map.tile_side_meters   = 1.4f;
    tile_map.tile_side_pixels   = 60;
    tile_map.pixels_per_meter =
        (f32)tile_map.tile_side_pixels / tile_map.tile_side_meters;

    World world = {0};
    world.tile_map = &tile_map;

    f32 lower_left_x = (f32)tile_map.tile_side_pixels / 2;
    f32 lower_left_y = (f32)(tile_map.tile_side_pixels * TILEMAP_HEIGHT) +
                       (f32)tile_map.tile_side_pixels / 2;

    TileChunk tile_chunk;
    tile_chunk.tiles  = (u32 *)temp_tiles;
    tile_map.tile_chunks = &tile_chunk;

    f32 player_width  = 0.75f * (f32)tile_map.tile_side_meters;
    f32 player_height = (f32)tile_map.tile_side_meters;

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
            TileMapPosition new_player_pos = game_state->player_pos;
            new_player_pos.tile_rel_x +=
                d_player_x * input->delta_time_for_frame;
            new_player_pos.tile_rel_y +=
                d_player_y * input->delta_time_for_frame;
            new_player_pos = realign_position(&tile_map, new_player_pos);
            // TODO: Delta function that auto-recanonicalizes

            TileMapPosition player_pos_left = new_player_pos;
            player_pos_left.tile_rel_x -= 0.5f * player_width;
            player_pos_left = realign_position(&tile_map, player_pos_left);

            TileMapPosition player_pos_right = new_player_pos;
            player_pos_right.tile_rel_x += 0.5f * player_width;
            player_pos_right = realign_position(&tile_map, player_pos_right);

            b32 empty_tile = is_tile_map_point_empty(&tile_map, new_player_pos);
            empty_tile &= is_tile_map_point_empty(&tile_map, player_pos_left);
            empty_tile &= is_tile_map_point_empty(&tile_map, player_pos_right);
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
            u32 tile_id = get_tile_value(&tile_map, col, row);
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

            f32 cen_x = screen_center_x - (tile_map.pixels_per_meter * game_state->player_pos.tile_rel_x) + (f32)rel_col * tile_map.tile_side_pixels;
            f32 cen_y = screen_center_y + (tile_map.pixels_per_meter * game_state->player_pos.tile_rel_y) - (f32)rel_row * tile_map.tile_side_pixels;
            f32 min_x = cen_x - 0.5f * tile_map.tile_side_pixels;
            f32 min_y = cen_y - 0.5f * tile_map.tile_side_pixels;
            f32 max_x = cen_x + 0.5f * tile_map.tile_side_pixels;
            f32 max_y = cen_y + 0.5f * tile_map.tile_side_pixels;
            render_rectangle(
                buffer, min_x, min_y, max_x, max_y, gray, gray, gray);
        }
    }

    f32 player_left =
        screen_center_x -
        (tile_map.pixels_per_meter * player_width * 0.5f);
    f32 player_top =
        screen_center_y -
        (tile_map.pixels_per_meter * player_height);
    f32 player_right = player_left + (tile_map.pixels_per_meter * player_width);
    f32 player_bottom = player_top + (tile_map.pixels_per_meter * player_height);
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
