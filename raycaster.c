/*  raycaster.c  –  platform-independent core logic
 *  ─────────────────────────────────────────────────
 *  No SDL headers.  Pure C + math.
 */
#include "raycaster.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOVE_SPD   3.0f    /* map-units / second                */
#define ROT_SPD    2.5f    /* radians  / second                 */
#define COL_MARGIN 0.15f   /* wall collision margin (map units) */

/* ── Player movement / rotation ────────────────────────────────────── */

/**  Check if a position is inside a wall.
 *   Returns true for walls (tile > 0) and out-of-bounds positions. */
static bool is_wall(const Map *m, float x, float y)
{
    int mx = (int)x;
    int my = (int)y;
    if (mx < 0 || my < 0 || mx >= m->w || my >= m->h) return true;
    return m->tiles[my][mx] > TILE_FLOOR;
}

void rc_update(GameState *gs, const Map *map, const Input *in, float dt)
{
    Player *p = &gs->player;

    /* ── Rotation ─────────────────────────────────────────────────── */
    /* Apply 2D rotation matrix to both direction and camera plane vectors.
     * Rotation matrix: [cos -sin]   transforms a vector (x, y) by angle rot.
     *                  [sin  cos]
     * Both direction and plane must rotate together to maintain FOV. */
    float rot = 0.0f;
    if (in->turn_left)  rot = -ROT_SPD * dt;
    if (in->turn_right) rot =  ROT_SPD * dt;

    if (rot != 0.0f) {
        float c = cosf(rot), s = sinf(rot);
        float od = p->dir_x;
        p->dir_x   = p->dir_x   * c - p->dir_y   * s;
        p->dir_y   = od          * s + p->dir_y   * c;
        float op = p->plane_x;
        p->plane_x = p->plane_x * c - p->plane_y * s;
        p->plane_y = op          * s + p->plane_y * c;
    }

    /* ── Translation ──────────────────────────────────────────────── */
    float dx = 0.0f, dy = 0.0f;

    if (in->forward) {
        dx += p->dir_x * MOVE_SPD * dt;
        dy += p->dir_y * MOVE_SPD * dt;
    }
    if (in->back) {
        dx -= p->dir_x * MOVE_SPD * dt;
        dy -= p->dir_y * MOVE_SPD * dt;
    }

    /* Axis-independent collision: test X and Y separately with a margin.
     * This enables "wall sliding" – if you hit a wall diagonally, you
     * slide along it instead of stopping. Note: X is tested first, then
     * Y uses the updated p->x to prevent corner-cutting through walls. */
    if (!is_wall(map, p->x + dx + (dx > 0 ? COL_MARGIN : -COL_MARGIN), p->y))
        p->x += dx;
    if (!is_wall(map, p->x, p->y + dy + (dy > 0 ? COL_MARGIN : -COL_MARGIN)))
        p->y += dy;

    /* ── Endgame detection (player must reach centre of trigger) ─── */
    int cx = (int)p->x;
    int cy = (int)p->y;
    if (cx >= 0 && cy >= 0 && cx < map->w && cy < map->h
        && map->info[cy][cx] == INFO_TRIGGER_ENDGAME) {

        float centre_x = cx + 0.5f;
        float centre_y = cy + 0.5f;
        float ex = p->x - centre_x;
        float ey = p->y - centre_y;
        if (ex * ex + ey * ey <= COL_MARGIN * COL_MARGIN)
            gs->game_over = true;
    }
}

/* ── Sprite sorting ────────────────────────────────────────────────── */

/** qsort comparator: sort sprites by perp_dist descending (farthest first)
 *  so the painter's algorithm draws distant sprites before near ones. */
static int sprite_cmp_desc(const void *a, const void *b)
{
    float da = ((const Sprite *)a)->perp_dist;
    float db = ((const Sprite *)b)->perp_dist;
    if (da > db) return -1;
    if (da < db) return  1;
    return 0;
}

/** Sort visible sprites back-to-front using qsort for O(N log N). */
static void sort_visible_sprites(GameState *gs)
{
    if (gs->visible_sprite_count > 1)
        qsort(gs->visible_sprites, (size_t)gs->visible_sprite_count,
              sizeof(Sprite), sprite_cmp_desc);
}

