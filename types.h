/* Copyright (C) 2018 Mark E Sowden <markelswo@gmail.com>
 * Bullfrog data types
 * */

#pragma once

typedef struct BullfrogVGAPalette {
    struct {
        uint8_t r, g, b;
    } pixels[256];
} BullfrogVGAPalette;
