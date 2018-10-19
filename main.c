/* Copyright (C) 2018 Mark E Sowden <markelswo@gmail.com>
 * Utility for reading/extracting/converting files from
 * Bullfrog's cancelled Creation game.
 * */

#include <PL/platform.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_image.h>

#include <assert.h>

#include "types.h"

/* see notes for further details */

#define Warning(...)    printf("WARNING: " __VA_ARGS__)
#define Error(...)      { printf("ERROR: " __VA_ARGS__); exit(EXIT_FAILURE); }

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
        Warning("Couldn't allocate output image buffer");
        return false;
    }

    image->data[0] = pl_calloc(image->size, sizeof(uint8_t));
    if(image->data[0] == NULL) {
        Warning("Couldn't allocate output image buffer");
        return false;
    }

    if(buf != NULL) {
        memcpy(image->data[0], buf, image->size);
    }

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
        Warning("Failed to find \"%s\"!\n", path);
        return false;
    }

    size_t sz = plGetFileSize(path);
    int num_colours = (int) (sz / 3);
    if(num_colours != 256) {
        Warning("Unexpected number of colours in palette (%d/256), aborting load!\n", num_colours);
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

/* converts to Valve's SMD format, for convenience */
void ConvertModel(const char *path, const char *out_path) {

}

BullfrogSpriteTable *LoadSpriteTable(const char *tab_path, const char *dat_path) {
    /* ensure both the tab and dat exist before going anywhere */

    if(!plFileExists(tab_path)) {
        Warning("Failed to load \"%s\", aborting!\n", tab_path);
        return NULL;
    }

    if(!plFileExists(dat_path)) {
        Warning("Failed to load \"%s\", aborting!\n", dat_path);
        return NULL;
    }

    size_t sz = plGetFileSize(tab_path);
    if(sz < sizeof(BullfrogSpriteTableIndex)) {
        Warning("Invalid table size, %lu, aborting!\n", sz);
        return NULL;
    }

    FILE *fp = fopen(tab_path, "rb");
    if(fp == NULL) {
        Warning("Failed to load \"%s\", aborting!\n", tab_path);
        return NULL;
    }

    char buf[sz];
    fread(buf, sizeof(char), sz, fp);
    fclose(fp);

    BullfrogSpriteTable *table = malloc(sizeof(BullfrogSpriteTable));
    if(table == NULL) {
        Warning("Failed to create table handle!\n");
        return NULL;
    }

    table->num_indices = (unsigned int) (sz / 6);
    table->indices = malloc(sizeof(BullfrogSpriteTableIndex) * table->num_indices);
    if(table->indices == NULL) {
        free(table);
        Warning("Failed to create table indices (%u)!\n", table->num_indices);
        return NULL;
    }

    memcpy(table->indices, buf, sizeof(BullfrogSpriteTableIndex) * table->num_indices);
#if 1 /* print out each index, so we know it's loaded in correctly */
    for(unsigned int i = 0; i < table->num_indices; ++i) {
        printf(
                "------------\n"
                "index %d\n"
                "OFFSET: %d\n"
                "WIDTH:  %d\n"
                "HEIGHT: %d\n",

                i,
                table->indices[i].off,
                table->indices[i].w,
                table->indices[i].h
        );
    }
#endif

    return table;
}

/**
 * Convert image from DAT table and output it to PNG.
 * Some of this is borrowed from Freesynd -
 * https://sourceforge.net/p/freesynd/code/HEAD/tree/freesynd/trunk/src/gfx/sprite.cpp#l143
 *
 * @param tab       Sprite table
 * @param index     Index into the sprite table, this is what we'll output
 * @param pal       Palette table
 * @param path      Input path
 * @param out_path  Output path
 */
void ConvertImageTable(const BullfrogSpriteTable *tab, uint index, const BullfrogVGAPalette *pal, const char *out_path) {
#if 0 /* rewriting this... */
    BullfrogSpriteTableIndex *bi = &tab->indices[index];
    if(bi->w == 0 || bi->h == 0) {
        return;
    }

    if(tab->length == 0 || bi->off > tab->length) {
        Error("Invalid table size, aborting!\n");
    }

    /* now load in the actual image data */

    FILE *fp = fopen(path, "rb");

    uint16_t num_sprites;
    fread(&num_sprites, sizeof(uint16_t), 1, fp);
    fseek(fp, SEEK_SET, bi->off);
    unsigned int stride = (unsigned int) ceil(bi->w);
    unsigned long block_length = (unsigned long) ((bi->w * bi->h) * 3);
    unsigned char blocks[block_length];
    fread(blocks, sizeof(char), block_length, fp);
    fclose(fp);

    /* if num sprites returns anything but 0, it's encoded */
    if(num_sprites != tab->num_indices) {
        Error("Image table is using blocks... TODO!\n");
    } else {
        for(unsigned int i = 0; i < bi->h; ++i) {
            uint8_t b = *
            int rle = b
        }
    }

    /* now write it */

    struct {
        uint8_t r, g, b;
    } img[rl];
    for(unsigned int i = 0; i < rl; i++) {
        img[i].r = pal->pixels[nr[i]].r;
        img[i].g = pal->pixels[nr[i]].g;
        img[i].b = pal->pixels[nr[i]].b;
    }

    free(nr);

    PLImage out;
    CreateImage(&out, (uint8_t *) img, tab->indices[index].w, tab->indices[index].h, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_RGB8);
    if(!plWriteImage(&out, out_path)) {
        Warning("Failed to write image to \"%s\"!\n(%s)\n", out_path, plGetError());
    }

    plFreeImage(&out);
#endif
}

void ConvertImage(const char *path, const char *pal_path, const char *out_path, unsigned int width, unsigned int height) {
    printf("Converting %s to %s ...\n", path, out_path);

    /* check that both the DAT and PAL exist */

    if(!plFileExists(path)) {
        Warning("Failed to load \"%s\", aborting!\n", path);
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
        Warning("Failed to write image to \"%s\"!\n(%s)\n", out_path, plGetError());
    }

    plFreeImage(&out);
}

/***************************************************************/
/* Creation Map API */

CreationMap *Map_Load(const char *path) {
    CreationMap *map = malloc(sizeof(CreationMap));
    if(map == NULL) {
        Error("Failed to allocate map (%ld bytes)!\n", sizeof(CreationMap));
    }

    /* load in the height-map */

    size_t len = plGetFileSize(path);
    unsigned int num_tiles = (unsigned int) (len / sizeof(CreationMapTile));
    if(num_tiles >= CREATION_MAP_MAX_TILES) {
        Error("Invalid size for \"%s\", expected %d tiles but found %d!\n", path, CREATION_MAP_MAX_TILES, num_tiles);
    }

#if 0
#if 1   /* output multiple textures */
    uint8_t tiles[CREATION_MAP_MAX_TILES][sizeof(CreationMapTile)];
    FILE *fp = fopen(path, "rb");
    size_t r = fread(tiles, sizeof(CreationMapTile), num_tiles, fp);
    if(r != num_tiles) {
        Error("Failed to read in all tiles from \"%s\" (%ld/%d), aborting!\n", path, r, num_tiles);
    }
    fclose(fp);

    char out_path[PL_SYSTEM_MAX_PATH];
    for(unsigned int j = 0; j < 12; ++j) {
        snprintf(out_path, sizeof(out_path), "map%d.png", j);

        struct {
            uint8_t r, g, b;
        } map_buf[CREATION_MAP_MAX_TILES];
        memset(map_buf, 0, sizeof(map_buf));
        for(unsigned int i = 0; i < CREATION_MAP_MAX_TILES; ++i) {
            map_buf[i].r = (uint8_t) (tiles[i][j] * 4);
            map_buf[i].g = (uint8_t) (tiles[i][j] * 4);
            map_buf[i].b = (uint8_t) (tiles[i][j] * 4);
        }

        PLImage out;
        CreateImage(&out, (uint8_t *) map_buf, CREATION_MAP_ROW_TILES, CREATION_MAP_ROW_TILES, PL_COLOURFORMAT_RGB,
                    PL_IMAGEFORMAT_RGB8);
        if(!plWriteImage(&out, out_path)) {
            Warning("Failed to write image to \"%s\"!\n(%s)\n", out_path, plGetError());
        }

        plFreeImage(&out);
    }
#else   /* output a single image */
    CreationMapTile tiles[CREATION_MAP_MAX_TILES];
    FILE *fp = fopen(path, "rb");
    if(fread(tiles, sizeof(CreationMapTile), CREATION_MAP_MAX_TILES, fp) != CREATION_MAP_MAX_TILES) {
        Error("Failed to read in all tiles from \"%s\", aborting!\n", path);
    }
    fclose(fp);

    struct {
        uint8_t r, g, b;
    } map_buf[CREATION_MAP_MAX_TILES];
    memset(map_buf, 0, sizeof(map_buf));
    for(unsigned int i = 0; i < CREATION_MAP_MAX_TILES; ++i) {
        map_buf[i].r = (uint8_t) (tiles[i].height1 * 4);
        map_buf[i].g = (uint8_t) (tiles[i].height1 * 4);
        map_buf[i].b = (uint8_t) (tiles[i].height1 * 4);
    }

    PLImage out;
    CreateImage(&out, (uint8_t *) map_buf, CREATION_MAP_ROW_TILES, CREATION_MAP_ROW_TILES, PL_COLOURFORMAT_RGB,
                PL_IMAGEFORMAT_RGB8);
    if(!plWriteImage(&out, "map.png")) {
        Warning("Failed to write image to \"%s\"!\n(%s)\n", "map.png", plGetError());
    }

    plFreeImage(&out);
#endif
#else
    FILE *fp = fopen(path, "rb");
    if(fp == NULL) {
        Error("Failed to open map \"%s\", aborting!\n", path);
    }

    size_t r = fread(map->tiles, sizeof(CreationMapTile), num_tiles, fp);
    if(r != num_tiles) {
        Error("Failed to read in all tiles from \"%s\" (%ld/%d), aborting!\n", path, r, num_tiles);
    }

    fclose(fp);
#endif

    /* todo: load in lev data */

    return map;
}

/***************************************************************/

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
//    printf("%lu bytes\n", pos.__pos);
    fclose(fp);
}

