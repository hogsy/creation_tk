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
    char identity[24];  /* "BULLFROG OBJECT DATA" */
    /* todo: pending */
} BullfrogObjectHeader;

/***************************************************************/

typedef struct __attribute__((__packed__)) BullfrogSpriteTableIndex {
    uint32_t    off;
    uint8_t     w;
    uint8_t     h;
} BullfrogSpriteTableIndex;

typedef struct BullfrogSpriteTable {
    BullfrogSpriteTableIndex *indices;
    unsigned int num_indices;
} BullfrogSpriteTable;
