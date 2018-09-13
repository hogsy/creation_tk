/* Copyright (C) 2018 Mark E Sowden <markelswo@gmail.com>
 * Utility for reading/extracting/converting files from
 * Bullfrog's cancelled Creation game.
 * */

#include <PL/platform.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_image.h>

#include "types.h"

/* see notes for further details */

bool CreateImage(PLImage *image, uint8_t *buf, unsigned int w, unsigned int h, PLColourFormat col, PLImageFormat dat) {
    memset(image, 0, sizeof(PLImage));
    image->width             = w;
    image->height            = h;
    image->colour_format     = col;
    image->format            = dat;
    image->size              = plGetImageSize(image->format, image->width, image->height);
    image->levels            = 1;

    image->data = pl_calloc(image->levels, sizeof(uint8_t*));
    if(image->data == NULL) {
        printf("Couldn't allocate output image buffer");
        return false;
    }

    image->data[0] = pl_calloc(image->size, sizeof(uint8_t));
    if(image->data[0] == NULL) {
        printf("Couldn't allocate output image buffer");
        return false;
    }

    memcpy(image->data[0], buf, image->size);

    return true;
}

/************************************************************/
/* RNC Compression
 * Based on dernc.c implementation
 * */

void DecompressRNC(const char *path) {

}

/************************************************************/
/* Palette
 * */

bool LoadPalette(BullfrogVGAPalette *pal, const char *path) {
    if(!plFileExists(path)) {
        printf("Failed to find \"%s\"!\n", path);
        return false;
    }

    size_t sz = plGetFileSize(path);
    int num_colours = (int) (sz / 3);
    if(num_colours != 256) {
        printf("Unexpected number of colours in palette (%d/256), aborting load!\n", num_colours);
        return false;
    }

    /* load the data and copy it into our temporary buffer */

    unsigned char buf[sz];

    FILE *fp = fopen(path, "rb");
    fread(buf, sizeof(char), sz, fp);
    fclose(fp);

    /* convert the pal from 6-bit VGA to 8-bit RGB */

    for(unsigned int i = 0, j = 0; i < 256; i++, j += 3) {
        pal->pixels[i].r = (uint8_t) ((buf[j] * 255) / 63);
        pal->pixels[i].g = (uint8_t) ((buf[j + 1] * 255) / 63);
        pal->pixels[i].b = (uint8_t) ((buf[j + 2] * 255) / 63);
    }

    return true;
}

void WritePalette(const BullfrogVGAPalette *pal, const char *path) {
    /* todo: allow writing new palettes */

    PLImage img;
    CreateImage(&img, (uint8_t *) pal->pixels, 256, 1, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_RGB8);
    plWriteImage(&img, path);
    plFreeImage(&img);
}

/************************************************************/

void ConvertImage(const char *path, const char *pal_path, const char *out_path, unsigned int width, unsigned int height) {
    printf("Converting %s to %s ...\n", path, out_path);

    /* check that both the DAT and PAL exist */

    if(!plFileExists(path)) {
        printf("Failed to load \"%s\", aborting!\n", path);
        return;
    }

    /* read in the palette data */

    BullfrogVGAPalette pal;
    LoadPalette(&pal, pal_path);

#if 0
    /* output the palette data to PNG for testing */

    char pal_out[PL_SYSTEM_MAX_PATH];
    snprintf(pal_out, sizeof(pic_pal), "%s_pal.png", path);
    WritePalette(pal_out, pal_buf, len);
#endif

    /* now load in the actual image data */

    unsigned long dat_len = plGetFileSize(path);
    unsigned char dat_buf[dat_len];
    FILE *fp = fopen(path, "rb");
    fread(dat_buf, sizeof(char), dat_len, fp);
    fclose(fp);

    struct {
        uint8_t r, g, b;
    } img[dat_len];
    for(unsigned int i = 0; i < dat_len; i++) {
        img[i].r = pal.pixels[dat_buf[i]].r;
        img[i].g = pal.pixels[dat_buf[i]].g;
        img[i].b = pal.pixels[dat_buf[i]].b;
    }

    PLImage out;
    CreateImage(&out, (uint8_t *) img, width, height, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_RGB8);
    if(!plWriteImage(&out, out_path)) {
        printf("Failed to write image to \"%s\"!\n(%s)\n", out_path, plGetError());
    }

    plFreeImage(&out);
}

