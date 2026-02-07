/*  platform_sdl.c  –  SDL3 front-end / renderer
 *  ───────────────────────────────────────────────
 *  Reads the RayHit buffer from the core and draws vertical strips.
 *  Handles window lifecycle and keyboard input.
 */
#include "platform_sdl.h"
#include "raycaster.h"
#include "texture.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

/* ── Internal state ────────────────────────────────────────────────── */
static SDL_Window   *window   = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture  *fb_tex   = NULL;  /* streaming framebuffer       */

/* ── Helpers: unpack RGBA8888 → 0-255 components ──────────────────── */
static void rgba(unsigned int c, int *r, int *g, int *b, int *a)
{
    *r = (c >> 24) & 0xFF;
    *g = (c >> 16) & 0xFF;
    *b = (c >>  8) & 0xFF;
    *a =  c        & 0xFF;
}

/* ── Public API ────────────────────────────────────────────────────── */

bool platform_init(const char *title)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "platform_init: SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(title, SCREEN_W, SCREEN_H, 0);
    if (!window) {
        fprintf(stderr, "platform_init: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        fprintf(stderr, "platform_init: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    fb_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               SCREEN_W, SCREEN_H);
    if (!fb_tex) {
        fprintf(stderr, "platform_init: SDL_CreateTexture failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void platform_shutdown(void)
{
    if (fb_tex)   SDL_DestroyTexture(fb_tex);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    SDL_Quit();
}

bool platform_poll_input(Input *in)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT)
            return false;
        if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE)
            return false;
    }

    /* Continuous key state (smoother than event-based) */
    const bool *ks = SDL_GetKeyboardState(NULL);

    in->forward      = ks[SDL_SCANCODE_W] || ks[SDL_SCANCODE_UP];
    in->back         = ks[SDL_SCANCODE_S] || ks[SDL_SCANCODE_DOWN];
    in->strafe_left  = ks[SDL_SCANCODE_A];
    in->strafe_right = ks[SDL_SCANCODE_D];
    in->turn_left    = ks[SDL_SCANCODE_LEFT];
    in->turn_right   = ks[SDL_SCANCODE_RIGHT];

    return true;   /* keep running */
}

/* ── Helpers: darken a colour for y-side shading ─────────────────── */

static unsigned int darken(unsigned int c)
{
    int r = (c >> 24) & 0xFF;
    int g = (c >> 16) & 0xFF;
    int b = (c >>  8) & 0xFF;
    int a =  c        & 0xFF;
    return (unsigned int)((r >> 1) << 24 | (g >> 1) << 16 | (b >> 1) << 8 | a);
}

void platform_render(const GameState *gs)
{
    /* Lock the streaming texture for direct pixel writes */
    void *tex_pixels = NULL;
    int   tex_pitch  = 0;
    if (!SDL_LockTexture(fb_tex, NULL, &tex_pixels, &tex_pitch)) {
        return;
    }
    unsigned int *fb = (unsigned int *)tex_pixels;
    int fb_stride = tex_pitch / 4;

    /* Fill the entire framebuffer with ceiling and floor colours */
    for (int y = 0; y < SCREEN_H / 2; y++)
        for (int x = 0; x < SCREEN_W; x++)
            fb[y * fb_stride + x] = COL_CEIL;
    for (int y = SCREEN_H / 2; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++)
            fb[y * fb_stride + x] = COL_FLOOR;

    /* Draw textured wall strips from the hit buffer */
    for (int x = 0; x < SCREEN_W; x++) {
        float dist = gs->hits[x].wall_dist;
        int line_h = (int)(SCREEN_H / dist);

        int draw_start = -line_h / 2 + SCREEN_H / 2;
        int draw_end   =  line_h / 2 + SCREEN_H / 2;

        /* Texture X coordinate from fractional wall hit position */
        int tex_x = (int)(gs->hits[x].wall_x * TEX_SIZE);
        if (tex_x >= TEX_SIZE) tex_x = TEX_SIZE - 1;

        int wt   = gs->hits[x].wall_type;
        int side = gs->hits[x].side;

        /* Clamp visible range to screen */
        int y_start = draw_start < 0 ? 0 : draw_start;
        int y_end   = draw_end >= SCREEN_H ? SCREEN_H - 1 : draw_end;

        for (int y = y_start; y <= y_end; y++) {
            /* Map screen Y to texture Y (0 .. TEX_SIZE-1) */
            int d = y * 2 - SCREEN_H + line_h;  /* offset from strip top */
            int tex_y = (d * TEX_SIZE) / (line_h * 2);
            if (tex_y < 0)            tex_y = 0;
            if (tex_y >= TEX_SIZE)    tex_y = TEX_SIZE - 1;

            unsigned int col = tm_get_pixel(wt, tex_x, tex_y);

            /* Darken y-side hits for depth cue */
            if (side == 1) col = darken(col);

            fb[y * fb_stride + x] = col;
        }
    }

    SDL_UnlockTexture(fb_tex);

    /* Blit the framebuffer to screen */
    SDL_RenderTexture(renderer, fb_tex, NULL, NULL);

    /* Debug overlay showing player coords */
    char dbg[64];
    snprintf(dbg, sizeof(dbg), "pos %.1f, %.1f", gs->player.x, gs->player.y);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDebugText(renderer, 8, 8, dbg);

    SDL_RenderPresent(renderer);
}

/* ── End-screen rendering ─────────────────────────────────────────── */

void platform_render_end_screen(const GameState *gs)
{
    /* Draw the 3D scene as background (reuse normal render path) */
    platform_render(gs);

    /* Semi-transparent dark overlay */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect overlay = {0, 0, SCREEN_W, SCREEN_H};
    SDL_RenderFillRect(renderer, &overlay);

    /* Large title text (scale 4×) */
    const char *title = "Congratulations! You found the exit.";
    float title_scale = 4.0f;
    float char_w = 8.0f * title_scale;
    float title_w = (float)strlen(title) * char_w;
    float title_x = (SCREEN_W - title_w) / 2.0f;
    float title_y = SCREEN_H / 2.0f - 60.0f;

    SDL_SetRenderScale(renderer, title_scale, title_scale);
    SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
    SDL_RenderDebugText(renderer, title_x / title_scale,
                        title_y / title_scale, title);

    /* Smaller subtitle text (scale 2×) */
    const char *sub = "Press Esc to end the game.";
    float sub_scale = 2.0f;
    float sub_char_w = 8.0f * sub_scale;
    float sub_w = (float)strlen(sub) * sub_char_w;
    float sub_x = (SCREEN_W - sub_w) / 2.0f;
    float sub_y = SCREEN_H / 2.0f + 20.0f;

    SDL_SetRenderScale(renderer, sub_scale, sub_scale);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDebugText(renderer, sub_x / sub_scale,
                        sub_y / sub_scale, sub);

    /* Reset scale and present */
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    SDL_RenderPresent(renderer);
}

bool platform_poll_end_input(void)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT)
            return false;
        if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE)
            return false;
    }
    return true;
}

/* ── Timer ─────────────────────────────────────────────────────────── */

double platform_get_time(void)
{
    return (double)SDL_GetPerformanceCounter()
         / (double)SDL_GetPerformanceFrequency();
}
