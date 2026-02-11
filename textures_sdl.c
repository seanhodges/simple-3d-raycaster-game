/*  textures_sdl.c  –  texture atlas manager (tiles + sprites)
 *  ────────────────────────────────────────────────────────
 *  Loads horizontal strips of square textures from BMP files.
 *  Falls back to solid colours if files are missing.
 */
#include "textures_sdl.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ── Internal pixel buffers ──────────────────────────────────────── */

static unsigned int pixels[TEX_COUNT * TEX_SIZE * TEX_SIZE];
static unsigned int sprite_pixels[SPRITE_TEX_COUNT * TEX_SIZE * TEX_SIZE];
static bool tile_atlas_loaded   = false;  /* true only when tile BMP loaded  */
static bool sprite_atlas_loaded = false;  /* true only when sprite BMP loaded*/

/* ── Solid-colour fallbacks ──────────────────────────────────────── */

static void fill_solid(void)
{
    int total = TEX_COUNT * TEX_SIZE * TEX_SIZE;
    for (int i = 0; i < total; i++)
        pixels[i] = COL_WALL;
}

static void fill_sprite_solid(void)
{
    int total = SPRITE_TEX_COUNT * TEX_SIZE * TEX_SIZE;
    for (int i = 0; i < total; i++)
        sprite_pixels[i] = 0xFF00FFFF;  /* bright magenta fallback */
}

/* ── Helper: load a horizontal atlas strip into a pixel buffer ───── */

static bool load_atlas(const char *path, unsigned int *buf,
                       int tex_count, const char *label)
{
    SDL_Surface *surf = SDL_LoadBMP(path);
    if (!surf) {
        fprintf(stderr, "load_atlas: cannot load %s '%s': %s – using solid colour\n",
                label, path, SDL_GetError());
        return false;
    }

    /* Convert to RGBA8888 for uniform access */
    SDL_Surface *conv = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA8888);
    SDL_DestroySurface(surf);
    if (!conv) {
        fprintf(stderr, "load_atlas: %s surface conversion failed: %s\n",
                label, SDL_GetError());
        return false;
    }

    /* Atlas layout: tex_count textures side-by-side horizontally. */
    int expected_w = tex_count * TEX_SIZE;
    if (conv->w < expected_w || conv->h < TEX_SIZE) {
        fprintf(stderr, "load_atlas: %s atlas too small (%dx%d, need %dx%d)\n",
                label, conv->w, conv->h, expected_w, TEX_SIZE);
        SDL_DestroySurface(conv);
        return false;
    }

    const unsigned int *src = (const unsigned int *)conv->pixels;
    int pitch_pixels = conv->pitch / 4;

    for (int t = 0; t < tex_count; t++) {
        for (int y = 0; y < TEX_SIZE; y++) {
            for (int x = 0; x < TEX_SIZE; x++) {
                int src_x = t * TEX_SIZE + x;
                int dst   = t * TEX_SIZE * TEX_SIZE + y * TEX_SIZE + x;
                buf[dst] = src[y * pitch_pixels + src_x];
            }
        }
    }

    SDL_DestroySurface(conv);
    return true;
}

/* ── Public API ───────────────────────────────────────────────────── */

bool tm_init_tiles(const char *atlas_path)
{
    memset(pixels, 0, sizeof(pixels));
    tile_atlas_loaded = false;

    if (load_atlas(atlas_path, pixels, TEX_COUNT, "tiles")) {
        tile_atlas_loaded = true;
    } else {
        fill_solid();
    }

    return true;
}

bool tm_init_sprites(const char *atlas_path)
{
    memset(sprite_pixels, 0, sizeof(sprite_pixels));
    sprite_atlas_loaded = false;

    if (load_atlas(atlas_path, sprite_pixels, SPRITE_TEX_COUNT, "sprite")) {
        sprite_atlas_loaded = true;
    } else {
        fill_sprite_solid();
    }

    return true;
}

void tm_shutdown(void)
{
    tile_atlas_loaded = false;
    sprite_atlas_loaded = false;
}

unsigned int tm_get_tile_pixel(uint16_t tile_type, int tex_x, int tex_y)
{
    if (!tile_atlas_loaded) return COL_WALL;

    /* Clamp inputs */
    if (tile_type >= TEX_COUNT) tile_type = TEX_COUNT - 1;
    if (tex_x < 0)             tex_x = 0;
    if (tex_x >= TEX_SIZE)     tex_x = TEX_SIZE - 1;
    if (tex_y < 0)             tex_y = 0;
    if (tex_y >= TEX_SIZE)     tex_y = TEX_SIZE - 1;

    return pixels[tile_type * TEX_SIZE * TEX_SIZE + tex_y * TEX_SIZE + tex_x];
}

unsigned int tm_get_sprite_pixel(uint16_t tex_id, int tex_x, int tex_y)
{
    if (!sprite_atlas_loaded) return 0xFF00FFFF;

    /* Clamp inputs */
    if (tex_id >= SPRITE_TEX_COUNT) tex_id = SPRITE_TEX_COUNT - 1;
    if (tex_x < 0)                  tex_x = 0;
    if (tex_x >= TEX_SIZE)          tex_x = TEX_SIZE - 1;
    if (tex_y < 0)                  tex_y = 0;
    if (tex_y >= TEX_SIZE)          tex_y = TEX_SIZE - 1;

    return sprite_pixels[tex_id * TEX_SIZE * TEX_SIZE + tex_y * TEX_SIZE + tex_x];
}
