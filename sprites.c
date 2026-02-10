/*  sprites.c  –  sprite collection and sorting (platform-independent)
 *  ──────────────────────────────────────────────────────────────────
 *  Scans the map's sprite plane, collects non-empty cells into a
 *  flat array, and sorts them back-to-front by distance to the player.
 *  No SDL headers.  Pure C.
 */
#include "sprites.h"

/* ── Collect and sort ─────────────────────────────────────────────── */

int sprites_collect_and_sort(const Map *map, const Player *player,
                             Sprite *out, int max_out)
{
    float px = player->x;
    float py = player->y;

    /* Pass 1: collect all non-empty sprite cells */
    int n = 0;
    for (int r = 0; r < map->h && n < max_out; r++) {
        for (int c = 0; c < map->w && n < max_out; c++) {
            uint16_t val = map->sprites[r][c];
            if (val == SPRITE_EMPTY) continue;
            out[n].x = c + 0.5f;
            out[n].y = r + 0.5f;
            out[n].texture_id = val - 1;  /* grid stores tex+1 */
            n++;
        }
    }

    if (n <= 1) return n;

    /* Pass 2: compute squared distances */
    float dist_sq[MAP_MAX_W * MAP_MAX_H];
    for (int i = 0; i < n; i++) {
        float dx = out[i].x - px;
        float dy = out[i].y - py;
        dist_sq[i] = dx * dx + dy * dy;
    }

    /* Insertion sort back-to-front (farthest first) */
    for (int i = 1; i < n; i++) {
        Sprite key_sprite = out[i];
        float  key_dist   = dist_sq[i];
        int j = i - 1;
        while (j >= 0 && dist_sq[j] < key_dist) {
            out[j + 1]     = out[j];
            dist_sq[j + 1] = dist_sq[j];
            j--;
        }
        out[j + 1]     = key_sprite;
        dist_sq[j + 1] = key_dist;
    }

    return n;
}
