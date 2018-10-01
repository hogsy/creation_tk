/* Copyright (C) 2018 Mark E Sowden <markelswo@gmail.com>
 * Bullfrog data types
 * */

#pragma once

typedef struct BullfrogVGAPalette {
    struct {
        uint8_t r, g, b;
    } pixels[256];
} BullfrogVGAPalette;

/***************************************************************/

typedef struct BullfrogObjectHeader {
    char    identity[24];  /* "BULLFROG OBJECT DATA" */
    /* todo: pending */
} BullfrogObjectHeader;

/***************************************************************/
/* Bullfrog Sprite Table */

typedef struct __attribute__((__packed__)) BullfrogSpriteTableIndex {
    uint32_t    off;
    uint8_t     w;
    uint8_t     h;
} BullfrogSpriteTableIndex;

typedef struct BullfrogSpriteTable {
    BullfrogSpriteTableIndex*   indices;
    unsigned int                num_indices;

    char*           pixels;
    unsigned int    length;
} BullfrogSpriteTable;

/***************************************************************/
/* Creation Formats */

#define CREATION_MAP_MAX_TILES  65537   /* maximum tiles allowed within a map */
#define CREATION_MAP_ROW_TILES  256     /* number of tiles along a row */

typedef struct __attribute__((__packed__)) CreationMapTile {
    int8_t      u0;             /* unused */
    uint8_t     height0;        /* base height? mesh isn't based from this */
    uint8_t     ceiling;        /* demo appears to randomise this and multiply based on height */
    uint8_t     height1;        /* terrain height */
    uint16_t    texture;        /* tile texture index */
    int16_t     u1;             /* unknown, seems to correlate to height? */
    int8_t      u2[4];          /* unused */
} CreationMapTile; /* 12 bytes */
static_assert(sizeof(CreationMapTile) == 12, "invalid struct size for CreationMapTile");

typedef struct CreationModel { /* todo */
    char    name[14];   /* internal project name */
    char    desc[14];
} CreationModel;
