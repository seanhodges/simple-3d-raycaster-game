#ifndef TEXTURES_SDL_H
#define TEXTURES_SDL_H

#include <stdbool.h>
#include <stdint.h>

/* ── Texture constants ────────────────────────────────────────────── */
#define TEX_SIZE  64             /* width & height of one texture tile */
#define TEX_COUNT 10             /* number of textures in the atlas    */

/* ── Fallback colour (RGBA8888) ───────────────────────────────────── */
#define COL_WALL  0x00008BFF     /* dark blue used when BMP fails to load */

/**  Load texture atlas from a BMP file. Falls back to a solid wall
 *   colour (COL_WALL) if the file cannot be loaded. Returns false
 *   only on unrecoverable error. */
bool tm_init(const char *atlas_path);

/**  Free texture memory. */
void tm_shutdown(void);

/**  Sample a pixel from the atlas.
 *   wall_type: 0 .. TEX_COUNT-1
 *   tex_x, tex_y: 0 .. TEX_SIZE-1
 *   Returns colour in RGBA8888 format. */
unsigned int tm_get_pixel(uint16_t wall_type, int tex_x, int tex_y);

#endif /* TEXTURES_SDL_H */
