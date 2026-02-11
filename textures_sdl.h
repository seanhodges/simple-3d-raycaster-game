#ifndef TEXTURES_SDL_H
#define TEXTURES_SDL_H

#include <stdbool.h>
#include <stdint.h>

/* ── Tile texture constants ───────────────────────────────────────── */
#define TEX_SIZE  64             /* width & height of one texture tile */
#define TEX_COUNT 10             /* number of textures in the atlas    */

/* ── Sprite texture constants ────────────────────────────────────── */
#define SPRITE_TEX_COUNT  4      /* number of sprite textures in atlas */
#define SPRITE_ALPHA_KEY  0x980088FFu /* #980088 magenta = transparent */

/* ── Fallback colour (RGBA8888) ───────────────────────────────────── */
#define COL_WALL  0x00008BFF     /* dark blue used when BMP fails to load */

/**  Load wall texture atlas from a BMP file. Falls back to a solid wall
 *   colour (COL_WALL) if the file cannot be loaded. Returns false
 *   only on unrecoverable error. */
bool tm_init_tiles(const char *atlas_path);

/**  Load sprite texture atlas from a BMP file. Falls back to a solid
 *   colour if the file cannot be loaded. Returns false only on
 *   unrecoverable error. */
bool tm_init_sprites(const char *atlas_path);

/**  Free texture memory. */
void tm_shutdown(void);

/**  Sample a pixel from the wall atlas.
 *   tile_type: 0 .. TEX_COUNT-1
 *   tex_x, tex_y: 0 .. TEX_SIZE-1
 *   Returns colour in RGBA8888 format. */
unsigned int tm_get_tile_pixel(uint16_t tile_type, int tex_x, int tex_y);

/**  Sample a pixel from the sprite atlas.
 *   tex_id: 0 .. SPRITE_TEX_COUNT-1
 *   tex_x, tex_y: 0 .. TEX_SIZE-1
 *   Returns colour in RGBA8888 format. */
unsigned int tm_get_sprite_pixel(uint16_t tex_id, int tex_x, int tex_y);

#endif /* TEXTURES_SDL_H */
