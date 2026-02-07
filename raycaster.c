/*  raycaster.c  –  platform-independent core logic
 *  ─────────────────────────────────────────────────
 *  No SDL headers.  Pure C + math.
 */
#include "raycaster.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI         3.14159265358979323846f
#define MOVE_SPD   3.0f    /* map-units / second                */
#define ROT_SPD    2.5f    /* radians  / second                 */
#define COL_MARGIN 0.15f   /* wall collision margin (map units) */

/* ── Map loading ───────────────────────────────────────────────────── */

bool rc_load_map(GameState *gs, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "rc_load_map: cannot open '%s'\n", path);
        return false;
    }

    memset(&gs->map, 0, sizeof(gs->map));

    char line[MAP_MAX_W + 2];          /* +newline +NUL */
    int  row = 0;
    bool player_set = false;

    while (fgets(line, sizeof(line), fp) && row < MAP_MAX_H) {
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        if (len > 0 && line[len - 1] == '\r') line[--len] = '\0';

        if (len > gs->map.w) gs->map.w = len;

        for (int col = 0; col < len; col++) {
            char c = line[col];
            if (c == 'X' || c == '#') {
                gs->map.cells[row][col] = 1;          /* wall, type 0 */
            } else if (c >= '0' && c <= '9') {
                gs->map.cells[row][col] = (c - '0') + 1; /* wall, type N */
            } else if (c == 'P' || c == 'p') {
                gs->map.cells[row][col] = CELL_FLOOR;  /* floor */
                gs->player.x = col + 0.5f;
                gs->player.y = row + 0.5f;
                player_set = true;
            } else if (c == 'F' || c == 'f') {
                gs->map.cells[row][col] = CELL_EXIT;   /* exit trigger */
            } else {
                gs->map.cells[row][col] = CELL_FLOOR;  /* empty */
            }
        }
        row++;
    }
    gs->map.h = row;
    fclose(fp);

    if (!player_set) {
        fprintf(stderr, "rc_load_map: no player start ('P') found\n");
        return false;
    }

    /* Default facing direction: east, with FOV-derived camera plane */
    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    gs->player.dir_x   =  1.0f;
    gs->player.dir_y   =  0.0f;
    gs->player.plane_x  =  0.0f;
    gs->player.plane_y  =  tanf(half_fov);

    return true;
}

/* ── Player movement / rotation ────────────────────────────────────── */

static bool is_wall(const Map *m, float x, float y)
{
    int mx = (int)x;
    int my = (int)y;
    if (mx < 0 || my < 0 || mx >= m->w || my >= m->h) return true;
    return m->cells[my][mx] > 0;
}

void rc_update(GameState *gs, const Input *in, float dt)
{
    Player *p = &gs->player;
    const Map *m = &gs->map;

    /* ── Rotation ─────────────────────────────────────────────────── */
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
    if (in->strafe_left) {
        dx -= p->plane_x * MOVE_SPD * dt;
        dy -= p->plane_y * MOVE_SPD * dt;
    }
    if (in->strafe_right) {
        dx += p->plane_x * MOVE_SPD * dt;
        dy += p->plane_y * MOVE_SPD * dt;
    }

    /* Slide along walls: test each axis independently with margin */
    if (!is_wall(m, p->x + dx + (dx > 0 ? COL_MARGIN : -COL_MARGIN), p->y))
        p->x += dx;
    if (!is_wall(m, p->x, p->y + dy + (dy > 0 ? COL_MARGIN : -COL_MARGIN)))
        p->y += dy;

    /* ── Exit detection (player must reach centre of exit cell) ──── */
    int cx = (int)p->x;
    int cy = (int)p->y;
    if (cx >= 0 && cy >= 0 && cx < m->w && cy < m->h
        && m->cells[cy][cx] == CELL_EXIT) {
        float centre_x = cx + 0.5f;
        float centre_y = cy + 0.5f;
        float ex = p->x - centre_x;
        float ey = p->y - centre_y;
        if (ex * ex + ey * ey <= COL_MARGIN * COL_MARGIN)
            gs->game_over = true;
    }
}

/* ── DDA Raycasting ────────────────────────────────────────────────── */

void rc_cast(GameState *gs)
{
    const Player *p = &gs->player;
    const Map    *m = &gs->map;

    for (int x = 0; x < SCREEN_W; x++) {
        /* Camera-space x: -1 (left) to +1 (right) */
        float cam_x = 2.0f * x / (float)SCREEN_W - 1.0f;

        float ray_dx = p->dir_x + p->plane_x * cam_x;
        float ray_dy = p->dir_y + p->plane_y * cam_x;

        /* Current map cell */
        int map_x = (int)p->x;
        int map_y = (int)p->y;

        /* Delta-dist: distance ray must travel to cross one cell boundary */
        float delta_dx = (ray_dx == 0.0f) ? 1e30f : fabsf(1.0f / ray_dx);
        float delta_dy = (ray_dy == 0.0f) ? 1e30f : fabsf(1.0f / ray_dy);

        float side_dx, side_dy;
        int step_x, step_y;

        if (ray_dx < 0) {
            step_x  = -1;
            side_dx = (p->x - map_x) * delta_dx;
        } else {
            step_x  = 1;
            side_dx = (map_x + 1.0f - p->x) * delta_dx;
        }
        if (ray_dy < 0) {
            step_y  = -1;
            side_dy = (p->y - map_y) * delta_dy;
        } else {
            step_y  = 1;
            side_dy = (map_y + 1.0f - p->y) * delta_dy;
        }

        /* ── DDA loop ─────────────────────────────────────────────── */
        int  side = 0;
        bool hit  = false;

        while (!hit) {
            if (side_dx < side_dy) {
                side_dx += delta_dx;
                map_x   += step_x;
                side = 0;
            } else {
                side_dy += delta_dy;
                map_y   += step_y;
                side = 1;
            }
            if (map_x < 0 || map_y < 0 || map_x >= m->w || map_y >= m->h) {
                hit = true;                    /* out of bounds = wall */
            } else if (m->cells[map_y][map_x] != 0) {
                hit = true;
            }
        }

        /* Perpendicular distance (avoids fish-eye) */
        float perp;
        if (side == 0)
            perp = (map_x - p->x + (1 - step_x) * 0.5f) / ray_dx;
        else
            perp = (map_y - p->y + (1 - step_y) * 0.5f) / ray_dy;

        if (perp < 0.001f) perp = 0.001f;

        /* Fractional position along the wall face (0.0 – 1.0) */
        float wall_x;
        if (side == 0)
            wall_x = p->y + perp * ray_dy;
        else
            wall_x = p->x + perp * ray_dx;
        wall_x -= floorf(wall_x);

        /* Extract wall_type from cell value (cell = wall_type + 1) */
        int cell = 0;
        if (map_x >= 0 && map_y >= 0 && map_x < m->w && map_y < m->h)
            cell = m->cells[map_y][map_x];

        gs->hits[x].wall_dist = perp;
        gs->hits[x].wall_x    = wall_x;
        gs->hits[x].side      = side;
        gs->hits[x].wall_type = (cell > 0) ? cell - 1 : 0;
    }
}
