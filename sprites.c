/*  sprites.c  –  sprite sorting (platform-independent)
 *  ───────────────────────────────────────────────────
 *  Sorts sprites back-to-front by squared distance to the player.
 *  No SDL headers.  Pure C.
 */
#include "sprites.h"

/* ── Sprite sorting ───────────────────────────────────────────────── */

int sprites_sort(const GameState *gs, int *sorted_order)
{
    int n = gs->sprite_count;
    if (n <= 0) return 0;
    if (n > MAX_SPRITES) n = MAX_SPRITES;

    float px = gs->player.x;
    float py = gs->player.y;

    /* Compute squared distances and initialise order array */
    float dist_sq[MAX_SPRITES];
    for (int i = 0; i < n; i++) {
        float dx = gs->sprites[i].x - px;
        float dy = gs->sprites[i].y - py;
        dist_sq[i] = dx * dx + dy * dy;
        sorted_order[i] = i;
    }

    /* Insertion sort (stable, fast for small N <= 64) — farthest first */
    for (int i = 1; i < n; i++) {
        int key_idx    = sorted_order[i];
        float key_dist = dist_sq[key_idx];
        int j = i - 1;
        while (j >= 0 && dist_sq[sorted_order[j]] < key_dist) {
            sorted_order[j + 1] = sorted_order[j];
            j--;
        }
        sorted_order[j + 1] = key_idx;
    }

    return n;
}