/* ── DDA Raycasting ────────────────────────────────────────────────── */
/* Digital Differential Analyzer (DDA) – an efficient grid-traversal algorithm.
 * For each screen column, cast one ray from the player's eye through the
 * scene. Step through the map grid cell-by-cell until hitting a wall.
 * The distance to the wall determines the height of the vertical strip drawn. */

void rc_cast(GameState *gs, const Map *map)
{
    const Player *p = &gs->player;

    /* Reset visible sprite list and visited bitmap for deduplication */
    gs->visible_sprite_count = 0;
    bool seen[MAP_MAX_H][MAP_MAX_W];
    memset(seen, 0, sizeof(seen));

    /* Inverse camera matrix determinant for sprite perpendicular distance.
     * perp_dist = inv_det * (-plane_y * sx + plane_x * sy)
     * where (sx, sy) is the sprite position relative to the player. */
    float inv_det = 1.0f / (p->plane_x * p->dir_y - p->dir_x * p->plane_y);

    /* Check the player's starting cell for sprites (not visited by DDA) */
    {
        int cx = (int)p->x;
        int cy = (int)p->y;
        if (cx >= 0 && cy >= 0 && cx < map->w && cy < map->h
            && map->sprites[cy][cx] != SPRITE_EMPTY) {
            seen[cy][cx] = true;
            float sx = cx + 0.5f - p->x;
            float sy = cy + 0.5f - p->y;
            float pd = inv_det * (-p->plane_y * sx + p->plane_x * sy);
            if (pd > 0.0f && gs->visible_sprite_count < MAX_VISIBLE_SPRITES) {
                Sprite *sp = &gs->visible_sprites[gs->visible_sprite_count++];
                sp->x = cx + 0.5f;
                sp->y = cy + 0.5f;
                sp->perp_dist   = pd;
                sp->texture_id  = map->sprites[cy][cx] - 1;
            }
        }
    }

    for (int x = 0; x < SCREEN_W; x++) {
        /* Camera-space x: -1 (left edge) to +1 (right edge).
         * This maps screen column to a position across the camera plane. */
        float cam_x = 2.0f * x / (float)SCREEN_W - 1.0f;

        /* Ray direction = player direction + (camera plane * cam_x).
         * This creates a ray that sweeps across the FOV as x goes 0→SCREEN_W. */
        float ray_dx = p->dir_x + p->plane_x * cam_x;
        float ray_dy = p->dir_y + p->plane_y * cam_x;

        /* Start in the map cell containing the player */
        int map_x = (int)p->x;
        int map_y = (int)p->y;

        /* Delta-dist: how far the ray travels to cross one full grid cell.
         * If ray_dx = 1, crossing one X cell takes exactly 1 unit. If ray_dx = 0.5,
         * it takes 2 units (hence 1 / ray_dx). We use absolute value because
         * distance is always positive. 1e30 (large number) handles zero direction. */
        float delta_dx = (ray_dx == 0.0f) ? 1e30f : fabsf(1.0f / ray_dx);
        float delta_dy = (ray_dy == 0.0f) ? 1e30f : fabsf(1.0f / ray_dy);

        /* Side-dist: distance from player to the NEXT grid boundary on each axis.
         * Step: which direction to move on the grid (+1 or -1).
         * If ray points left (ray_dx < 0), we step -1 and measure distance to
         * the left edge of the current cell. Otherwise, step +1 to the right edge. */
        float side_dx, side_dy;
        int step_x, step_y;

        if (ray_dx < 0) {
            step_x  = -1;
            side_dx = (p->x - map_x) * delta_dx;   /* dist to left edge */
        } else {
            step_x  = 1;
            side_dx = (map_x + 1.0f - p->x) * delta_dx;  /* dist to right edge */
        }
        if (ray_dy < 0) {
            step_y  = -1;
            side_dy = (p->y - map_y) * delta_dy;   /* dist to top edge */
        } else {
            step_y  = 1;
            side_dy = (map_y + 1.0f - p->y) * delta_dy;  /* dist to bottom edge */
        }

        /* ── DDA loop ─────────────────────────────────────────────── */
        /* Step through the grid one cell at a time, always advancing along
         * the axis where the next boundary is closest. side=0 means we crossed
         * an X boundary (vertical wall face), side=1 means Y boundary (horizontal). */
        int  side = 0;
        bool hit  = false;

        while (!hit) {
            /* Compare distances to next boundary on each axis – step the shorter one */
            if (side_dx < side_dy) {
                side_dx += delta_dx;      /* advance to next X boundary */
                map_x   += step_x;
                side = 0;                 /* hit a vertical wall face */
            } else {
                side_dy += delta_dy;      /* advance to next Y boundary */
                map_y   += step_y;
                side = 1;                 /* hit a horizontal wall face */
            }
            /* Check if we hit a wall or went out of bounds */
            if (map_x < 0 || map_y < 0 || map_x >= map->w || map_y >= map->h) {
                hit = true;                    /* out of bounds = wall */
            } else if (map->tiles[map_y][map_x] > TILE_FLOOR) {
                hit = true;
            } else if (!seen[map_y][map_x]
                       && map->sprites[map_y][map_x] != SPRITE_EMPTY) {
                /* Floor cell with an unseen sprite — collect it */
                seen[map_y][map_x] = true;
                float sx = map_x + 0.5f - p->x;
                float sy = map_y + 0.5f - p->y;
                float pd = inv_det * (-p->plane_y * sx + p->plane_x * sy);
                if (pd > 0.0f && gs->visible_sprite_count < MAX_VISIBLE_SPRITES) {
                    Sprite *sp = &gs->visible_sprites[gs->visible_sprite_count++];
                    sp->x = map_x + 0.5f;
                    sp->y = map_y + 0.5f;
                    sp->perp_dist   = pd;
                    sp->texture_id  = map->sprites[map_y][map_x] - 1;
                }
            }
        }

        /* Perpendicular distance: project the hit point onto the camera plane.
         * Using Euclidean distance would cause "fish-eye" – walls at screen edges
         * would look curved because rays to the edges are longer. Instead, we
         * measure how far FORWARD (perpendicular to camera plane) the ray traveled.
         * Formula: (hit_cell_edge - player_pos) / ray_direction
         * The (1 - step) * 0.5 term corrects for which edge of the cell we hit. */
        float perp;
        if (side == 0)
            perp = (map_x - p->x + (1 - step_x) * 0.5f) / ray_dx;
        else
            perp = (map_y - p->y + (1 - step_y) * 0.5f) / ray_dy;

        if (perp < 0.001f) perp = 0.001f;  /* clamp to avoid division by zero in rendering */

        /* Fractional position along the wall face (0.0 – 1.0), used for texture X.
         * If we hit a vertical wall (side=0), use Y coordinate; otherwise use X.
         * This gives us the exact spot where the ray intersected the wall face.
         * Subtract the integer part to get only the fractional portion. */
        float wall_x;
        if (side == 0)
            wall_x = p->y + perp * ray_dy;   /* vertical wall: Y varies along face */
        else
            wall_x = p->x + perp * ray_dx;   /* horizontal wall: X varies along face */
        wall_x -= floorf(wall_x);            /* keep only fractional part [0.0, 1.0) */

        /* Extract tile_type (texture index) from tile value.
         * Tile encoding: 0 = floor, 1 = tile type 0, 2 = tile type 1, etc.
         * So tile_type = tile - 1. Out-of-bounds tiles default to type 0. */
        int tile = 0;
        if (map_x >= 0 && map_y >= 0 && map_x < map->w && map_y < map->h)
            tile = map->tiles[map_y][map_x];

        /* Store results in the hit buffer for this column – the renderer reads this */
        gs->hits[x].wall_dist = perp;
        gs->hits[x].wall_x    = wall_x;
        gs->hits[x].side      = side;
        gs->hits[x].tile_type = (tile > 0) ? tile - 1 : 0;

        /* Store perpendicular distance in z-buffer for sprite clipping */
        gs->z_buffer[x] = perp;
    }

    /* Sort visible sprites back-to-front for correct painter's order */
    sort_visible_sprites(gs);
}
