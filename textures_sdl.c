/*  textures_sdl.c  –  texture atlas manager
 *  ────────────────────────────────────
 *  Loads a horizontal strip of TEX_COUNT square textures from a BMP
 *  file.  Falls back to a solid wall colour if the file is missing.
 */
#include "textures_sdl.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ── Internal pixel buffer ────────────────────────────────────────── */

static unsigned int pixels[TEX_COUNT * TEX_SIZE * TEX_SIZE];
static bool initialised = false;
static bool atlas_loaded = false;  /* true only when BMP loaded OK */

/* ── Solid-colour fallback ────────────────────────────────────────── */

static void fill_solid(void)
{
    int total = TEX_COUNT * TEX_SIZE * TEX_SIZE;
    for (int i = 0; i < total; i++)
        pixels[i] = COL_WALL;
}

/* ── Public API ───────────────────────────────────────────────────── */

bool tm_init(const char *atlas_path)
{
    memset(pixels, 0, sizeof(pixels));
    atlas_loaded = false;

    SDL_Surface *surf = SDL_LoadBMP(atlas_path);
    if (!surf) {
        fprintf(stderr, "tm_init: cannot load '%s': %s – using solid colour\n",
                atlas_path, SDL_GetError());
        fill_solid();
        initialised = true;
        return true;
    }

    /* Convert to RGBA8888 for uniform access */
    SDL_Surface *conv = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA8888);
    SDL_DestroySurface(surf);
    if (!conv) {
        fprintf(stderr, "tm_init: surface conversion failed: %s – using solid colour\n",
                SDL_GetError());
        fill_solid();
        initialised = true;
        return true;
    }

    /* Copy pixels from the atlas into our flat buffer.
     * Atlas layout: TEX_COUNT textures side-by-side horizontally.
     * Expected atlas width = TEX_COUNT * TEX_SIZE, height = TEX_SIZE. */
    int expected_w = TEX_COUNT * TEX_SIZE;
    if (conv->w < expected_w || conv->h < TEX_SIZE) {
        fprintf(stderr, "tm_init: atlas too small (%dx%d, need %dx%d) – using solid colour\n",
                conv->w, conv->h, expected_w, TEX_SIZE);
        SDL_DestroySurface(conv);
        fill_solid();
        initialised = true;
        return true;
    }

    const unsigned int *src = (const unsigned int *)conv->pixels;
    int pitch_pixels = conv->pitch / 4;

    for (int t = 0; t < TEX_COUNT; t++) {
        for (int y = 0; y < TEX_SIZE; y++) {
            for (int x = 0; x < TEX_SIZE; x++) {
                int src_x = t * TEX_SIZE + x;
                int dst   = t * TEX_SIZE * TEX_SIZE + y * TEX_SIZE + x;
                pixels[dst] = src[y * pitch_pixels + src_x];
            }
        }
    }

    SDL_DestroySurface(conv);
    atlas_loaded = true;
    initialised = true;
    return true;
}

void tm_shutdown(void)
{
    initialised = false;
}

unsigned int tm_get_pixel(uint16_t wall_type, int tex_x, int tex_y)
{
    if (!initialised || !atlas_loaded) return COL_WALL;

    /* Clamp inputs */
    if (wall_type < 0) wall_type = 0;
    if (wall_type >= TEX_COUNT) wall_type = TEX_COUNT - 1;
    if (tex_x < 0)             tex_x = 0;
    if (tex_x >= TEX_SIZE)     tex_x = TEX_SIZE - 1;
    if (tex_y < 0)             tex_y = 0;
    if (tex_y >= TEX_SIZE)     tex_y = TEX_SIZE - 1;

    return pixels[wall_type * TEX_SIZE * TEX_SIZE + tex_y * TEX_SIZE + tex_x];
}