/* Generate a .MAP file from some height-map data, just to see
 * if we're on the right track */
void GenerateMap(const char *name) {
    char height_path[PL_SYSTEM_MAX_PATH];
    snprintf(height_path, sizeof(height_path), "LEVELS/%s_h.png", name);
    PLImage height_image;
    if(!plLoadImage(height_path, &height_image)) {
        Error("Failed to load image, \"%s\", aborting!\n%s", height_path, plGetError());
    }

    /* copy the data into our output buffer, for our map data */

    CreationMapTile tiles[CREATION_MAP_MAX_TILES];
    memset(tiles, 0, sizeof(CreationMapTile) * CREATION_MAP_MAX_TILES);
    uint8_t *pixel = height_image.data[0];
    for(unsigned int i = 0; i < CREATION_MAP_MAX_TILES; ++i) {
        uint8_t hp = (uint8_t) (((*(pixel++)) + (*(pixel++)) + (*(pixel++))) / 24);

        /* texture index */
        static uint8_t idx = 0;
        if(idx >= 61) { idx = 0; }
        tiles[i].texture = ++idx;

        tiles[i].height0 = tiles[i].height1 = hp;
        tiles[i].ceiling = (uint8_t) (hp * ((rand() % 255) + 1));
        pixel++;
    }
    plFreeImage(&height_image);

    /* now write our new map */

    char map_path[PL_SYSTEM_MAX_PATH];
    snprintf(map_path, sizeof(map_path), "LEVELS/%s.MAP", name);
    FILE *fp = fopen(map_path, "wb");
    if(fp == NULL) {
        Error("Failed to open \"%s\" for write, aborting!\n", map_path);
    }

    fwrite(tiles, sizeof(CreationMapTile), CREATION_MAP_MAX_TILES, fp);
    fclose(fp);
}

