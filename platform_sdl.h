#ifndef PLATFORM_SDL_H
#define PLATFORM_SDL_H

#include "raycaster.h"
#include <stdbool.h>

bool platform_init(const char *title);
void platform_shutdown(void);
bool platform_poll_input(Input *in);
void platform_render(const GameState *gs);

#endif /* PLATFORM_SDL_H */
