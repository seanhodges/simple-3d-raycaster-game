/*  sprites.c  –  sprite sorting (platform-independent)
 *  ──────────────────────────────────────────────────────────────────
 *  Sorts a pre-collected array of visible sprites back-to-front by
 *  perpendicular distance for correct painter's-algorithm rendering.
 *  No SDL headers.  Pure C.
 */
#include "sprites.h"

/* ── Sort ──────────────────────────────────────────────────────────── */

void sprites_sort(Sprite *sprites, int count)
{
    if (count <= 1) return;

    /* Insertion sort back-to-front (farthest first) by perp_dist */
    for (int i = 1; i < count; i++) {
        Sprite key = sprites[i];
        float  kd  = key.perp_dist;
        int j = i - 1;
        while (j >= 0 && sprites[j].perp_dist < kd) {
            sprites[j + 1] = sprites[j];
            j--;
        }
        sprites[j + 1] = key;
    }
}