void ReadBullfrogDataObjectFile(const char *path) {
    size_t sz = plGetFileSize(path);
    if(sz <= sizeof(BullfrogObjectHeader)) {
        return;
    }

    BullfrogObjectHeader header;
    memset(&header, 0, sizeof(BullfrogObjectHeader));
    FILE *fp = fopen(path, "rb");
    fread(&header, sizeof(BullfrogObjectHeader), 1, fp);
    fclose(fp);

    if(pl_strncasecmp(header.identity, "BULLFROG OBJECT DATA", 20) != 0) {
        return;
    }

#define prnt_var(var)   printf(" " plStringify(var) " : %d\n", header.var)

    printf("Bullfrog Object Data : %s (%lu bytes)\n", path, sz);
    prnt_var(length0);
    prnt_var(length1);
    prnt_var(length2);
    prnt_var(unknown0);
    prnt_var(unknown1);
    prnt_var(num_blobs0);
    prnt_var(num_blobs1);
    prnt_var(unknown3);
    prnt_var(unknown4);
    prnt_var(unknown5);
    prnt_var(unknown6);
    prnt_var(unknown7);
    prnt_var(unknown8);
    prnt_var(unknown9);
    prnt_var(unknown10);
    prnt_var(unknown11);
    prnt_var(unknown12);
    printf("\n");
}

