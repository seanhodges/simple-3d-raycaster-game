#ifndef MAP_H
#define MAP_H

#include "game_globals.h"

/**  Load map from files. Fills tiles, info, and sprites planes plus player spawn.
 *   sprites_path may be NULL (sprites plane left zeroed).
 *   Returns false on failure. */
bool map_load(Map *map, Player *player, const char *tiles_path,
              const char *info_path, const char *sprites_path);

#endif /* MAP_H */
