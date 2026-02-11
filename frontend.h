#ifndef FRONTEND_H
#define FRONTEND_H

#include "game_globals.h"

/* ── Rendering colours (RGBA8888) ──────────────────────────────────── */
#define COL_CEIL       0xAAAAAAFF   /* ceiling (light grey)              */
#define COL_FLOOR      0x66666666   /* floor (dark grey)                 */
#define COL_WALL_SHADE 0x000068FF   /* shading reference (darker blue)   */

bool frontend_init(void);
void frontend_shutdown(void);
bool frontend_poll_input(Input *in);
void frontend_render(const GameState *gs);

/**  Render the end-game screen */
void frontend_render_end_screen(void);

/**  Poll input during end screen. Returns false on quit or Escape. */
bool frontend_poll_end_input(void);

/**  High-resolution timer: seconds since an arbitrary epoch. */
double frontend_get_time(void);

#endif /* FRONTEND_H */
