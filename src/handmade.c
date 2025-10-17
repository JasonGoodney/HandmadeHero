#include "handmade.h"
#include "handmade_math.h"
#include "handmade_random.h"
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
    u32 c0    = 255 << 24;
    u32 c1    = round_f32_to_u32(r * 255.0f) << 16;
    u32 c2    = round_f32_to_u32(g * 255.0f) << 8;
    u32 c3    = round_f32_to_u32(b * 255.0f) << 0;
    u32 color = (c0 | c1 | c2 | c3);

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
draw_bitmap(struct game_back_buffer *buffer, LoadedBitmap *bitmap, f32 x, f32 y)
{

    int min_x = round_f32_to_i32(x);
    int min_y = round_f32_to_i32(y);
    int max_x = round_f32_to_i32(x + (f32)bitmap->width);
    int max_y = round_f32_to_i32(y + (f32)bitmap->height);

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

    u32 *source_row = bitmap->pixels + (bitmap->width * (bitmap->height - 1));
    u8 *dest_row    = (u8 *)buffer->data + (min_x * buffer->bytes_per_pixel) +
                   (min_y * buffer->pitch);
    for (int y = min_y; y < max_y; y++)
    {
        u32 *dest   = (u32 *)dest_row;
        u32 *source = source_row;
        for (int x = min_x; x < max_x; x++)
        {
            *dest++ = *source++;
        }

        dest_row += buffer->pitch;
        source_row -= bitmap->width;
    }
}

#pragma pack(push, 1)
typedef struct bitmap_header
{
    u16 file_type;
    u32 file_size;
    u16 reserved1;
    u16 reserved2;
    u32 bitmap_offset;

    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bits_per_pixel;

    u32 compression;
    u32 size_of_bitmap;
    i32 horizontal_resolution;
    i32 vertical_resolution;
    u32 colors_used;
    u32 colors_important;

    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
} BitmapHeader;
#pragma pack(pop)

#if HANDMADE_INTERNAL
internal LoadedBitmap
debug_load_bmp(ThreadContext *context,
               debug_platform_read_file_f *read_file,
               char *filename)
{
    LoadedBitmap result = {0};

    // NOTE: Byte order in memory is AABBGGRR, bottom up
    // // NOTE: Byte order in memory is AARRGGBB, bottom up

    struct debug_read_file_result read_result = read_file(context, filename);
    if (read_result.data)
    {
        BitmapHeader *header = (BitmapHeader *)read_result.data;
        u32 *pixels   = (u32 *)((u8 *)read_result.data + header->bitmap_offset);
        result.pixels = pixels;
        result.width  = header->width;
        result.height = header->height;

        // NOTE: If you are using the generically for some reason,
        // please remember the BMP files can go in either direction
        // and the height will be negative fro top-down.
        // (Also, there can be compression, etc., etc.,... do not think
        // this is complete BMP loading code)

        u32 *source_dest = pixels;
        for (i32 y = 0; y < header->height; y++)
        {
            for (i32 x = 0; x < header->width; x++)
            {
                *source_dest = (*source_dest >> 8) | (*source_dest << 24);
                source_dest++;
            }
        }
    }

    return result;
}
#endif

