/*  texture.c  –  texture atlas manager
 *  ────────────────────────────────────
 *  Loads a horizontal strip of TEX_COUNT square textures from a BMP
 *  file.  Falls back to procedural textures if the file is missing.
 */
#include "texture.h"
#include "raycaster.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

/* ── Internal pixel buffer ────────────────────────────────────────── */

static unsigned int pixels[TEX_COUNT * TEX_SIZE * TEX_SIZE];
static bool initialised = false;

/* ── Procedural fallback ──────────────────────────────────────────── */

static unsigned int pack_rgba(int r, int g, int b)
{
    return ((unsigned int)r << 24)
         | ((unsigned int)g << 16)
         | ((unsigned int)b <<  8)
         | 0xFF;
}

static void generate_procedural(void)
{
    /* Base colours for each wall type (R, G, B) */
    static const int base[TEX_COUNT][3] = {
        { 139,  69,  19 },   /* 0: brown brick   */
        { 128, 128, 128 },   /* 1: grey stone     */
        {  34, 139,  34 },   /* 2: green moss     */
        { 178,  34,  34 },   /* 3: red brick      */
        {  70, 130, 180 },   /* 4: steel blue     */
        { 210, 180, 140 },   /* 5: tan sandstone  */
        { 106,  90, 205 },   /* 6: slate purple   */
        { 218, 165,  32 },   /* 7: goldenrod      */
        {  47,  79,  79 },   /* 8: dark teal      */
        { 160,  82,  45 },   /* 9: sienna         */
    };

    for (int t = 0; t < TEX_COUNT; t++) {
        int br = base[t][0], bg = base[t][1], bb = base[t][2];

        for (int y = 0; y < TEX_SIZE; y++) {
            for (int x = 0; x < TEX_SIZE; x++) {
                int idx = t * TEX_SIZE * TEX_SIZE + y * TEX_SIZE + x;

                /* Checkerboard pattern with slight variation */
                int checker = ((x / 8) + (y / 8)) & 1;
                float shade = checker ? 1.0f : 0.75f;

                /* Mortar lines for brick-like appearance */
                bool mortar = (x % 16 == 0) || (y % 16 == 0);
                if (mortar) shade *= 0.6f;

                int r = (int)(br * shade);
                int g = (int)(bg * shade);
                int b = (int)(bb * shade);
                if (r > 255) r = 255;
                if (g > 255) g = 255;
                if (b > 255) b = 255;

                pixels[idx] = pack_rgba(r, g, b);
            }
        }
    }
}

/* ── Public API ───────────────────────────────────────────────────── */

bool tm_init(const char *atlas_path)
{
    memset(pixels, 0, sizeof(pixels));

    SDL_Surface *surf = SDL_LoadBMP(atlas_path);
    if (!surf) {
        fprintf(stderr, "tm_init: cannot load '%s': %s – using procedural textures\n",
                atlas_path, SDL_GetError());
        generate_procedural();
        initialised = true;
        return true;
    }

    /* Convert to RGBA8888 for uniform access */
    SDL_Surface *conv = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA8888);
    SDL_DestroySurface(surf);
    if (!conv) {
        fprintf(stderr, "tm_init: surface conversion failed: %s – using procedural textures\n",
                SDL_GetError());
        generate_procedural();
        initialised = true;
        return true;
    }

    /* Copy pixels from the atlas into our flat buffer.
     * Atlas layout: TEX_COUNT textures side-by-side horizontally.
     * Expected atlas width = TEX_COUNT * TEX_SIZE, height = TEX_SIZE. */
    int expected_w = TEX_COUNT * TEX_SIZE;
    if (conv->w < expected_w || conv->h < TEX_SIZE) {
        fprintf(stderr, "tm_init: atlas too small (%dx%d, need %dx%d) – using procedural textures\n",
                conv->w, conv->h, expected_w, TEX_SIZE);
        SDL_DestroySurface(conv);
        generate_procedural();
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
    initialised = true;
    return true;
}

void tm_shutdown(void)
{
    initialised = false;
}

unsigned int tm_get_pixel(int wall_type, int tex_x, int tex_y)
{
    if (!initialised) return 0x000000FF;

    /* Clamp inputs */
    if (wall_type < 0)          wall_type = 0;
    if (wall_type >= TEX_COUNT) wall_type = TEX_COUNT - 1;
    if (tex_x < 0)             tex_x = 0;
    if (tex_x >= TEX_SIZE)     tex_x = TEX_SIZE - 1;
    if (tex_y < 0)             tex_y = 0;
    if (tex_y >= TEX_SIZE)     tex_y = TEX_SIZE - 1;

    return pixels[wall_type * TEX_SIZE * TEX_SIZE + tex_y * TEX_SIZE + tex_x];
}
