#ifndef SPRITES_H
#define SPRITES_H

#include "game_globals.h"

/**  Sort sprites back-to-front by perpendicular distance (farthest first).
 *   Operates in-place using insertion sort. */
void sprites_sort(Sprite *sprites, int count);

#endif /* SPRITES_H */