/* Generate an overview of the map, using texture tiles */
void GenerateOverview(void) {
    /* load in the texture sheet */
    PLImage textures;
    plLoadImage("DATA/BLOCK64.png", &textures);

    /* now load in the map */

    CreationMap *map = Map_Load("LEVELS/original/DEFAULT.MAP");

    /* each texture is 64x64
     * and there are 64 textures in total */
    /* 65536 */
    /* 4096 */
    /* w16384xh16384 */

    PLImage output;
    CreateImage(&output, NULL, 16384, 16384, PL_COLOURFORMAT_RGB, PL_IMAGEFORMAT_RGB8);
    for(unsigned int i = 0; i < CREATION_MAP_MAX_TILES - 1; ++i) {
        CreationMapTile *tile = &map->tiles[i];

        /* x and y coordinate for the tile texture within the texture sheet */
        unsigned int t_x = (unsigned int)((tile->texture % 4) * 64);
        unsigned int t_y = ((unsigned int)(tile->texture / 4)) * 64;

        /* x and y destination coordinate */
        unsigned int d_x = ((i % CREATION_MAP_ROW_TILES) * 64);
        unsigned int d_y = (i / CREATION_MAP_ROW_TILES) * 64;

        uint8_t *pos = output.data[0] + ((d_y * output.width) + d_x) * 3;
        uint8_t *src = textures.data[0] + ((t_y * textures.width) + t_x) * 4;
        for(unsigned int y = 0; y < 64; ++y) {
            uint8_t *s = src;
            uint8_t *p = pos;
            for(unsigned int j = 0; j < 64; ++j) {
                memcpy(p, s, 3);
                p += 3;
                s += 4;
            }

            src += textures.width * 4;
            pos += output.width * 3;
        }
    }

    plWriteImage(&output, "overview.bmp");
    plFreeImage(&output);
}

int main(int argc, char **argv) {
    plInitialize(argc, argv);

    /* ensure random is random */
    srand((unsigned int) time(NULL));

    BullfrogVGAPalette pal;
    LoadPalette(&pal, "DATA/PALETTE.DAT");

    ConvertImage("DATA/BRIEF.DAT", "DATA/BRIEF.PAL", "DATA/BRIEF.png", 640, 480);
    ConvertImage("DATA/CRE8LOGO.DAT", "DATA/CRE8LOGO.PAL", "DATA/CRE8LOGO.png", 640, 480);
    ConvertImage("DATA/PANELHI.DAT", "DATA/PALETTE.DAT", "DATA/PANELHI.png", 640, 480);
    ConvertImage("DATA/HELPBACK.DAT", "DATA/HELPBACK.PAL", "DATA/HELPBACK.png", 640, 480);
    ConvertImage("DATA/TEXTURE.TEX", "DATA/PALETTE.DAT", "DATA/TEXTURE.png", 256, 128);
    ConvertImage("DATA/TEX01.DAT", "DATA/PALETTE.DAT", "DATA/TEX01.png", 256, 256);
    ConvertImage("DATA/TABLES.DAT", "DATA/PALETTE.DAT", "DATA/TABLES.png", 256, 128);
    ConvertImage("DATA/BACKGRND.DAT", "DATA/PALETTE.DAT", "DATA/BACKGRND.png", 320, 200);
    ConvertImage("DATA/BLOCK32.DAT", "DATA/PALETTE.DAT", "DATA/BLOCK32.png", 256, 256);
    ConvertImage("DATA/BLOCK64.DAT", "DATA/PALETTE.DAT", "DATA/BLOCK64.png", 256, 1024);

    /* tables */

#if 0
    BullfrogSpriteTable *tab = LoadSpriteTable("DATA/PANEL.TAB", "DATA/PANEL.DAT");
    if(tab == NULL) {
        Error("Failed to load table!\n");
    }

    for(unsigned int i = 0; i < tab->num_indices; ++i) {
        char out[PL_SYSTEM_MAX_PATH];
        snprintf(out, sizeof(out), "DATA/PANEL_%d.png", i);
        ConvertImageTable(tab, i, &pal, out);
    }
#endif

    /* convert PALETTE.DAT and GAMEPAL.PAL */

    WritePalette(&pal, "DATA/PALETTE.png");
    LoadPalette(&pal, "DATA/GAMEPAL.PAL");
    WritePalette(&pal, "DATA/GAMEPAL.png");

    /* now try generating a brand new .MAP as well! */

    GenerateMap("DEFAULT");
    GenerateOverview();

    /* load in the map data for analysis */

    DB_ReadLevData();

    /* now let's analyse those Bullfrog Object Data files */

    printf("\n");
    plScanDirectory("DATA", "DAT", ReadBullfrogDataObjectFile, false);

    return 0;
}