#ifndef SPRITES_H
#define SPRITES_H

#include "game_globals.h"

/**  Collect visible sprites from the map grid and sort back-to-front.
 *   Scans map->sprites[][] for non-empty cells, builds Sprite instances
 *   at cell centres, and sorts by distance to the player (farthest first).
 *   out[] must have space for at least (map->w * map->h) entries.
 *   Returns the number of sprites written to out[]. */
int sprites_collect_and_sort(const Map *map, const Player *player,
                             Sprite *out, int max_out);

#endif /* SPRITES_H */
