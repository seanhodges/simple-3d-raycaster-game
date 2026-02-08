#ifndef RAYCASTER_H
#define RAYCASTER_H

#include "game_globals.h"

/* ── Display constants ─────────────────────────────────────────────── */
#define SCREEN_H  600
#define FOV_DEG   60.0f

/* ── Texture constants ────────────────────────────────────────────── */
#define TEX_SIZE  64             /* width & height of one texture tile */
#define TEX_COUNT 10             /* number of textures in the atlas    */

/* ── Special cell values ──────────────────────────────────────────── */
#define CELL_FLOOR  0            /* empty floor                        */
#define CELL_EXIT  -1            /* exit trigger (walkable floor)      */

/* ── Colours (RGBA8888) ────────────────────────────────────────────── */
#define COL_CEIL  0xAAAAAAFF   /* white */
#define COL_FLOOR 0x66666666   /* grey */
#define COL_WALL  0x00008BFF   /* dk blue */
#define COL_WALL_SHADE 0x000068FF /* slightly darker for y-side hits */

/* ── Public API ────────────────────────────────────────────────────── */

/**  Update player position/rotation from input.  dt in seconds. */
void rc_update(GameState *gs, const Map *map, const Input *in, float dt);

/**  Cast all rays and fill gs->hits[]. */
void rc_cast(GameState *gs, const Map *map);

#endif /* RAYCASTER_H */
