#ifndef PLATFORM_SDL_H
#define PLATFORM_SDL_H

#include "game_globals.h"

/* ── Rendering colours (RGBA8888) ──────────────────────────────────── */
#define COL_CEIL       0xAAAAAAFF   /* ceiling (light grey)              */
#define COL_FLOOR      0x66666666   /* floor (dark grey)                 */
#define COL_WALL_SHADE 0x000068FF   /* shading reference (darker blue)   */

bool platform_init(void);
void platform_shutdown(void);
bool platform_poll_input(Input *in);
void platform_render(const GameState *gs);

/**  Render the end-game screen */
void platform_render_end_screen(void);

/**  Poll input during end screen. Returns false on quit or Escape. */
bool platform_poll_end_input(void);

/**  High-resolution timer: seconds since an arbitrary epoch. */
double platform_get_time(void);

#endif /* PLATFORM_SDL_H */
