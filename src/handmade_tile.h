#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

#include "handmade_platform.h"
#include "handmade_math.h"

typedef struct tile_map_difference
{
    f32 d_x;
    f32 d_y;
    f32 d_z;
} TileMapDifference;

typedef struct tile_map_position
{
    // NOTE: These are fixed point tile locations. The high
    // bits are the tile chunk index, and the low buts are
    // the tile index in the chunk.
    u32 abs_tile_x;
    u32 abs_tile_y;
    u32 abs_tile_z;

    // NOTE: Offset from the tile's center
    vec2 offset;
} TileMapPosition;

typedef struct tile_chunk_position
{
    u32 tile_chunk_x;
    u32 tile_chunk_y;
    u32 tile_chunk_z;

    u32 rel_tile_x;
    u32 rel_tile_y;
} TileChunkPosition;

typedef struct tile_chunk
{
    u32 *tiles;
} TileChunk;

typedef struct tile_map
{
    u32 chunk_shift;
    u32 chunk_mask;
    u32 chunk_dim;

    f32 tile_side_meters;

    // TODO: Real sparseness so anywhere in the world can be represented without
    // the giant pointer array.
    u32 tile_chunk_count_x;
    u32 tile_chunk_count_y;
    u32 tile_chunk_count_z;

    TileChunk *tile_chunks;
} TileMap;


#endif // HANDMADE_TILE_H
