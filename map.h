#ifndef MAP_H
#define MAP_H

#include "raycaster.h"

/**  Load map from file into gs->map and gs->player. Returns false on failure. */
bool map_load(GameState *gs, const char *path);

#endif /* MAP_H */
