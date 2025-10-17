#include "handmade_tile.h"
#include "handmade.h"
#include "handmade_math.h"

internal u32
get_tile_value_unchecked(TileMap *tile_map,
                         TileChunk *tile_chunk,
                         u32 tile_x,
                         u32 tile_y)
{
    ASSERT(tile_chunk);
    ASSERT(tile_x < tile_map->chunk_dim);
    ASSERT(tile_y < tile_map->chunk_dim);

    u32 tile_value = tile_chunk->tiles[tile_y * tile_map->chunk_dim + tile_x];
    return tile_value;
}

internal void
set_tile_value_unchecked(TileMap *tile_map,
                         TileChunk *tile_chunk,
                         u32 tile_x,
                         u32 tile_y,
                         u32 tile_value)
{
    ASSERT(tile_chunk);
    ASSERT(tile_x < tile_map->chunk_dim);
    ASSERT(tile_y < tile_map->chunk_dim);

    tile_chunk->tiles[tile_y * tile_map->chunk_dim + tile_x] = tile_value;
}

internal u32
get_tile_value_checked(TileMap *tile_map,
                       TileChunk *tile_chunk,
                       u32 test_tile_x,
                       u32 test_tile_y)
{
    u32 tile_value = 0;

    if (tile_chunk && tile_chunk->tiles)
    {
        tile_value = get_tile_value_unchecked(
            tile_map, tile_chunk, test_tile_x, test_tile_y);
    }
    return tile_value;
}

internal void
set_tile_value_checked(TileMap *tile_map,
                       TileChunk *tile_chunk,
                       u32 test_tile_x,
                       u32 test_tile_y,
                       u32 tile_value)
{
    if (tile_chunk && tile_chunk->tiles)
    {
        set_tile_value_unchecked(
            tile_map, tile_chunk, test_tile_x, test_tile_y, tile_value);
    }
}

internal TileChunk *
get_tile_chunk(TileMap *tile_map,
               u32 tile_chunk_x,
               u32 tile_chunk_y,
               u32 tile_chunk_z)
{
    TileChunk *tile_chunk = 0;

    b32 x_in_bounds =
        tile_chunk_x >= 0 && tile_chunk_x < tile_map->tile_chunk_count_x;
    b32 y_in_bounds =
        tile_chunk_y >= 0 && tile_chunk_y < tile_map->tile_chunk_count_y;
    b32 z_in_bounds =
        tile_chunk_z >= 0 && tile_chunk_z < tile_map->tile_chunk_count_z;

    if (x_in_bounds && y_in_bounds && z_in_bounds)
    {
        u32 tile_chunk_index = (tile_chunk_z * tile_map->tile_chunk_count_y *
                                tile_map->tile_chunk_count_x) +
                               (tile_chunk_y * tile_map->tile_chunk_count_x) +
                               (tile_chunk_x);
        tile_chunk = &tile_map->tile_chunks[tile_chunk_index];
    }

    return tile_chunk;
}

internal TileChunkPosition
get_chunk_position_for(TileMap *tile_map,
                       u32 abs_tile_x,
                       u32 abs_tile_y,
                       u32 abs_tile_z)
{
    TileChunkPosition result;

    result.tile_chunk_x = abs_tile_x >> tile_map->chunk_shift;
    result.tile_chunk_y = abs_tile_y >> tile_map->chunk_shift;
    result.tile_chunk_z = abs_tile_z;

    result.rel_tile_x = abs_tile_x & tile_map->chunk_mask;
    result.rel_tile_y = abs_tile_y & tile_map->chunk_mask;

    return result;
}

internal u32
get_tile_value(TileMap *tile_map,
               u32 abs_tile_x,
               u32 abs_tile_y,
               u32 abs_tile_z)
{
    TileChunkPosition chunk_pos =
        get_chunk_position_for(tile_map, abs_tile_x, abs_tile_y, abs_tile_z);
    TileChunk *tile_chunk = get_tile_chunk(tile_map,
                                           chunk_pos.tile_chunk_x,
                                           chunk_pos.tile_chunk_y,
                                           chunk_pos.tile_chunk_z);

    u32 tile_value = get_tile_value_checked(
        tile_map, tile_chunk, chunk_pos.rel_tile_x, chunk_pos.rel_tile_y);

    return tile_value;
}

internal u32
get_tile_value_from_position(TileMap *tile_map, TileMapPosition position)
{
    return get_tile_value(tile_map,
                          position.abs_tile_x,
                          position.abs_tile_y,
                          position.abs_tile_z);
}

internal b32
is_tile_map_point_empty(TileMap *tile_map, TileMapPosition canon_pos)
{
    u32 tile_chunk_value = get_tile_value(tile_map,
                                          canon_pos.abs_tile_x,
                                          canon_pos.abs_tile_y,
                                          canon_pos.abs_tile_z);
    b32 empty = ((tile_chunk_value == 1) || (tile_chunk_value == 3) ||
                 (tile_chunk_value == 4));

    return empty;
}

internal void
set_tile_value(MemoryArena *arena,
               TileMap *tile_map,
               u32 abs_tile_x,
               u32 abs_tile_y,
               u32 abs_tile_z,
               u32 tile_value)
{
    TileChunkPosition chunk_pos =
        get_chunk_position_for(tile_map, abs_tile_x, abs_tile_y, abs_tile_z);
    TileChunk *tile_chunk = get_tile_chunk(tile_map,
                                           chunk_pos.tile_chunk_x,
                                           chunk_pos.tile_chunk_y,
                                           chunk_pos.tile_chunk_z);

    ASSERT(tile_chunk);
    if (!tile_chunk->tiles)
    {
        u32 tile_count    = tile_map->chunk_dim * tile_map->chunk_dim;
        tile_chunk->tiles = PUSH_ARRAY(arena, tile_count, u32);

        for (u32 tile_index = 0; tile_index < tile_count; tile_index++)
        {
            tile_chunk->tiles[tile_index] = 1;
        }
    }
    set_tile_value_checked(tile_map,
                           tile_chunk,
                           chunk_pos.rel_tile_x,
                           chunk_pos.rel_tile_y,
                           tile_value);
}

// TODO: move to map positioning file
internal void
realign_coordinate(TileMap *tile_map, u32 *tile, f32 *tile_rel)
{
    // TODO: Need to do something that doesn't use the divide/multiply method
    // for recanonicalizing because this this can end up rounding back on to
    // the tile you just came from

    // NOTE: tile_map is assumed to be toroidal topology.
    // If you step off one end you come back on the other.

    i32 offset = round_f32_to_i32(*tile_rel / tile_map->tile_side_meters);
    *tile += offset;
    *tile_rel -= offset * tile_map->tile_side_meters;

    // check x/y within bounds of a tile
    ASSERT(*tile_rel >= -0.5f * tile_map->tile_side_meters);
    ASSERT(*tile_rel <= 0.5f * tile_map->tile_side_meters);
}

internal TileMapPosition
realign_position(TileMap *tile_map, TileMapPosition pos)
{
    TileMapPosition result = pos;

    realign_coordinate(tile_map, &result.abs_tile_x, &result.offset_x);
    realign_coordinate(tile_map, &result.abs_tile_y, &result.offset_y);

    return result;
}

internal b32
on_same_tile(TileMapPosition *a, TileMapPosition *b)
{
    b32 result = a->abs_tile_x == b->abs_tile_x &&
                 a->abs_tile_y == b->abs_tile_y &&
                 a->abs_tile_z == b->abs_tile_z;

    return result;
}
