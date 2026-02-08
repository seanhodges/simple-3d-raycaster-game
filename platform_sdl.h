#ifndef PLATFORM_SDL_H
#define PLATFORM_SDL_H

#include "raycaster.h"
#include <stdbool.h>

bool platform_init(const char *title);
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
