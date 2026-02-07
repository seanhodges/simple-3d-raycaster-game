#ifndef TEXTURE_H
#define TEXTURE_H

#include "raycaster.h"
#include <stdbool.h>

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
unsigned int tm_get_pixel(int wall_type, int tex_x, int tex_y);

#endif /* TEXTURE_H */
