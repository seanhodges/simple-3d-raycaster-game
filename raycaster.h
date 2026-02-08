#ifndef RAYCASTER_H
#define RAYCASTER_H

#include "game_globals.h"

/* ── Raycasting constants ──────────────────────────────────────────── */
#define FOV_DEG   60.0f          /* field of view in degrees           */

/* ── Special cell values ──────────────────────────────────────────── */
#define CELL_FLOOR  0            /* empty floor                        */

/* TODO(sean): Replace special cells like CELL_EXIT with a proper trigger in a dedicated map plane */
#define CELL_EXIT  65535         /* exit trigger (walkable floor)      */

/* ── Public API ────────────────────────────────────────────────────── */

/**  Update player position/rotation from input.  dt in seconds. */
void rc_update(GameState *gs, const Map *map, const Input *in, float dt);

/**  Cast all rays and fill gs->hits[]. */
void rc_cast(GameState *gs, const Map *map);

#endif /* RAYCASTER_H */
