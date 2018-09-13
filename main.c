/* Copyright (C) 2018 Mark E Sowden <markelswo@gmail.com>
 * Utility for reading/extracting/converting files from
 * Bullfrog's cancelled Creation game.
 * */

#include <PL/platform.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_image.h>

/* see notes for further details */

PLImage CreateImage(uint8_t *buf, unsigned int w, unsigned int h, PLColourFormat col, PLImageFormat dat) {
    PLImage image;
    memset(&image, 0, sizeof(PLImage));
    image.width             = w;
    image.height            = h;
    image.colour_format     = col;
    image.format            = dat;
    image.size              = plGetImageSize(image.format, image.width, image.height);
    image.levels            = 1;

    image.data = pl_calloc(image.levels, sizeof(uint8_t*));
    if(image.data == NULL) {
        printf("Couldn't allocate output image buffer");
        exit(EXIT_FAILURE);
    }

    image.data[0] = pl_calloc(image.size, sizeof(uint8_t));
    if(image.data[0] == NULL) {
        printf("Couldn't allocate output image buffer");
        exit(EXIT_FAILURE);
    }

    memcpy(image.data[0], buf, image.size);

    return image;
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

unsigned char *ReadPalette(const char *path, unsigned char *buf, size_t len) {
    if(!plFileExists(path)) {
        return NULL;
    }

    size_t sz = plGetFileSize(path);
    if(sz != 768) {
        printf("Unexpected palette size, does not equal expected 768 (%lu), aborting!\n", len);
        return NULL;
    }

    /* we could probably check to ensure that len passed here matches
     * the expected size, but we'll worry about that later */

    FILE *fp = fopen(path, "rb");
    fread(buf, sizeof(char), len, fp);
    fclose(fp);

    return buf;
}

void WritePalette(const char *path, const unsigned char *buf, size_t len) {
    /* todo: allow writing new palettes */

    int num_colours = (int) (len / 3);
    if(num_colours != 256) {
        printf("Invalid number of colours in palette (%d), aborting write!\n", num_colours);
        return;
    }

    struct {
        uint8_t r, g, b;
    } pal[256];
    for(unsigned int i = 0, j = 0; i < 256; i++, j += 3) {
        pal[i].r = (uint8_t) ((buf[j] * 255) / 63);
        pal[i].g = (uint8_t) ((buf[j + 1] * 255) / 63);
        pal[i].b = (uint8_t) ((buf[j + 2] * 255) / 63);
    }

    PLImage img = CreateImage((uint8_t *) pal, 256, 1, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_RGB8);
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

    unsigned long len = plGetFileSize(pal_path);
    unsigned char pal_buf[len];
    if(!ReadPalette(pal_path, pal_buf, len)) {
        printf("Failed to load \"%s\", aborting!\n", pal_path);
        return;
    }

#if 0
    /* output the palette data to PNG for testing */

    char pal_out[PL_SYSTEM_MAX_PATH];
    snprintf(pal_out, sizeof(pic_pal), "%s_pal.png", path);
    WritePalette(pal_out, pal_buf, len);
#endif

    /* convert the pal from 6-bit VGA to 8-bit RGB */

    struct {
        uint8_t r, g, b;
    } pal[256];
    for(unsigned int i = 0, j = 0; i < 256; i++, j += 3) {
        pal[i].r = (uint8_t) ((pal_buf[j] * 255) / 63);
        pal[i].g = (uint8_t) ((pal_buf[j + 1] * 255) / 63);
        pal[i].b = (uint8_t) ((pal_buf[j + 2] * 255) / 63);
    }

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
        img[i].r = pal[dat_buf[i]].r;
        img[i].g = pal[dat_buf[i]].g;
        img[i].b = pal[dat_buf[i]].b;
    }

    PLImage out = CreateImage((uint8_t *) img, width, height, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_RGB8);
    if(!plWriteImage(&out, out_path)) {
        printf("Failed to write image to \"%s\"!\n(%s)\n", out_path, plGetError());
    }

    plFreeImage(&out);
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
        map_buf[i].r = (uint8_t) (data[i][1] * 4);
        map_buf[i].g = (uint8_t) (data[i][1] * 4);
        map_buf[i].b = (uint8_t) (data[i][1] * 4);
    }

    PLImage out = CreateImage((uint8_t *) map_buf, 256, 256, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_RGB8);
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

    unsigned char buf[768];

    if(ReadPalette("DATA/PALETTE.DAT", buf, 768) != NULL) {
        WritePalette("DATA/PALETTE.png", buf, 768);
    }

    if(ReadPalette("DATA/GAMEPAL.PAL", buf, 768) != NULL) {
        WritePalette("DATA/GAMEPAL.png", buf, 768);
    }

    /* load in the map data for analysis */

    DB_ReadMapData();

    return 0;
}