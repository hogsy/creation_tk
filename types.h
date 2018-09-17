/* Copyright (C) 2018 Mark E Sowden <markelswo@gmail.com>
 * Bullfrog data types
 * */

#pragma once

typedef struct BullfrogVGAPalette {
    struct {
        uint8_t r, g, b;
    } pixels[256];
} BullfrogVGAPalette;

typedef struct BullfrogObjectHeader {
    char identity[24];  /* "BULLFROG OBJECT DATA" */
    /* todo: pending */
} BullfrogObjectHeader;

typedef struct BullfrogTABIndex {
    uint32_t    offset;
    uint8_t     width;
    uint8_t     height;
} BullfrogTABIndex;