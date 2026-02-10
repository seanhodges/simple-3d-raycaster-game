#ifndef RAYCASTER_H
#define RAYCASTER_H

#include "game_globals.h"

/* ── Raycasting constants ──────────────────────────────────────────── */
#define FOV_DEG   60.0f          /* field of view in degrees           */

/* ── Tiles plane values ───────────────────────────────────────────── */
#define TILE_FLOOR  0            /* empty floor (walkable)             */

/* ── Info plane values ────────────────────────────────────────────── */
#define INFO_EMPTY              0 /* no metadata at this cell          */
#define INFO_SPAWN_PLAYER_N     1 /* player spawn, facing north        */
#define INFO_SPAWN_PLAYER_E     2 /* player spawn, facing east         */
#define INFO_SPAWN_PLAYER_S     3 /* player spawn, facing south        */
#define INFO_SPAWN_PLAYER_W     4 /* player spawn, facing west         */
#define INFO_TRIGGER_ENDGAME    5 /* endgame trigger                   */

/* ── Public API ────────────────────────────────────────────────────── */

/**  Update player position/rotation from input.  dt in seconds. */
void rc_update(GameState *gs, const Map *map, const Input *in, float dt);

/**  Cast all rays and fill gs->hits[]. */
void rc_cast(GameState *gs, const Map *map);

#endif /* RAYCASTER_H */
