#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

#import "handmade_platform.h"

typedef struct tile_map_position
{
    // NOTE: These are fixed point tile locations. The high
    // bits are the tile chunk index, and the low buts are
    // the tile index in the chunk.
    u32 abs_tile_x;
    u32 abs_tile_y;
    f32 tile_rel_x;
    f32 tile_rel_y;
} TileMapPosition;

typedef struct tile_chunk_position
{
    u32 tile_chunk_x;
    u32 tile_chunk_y;
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
    i32 tile_side_pixels;
    f32 pixels_per_meter;

    i32 tile_chunk_count_x;
    i32 tile_chunk_count_y;

    TileChunk *tile_chunks;
} TileMap;


#endif // HANDMADE_TILE_H
