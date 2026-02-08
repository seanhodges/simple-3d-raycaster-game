#ifndef MAP_H
#define MAP_H

#include "raycaster.h"

/**  Load map from file. Fills map grid and player spawn. Returns false on failure. */
bool map_load(Map *map, Player *player, const char *path);

#endif /* MAP_H */
