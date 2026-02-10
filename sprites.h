#ifndef SPRITES_H
#define SPRITES_H

#include "game_globals.h"

/**  Sort sprites by distance to player (back-to-front, farthest first).
 *   Fills sorted_order[] with indices into gs->sprites[].
 *   sorted_order must hold at least gs->sprite_count entries.
 *   Returns the number of sprites written to sorted_order. */
int sprites_sort(const GameState *gs, int *sorted_order);

#endif /* SPRITES_H */