// TODO: handle endianess for pixel buffer based on OS
GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    ASSERT(sizeof(struct game_state) <= memory->permanent_size);
    ASSERT((&input->controllers[0].terminator -
            &input->controllers[0].buttons[0]) ==
           (ARRAY_SIZE(input->controllers[0].buttons)));

    struct game_state *game_state = (struct game_state *)memory->permanent;

    if (!memory->is_initialized)
    {
#if HANDMADE_INTERNAL
        ThreadContext thread = {0};
        // game_state->backdrop = debug_load_bmp(&thread,
        //                                       memory->debug_platform_read_file,
        //                                       "../../data/structured_art.bmp");
        game_state->backdrop =
            debug_load_bmp(&thread,
                           memory->debug_platform_read_file,
                           "../../data/test/test_background.bmp");
        game_state->hero_head =
            debug_load_bmp(&thread,
                           memory->debug_platform_read_file,
                           "../../data/test/test_hero_front_head.bmp");
        game_state->hero_cape =
            debug_load_bmp(&thread,
                           memory->debug_platform_read_file,
                           "../../data/test/test_hero_front_cape.bmp");
        game_state->hero_torso =
            debug_load_bmp(&thread,
                           memory->debug_platform_read_file,
                           "../../data/test/test_hero_front_torso.bmp");

#endif
        game_state->player_speed = 2.0f;

        game_state->player_pos.abs_tile_x = 1;
        game_state->player_pos.abs_tile_y = 1;
        game_state->player_pos.abs_tile_z = 0;
        game_state->player_pos.offset_x   = 5.0f;
        game_state->player_pos.offset_y   = 5.0f;

        size_t game_state_size = sizeof(struct game_state);
        initialize_arena(&game_state->world_arena,
                         (memory->permanent_size - game_state_size),
                         (u8 *)(memory->permanent + game_state_size));

        game_state->world = PUSH_SIZE(&game_state->world_arena, World);
        World *world      = game_state->world;
        world->tile_map =
            (TileMap *)PUSH_SIZE(&game_state->world_arena, TileMap);

        TileMap *tile_map            = world->tile_map;
        tile_map->chunk_shift        = 4;
        tile_map->chunk_mask         = (1 << tile_map->chunk_shift) - 1;
        tile_map->chunk_dim          = (1 << tile_map->chunk_shift);
        tile_map->tile_chunk_count_x = 128;
        tile_map->tile_chunk_count_y = 128;
        tile_map->tile_chunk_count_z = 2;
        tile_map->tile_side_meters   = 1.4f;

        tile_map->tile_chunks = PUSH_ARRAY(
            &game_state->world_arena,
            (tile_map->tile_chunk_count_x * tile_map->tile_chunk_count_y *
             tile_map->tile_chunk_count_z),
            TileChunk);

        u32 tiles_per_width     = 17;
        u32 tiles_per_height    = 9;
        u32 screen_x            = 0;
        u32 screen_y            = 0;
        u32 abs_tile_z          = 0;
        u32 random_number_index = 0;

        // TODO: Replace all this with real world generation!
        b32 door_left   = 0;
        b32 door_right  = 0;
        b32 door_top    = 0;
        b32 door_bottom = 0;
        b32 door_up     = 0;
        b32 door_down   = 0;
        for (u32 screen_index = 0; screen_index < 100; screen_index++)
        {
            ASSERT(random_number_index < ARRAY_SIZE(random_number_table));
            u32 random_choice;
            b32 created_z_plane = 0;
            if (door_up || door_down)
            {
                random_choice = random_number_table[random_number_index++] % 2;
            }
            else
            {
                random_choice = random_number_table[random_number_index++] % 3;
            }

            if (random_choice == 2)
            {
                created_z_plane = 1;
                if (abs_tile_z == 0)
                {
                    door_up = 1;
                }
                else
                {
                    door_down = 1;
                }
            }
            else if (random_choice == 1)
            {
                door_right = 1;
            }
            else
            {
                door_top = 1;
            }

            for (u32 tile_y = 0; tile_y < tiles_per_height; tile_y++)
            {
                for (u32 tile_x = 0; tile_x < tiles_per_width; tile_x++)
                {
                    u32 abs_tile_x = screen_x * tiles_per_width + tile_x;
                    u32 abs_tile_y = screen_y * tiles_per_height + tile_y;

                    u32 tile_value = 1;
                    if ((tile_x == 0) &&
                        (!door_left || (tile_y != tiles_per_height / 2)))
                    {
                        tile_value = 2;
                    }

                    if ((tile_x == tiles_per_width - 1) &&
                        (!door_right || (tile_y != tiles_per_height / 2)))
                    {
                        tile_value = 2;
                    }

                    if ((tile_y == 0) &&
                        (!door_bottom || (tile_x != tiles_per_width / 2)))
                    {
                        tile_value = 2;
                    }

                    if ((tile_y == tiles_per_height - 1) &&
                        (!door_top || (tile_x != tiles_per_width / 2)))
                    {
                        tile_value = 2;
                    }

                    if (tile_x == 10 && tile_y == 6)
                    {
                        if (door_up)
                        {
                            tile_value = 3;
                        }
                        else if (door_down)
                        {
                            tile_value = 4;
                        }
                    }

                    set_tile_value(&game_state->world_arena,
                                   tile_map,
                                   abs_tile_x,
                                   abs_tile_y,
                                   abs_tile_z,
                                   tile_value);
                }
            }

            door_left   = door_right;
            door_bottom = door_top;
            door_right  = 0;
            door_top    = 0;

            if (created_z_plane)
            {
                door_down = !door_down;
                door_up   = !door_up;
            }
            else
            {
                door_down = 0;
                door_up   = 0;
            }

            if (random_choice == 2)
            {
                if (abs_tile_z == 0)
                {
                    abs_tile_z = 1;
                }
                else
                {
                    abs_tile_z = 0;
                }
            }
            else if (random_choice == 1)
            {
                screen_x += 1;
            }
            else
            {
                screen_y += 1;
            }
        }

        game_state->player_width  = 0.75f * (f32)tile_map->tile_side_meters;
        game_state->player_height = (f32)tile_map->tile_side_meters;

        memory->is_initialized = 1;
    }

    f32 player_width  = game_state->player_width;
    f32 player_height = game_state->player_height;
    World *world      = game_state->world;
    TileMap *tile_map = world->tile_map;

    i32 tile_side_pixels = 60;
    u32 pixels_per_meter = (f32)tile_side_pixels / tile_map->tile_side_meters;

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

            if (controller->action_up.ended_pressed)
            {
                game_state->player_speed = 10.0f;
            }
            if (controller->action_down.ended_pressed)
            {
                game_state->player_speed = 2.0f;
            }
            d_player_x *= game_state->player_speed;
            d_player_y *= game_state->player_speed;

            // TODO: Strafing is fast. Will fix once we have vectors.
            TileMapPosition new_player_pos = game_state->player_pos;
            new_player_pos.offset_x += d_player_x * input->delta_time_for_frame;
            new_player_pos.offset_y += d_player_y * input->delta_time_for_frame;
            new_player_pos = realign_position(tile_map, new_player_pos);
            // TODO: Delta function that auto-recanonicalizes

            TileMapPosition player_pos_left = new_player_pos;
            player_pos_left.offset_x -= 0.5f * player_width;
            player_pos_left = realign_position(tile_map, player_pos_left);

            TileMapPosition player_pos_right = new_player_pos;
            player_pos_right.offset_x += 0.5f * player_width;
            player_pos_right = realign_position(tile_map, player_pos_right);

            b32 empty_tile = is_tile_map_point_empty(tile_map, new_player_pos);
            empty_tile &= is_tile_map_point_empty(tile_map, player_pos_left);
            empty_tile &= is_tile_map_point_empty(tile_map, player_pos_right);
            if (empty_tile)
            {
                if (!on_same_tile(&game_state->player_pos, &new_player_pos))
                {
                    u32 new_tile_value =
                        get_tile_value_from_position(tile_map, new_player_pos);
                    if (new_tile_value == 3)
                    {
                        new_player_pos.abs_tile_z++;
                    }
                    else if (new_tile_value == 4)
                    {
                        new_player_pos.abs_tile_z--;
                    }
                }
                game_state->player_pos = new_player_pos;
            }
        }
    }

    draw_bitmap(buffer, &game_state->backdrop, 0, 0);

    f32 screen_center_x = 0.5f * (f32)buffer->width;
    f32 screen_center_y = 0.5f * (f32)buffer->height;

    for (i32 rel_row = -10; rel_row < 10; rel_row++)
    {
        for (i32 rel_col = -20; rel_col < 20; rel_col++)
        {
            u32 col     = game_state->player_pos.abs_tile_x + rel_col;
            u32 row     = game_state->player_pos.abs_tile_y + rel_row;
            u32 tile_id = get_tile_value(
                tile_map, col, row, game_state->player_pos.abs_tile_z);
            if (tile_id > 1)
            {
                f32 gray = 0.5f;
                if (tile_id == 2)
                {
                    gray = 1.0f;
                }
                if (tile_id > 2)
                {
                    gray = 0.25f;
                }
                if (col == game_state->player_pos.abs_tile_x &&
                    row == game_state->player_pos.abs_tile_y)
                {
                    gray = 0.0f;
                }

                f32 cen_x =
                    screen_center_x -
                    (pixels_per_meter * game_state->player_pos.offset_x) +
                    (f32)rel_col * tile_side_pixels;
                f32 cen_y =
                    screen_center_y +
                    (pixels_per_meter * game_state->player_pos.offset_y) -
                    (f32)rel_row * tile_side_pixels;
                f32 min_x = cen_x - 0.5f * tile_side_pixels;
                f32 min_y = cen_y - 0.5f * tile_side_pixels;
                f32 max_x = cen_x + 0.5f * tile_side_pixels;
                f32 max_y = cen_y + 0.5f * tile_side_pixels;
                render_rectangle(
                    buffer, min_x, min_y, max_x, max_y, gray, gray, gray);
            }
        }
    }

    f32 player_left =
        screen_center_x - (pixels_per_meter * player_width * 0.5f);
    f32 player_top    = screen_center_y - (pixels_per_meter * player_height);
    f32 player_right  = player_left + (pixels_per_meter * player_width);
    f32 player_bottom = player_top + (pixels_per_meter * player_height);

    render_rectangle(buffer,
                     player_left,
                     player_top,
                     player_right,
                     player_bottom,
                     1.0f,
                     1.0f,
                     0.0f);

    // draw_bitmap(buffer, &game_state->hero_head, player_left, player_top);
    draw_bitmap(buffer, &game_state->hero_head, 0, 0);

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