void DB_ReadLevData(void) {
    FILE *fp = fopen("LEVELS/DEFAULT.LEV", "rb");

    /* read in the header */

    int sets = fgetc(fp);
    fseek(fp, SEEK_CUR, 8);

    /* now for the rest */

    int8_t buf[sets][10];
    fread(buf, 10, (size_t) sets, fp);
    fpos_t pos;
    fgetpos(fp, &pos);
    printf("%lu bytes\n", pos.__pos);
    fclose(fp);
}

void DB_ReadMapData(void) {
    size_t len = plGetFileSize("LEVELS/DEFAULT_orig.MAP");
    unsigned int num_tiles = (unsigned int) (len / 12);

    typedef struct TILE {
        int8_t  u0;
        int8_t  height0;
        int8_t  u1;
        int8_t  height1;
    } TILE;
    uint8_t data[num_tiles][12];

    FILE *fp = fopen("LEVELS/DEFAULT_orig.MAP", "rb");
    fread(data, 12, num_tiles, fp);
    fclose(fp);

    /* map it out into an image so we can check it out */

    struct {
        uint8_t r, g, b;
    } map_buf[num_tiles];
    for(unsigned int i = 0; i < num_tiles; ++i) {
        map_buf[i].r = (uint8_t) (data[i][3] * 4);
        map_buf[i].g = (uint8_t) (data[i][3] * 4);
        map_buf[i].b = (uint8_t) (data[i][3] * 4);
    }

    PLImage out;
    CreateImage(&out, (uint8_t *) map_buf, 256, 256, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_RGB8);
    if(!plWriteImage(&out, "map.png")) {
        printf("Failed to write image to \"%s\"!\n(%s)\n", "map.png", plGetError());
    }

    plFreeImage(&out);
}

int main(int argc, char **argv) {
    plInitialize(argc, argv);

    ConvertImage("DATA/BRIEF.DAT", "DATA/BRIEF.PAL", "DATA/BRIEF.png", 640, 480);
    ConvertImage("DATA/CRE8LOGO.DAT", "DATA/CRE8LOGO.PAL", "DATA/CRE8LOGO.png", 640, 480);
    ConvertImage("DATA/HELPBACK.DAT", "DATA/HELPBACK.PAL", "DATA/HELPBACK.png", 640, 480);
    ConvertImage("DATA/TEXTURE.TEX", "DATA/PALETTE.DAT", "DATA/TEXTURE.png", 256, 128);
    ConvertImage("DATA/TEX01.DAT", "DATA/PALETTE.DAT", "DATA/TEX01.png", 256, 256);
    ConvertImage("DATA/TABLES.DAT", "DATA/PALETTE.DAT", "DATA/TABLES.png", 256, 128);
    ConvertImage("DATA/SQUID.DAT", "DATA/PALETTE.DAT", "DATA/SQUID.png", 256, 128);
    ConvertImage("DATA/BACKGRND.DAT", "DATA/PALETTE.DAT", "DATA/BACKGRND.png", 320, 200);
    ConvertImage("DATA/BLOCK32.DAT", "DATA/PALETTE.DAT", "DATA/BLOCK32.png", 256, 256);
    ConvertImage("DATA/BLOCK64.DAT", "DATA/PALETTE.DAT", "DATA/BLOCK64.png", 256, 1024);

    /* convert PALETTE.DAT and GAMEPAL.PAL */

    BullfrogVGAPalette pal;
    LoadPalette(&pal, "DATA/PALETTE.DAT");
    WritePalette(&pal, "DATA/PALETTE.png");
    LoadPalette(&pal, "DATA/GAMEPAL.PAL");
    WritePalette(&pal, "DATA/GAMEPAL.png");

    /* load in the map data for analysis */

    DB_ReadLevData();
    DB_ReadMapData();

    return 0;
}