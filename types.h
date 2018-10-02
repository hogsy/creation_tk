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
/* Bullfrog Object Data */

typedef struct __attribute__((__packed__)) BullfrogObjectHeader {
    char        identity[24];   /* "BULLFROG OBJECT DATA" */
    uint32_t    length0;        /* length of first data chunk following header */
    uint32_t    length1;        /* length of second data chunk */
    uint32_t    length2;        /* length of the final data chunk */
    uint8_t     unknown0;       /* almost always the same for every file */
    uint8_t     unknown1;       /* almost always the same for every file */
    uint16_t    num_blobs0;     /* number of data blobs in the first chunk */
    uint16_t    num_blobs1;     /* number of data blobs in the second and last chunk */
    uint16_t    unknown3;       /* always 79 */
    uint32_t    unknown4;       /* 06 00 00 00 */
    uint8_t     unknown5;       /* 7F */
    uint8_t     unknown6;       /* 1F */
    uint16_t    unknown7;       /* 00 00 */
    uint8_t     unknown8;       /* 7F */
    uint8_t     unknown9;       /* 12 */
    uint16_t    unknown10;      /* 00 00 */
    uint8_t     unknown11;
    uint8_t     unknown12;      /* this is always either 177 or 221 */
    /* first data chunk... */
} BullfrogObjectHeader;
static_assert(sizeof(BullfrogObjectHeader) == 58, "invalid struct size for BullfrogObjectHeader");

typedef struct BullfrogObjectData {
    BullfrogObjectHeader    header;
} BullfrogObjectData;

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
