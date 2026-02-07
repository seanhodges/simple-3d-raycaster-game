/*  platform_sdl.c  –  SDL3 front-end / renderer
 *  ───────────────────────────────────────────────
 *  Reads the RayHit buffer from the core and draws vertical strips.
 *  Handles window lifecycle and keyboard input.
 */
#include "platform_sdl.h"
#include "raycaster.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

/* ── Internal state ────────────────────────────────────────────────── */
static SDL_Window   *window   = NULL;
static SDL_Renderer *renderer = NULL;

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

    return true;
}

void platform_shutdown(void)
{
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

void platform_render(const GameState *gs)
{
    int r, g, b, a;

    /* Clear to ceiling colour */
    rgba(COL_CEIL, &r, &g, &b, &a);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderClear(renderer);

    /* Draw floor (bottom half) */
    rgba(COL_FLOOR, &r, &g, &b, &a);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_FRect floor_rect = { 0, SCREEN_H / 2.0f, SCREEN_W, SCREEN_H / 2.0f };
    SDL_RenderFillRect(renderer, &floor_rect);

    /* Draw wall strips from the hit buffer */
    for (int x = 0; x < SCREEN_W; x++) {
        float dist = gs->hits[x].wall_dist;
        int line_h = (int)(SCREEN_H / dist);

        int draw_start = -line_h / 2 + SCREEN_H / 2;
        if (draw_start < 0) draw_start = 0;
        int draw_end = line_h / 2 + SCREEN_H / 2;
        if (draw_end >= SCREEN_H) draw_end = SCREEN_H - 1;

        /* Shade y-side hits slightly darker for depth cue */
        unsigned int col = (gs->hits[x].side == 1) ? COL_WALL_SHADE : COL_WALL;
        rgba(col, &r, &g, &b, &a);
        SDL_SetRenderDrawColor(renderer, r, g, b, a);

        SDL_FRect strip = { (float)x, (float)draw_start,
                            1.0f, (float)(draw_end - draw_start) };
        SDL_RenderFillRect(renderer, &strip);
    }

    /* Optional: debug overlay showing player coords */
    char dbg[64];
    snprintf(dbg, sizeof(dbg), "pos %.1f, %.1f", gs->player.x, gs->player.y);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDebugText(renderer, 8, 8, dbg);

    SDL_RenderPresent(renderer);
}

double platform_get_time(void)
{
    return (double)SDL_GetPerformanceCounter()
         / (double)SDL_GetPerformanceFrequency();
}
