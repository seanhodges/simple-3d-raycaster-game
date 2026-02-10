/*  map_manager_fake.c  –  test-only map loader with hardcoded map
 *  ───────────────────────────────────────────────────────────────
 *  Provides a map_load() implementation that ignores the file paths
 *  and returns a small hardcoded map containing every tile type.
 *  Link against this instead of map_manager_ascii.o for unit tests
 *  that need a known, deterministic map without touching the filesystem.
 *
 *  Hardcoded tiles layout (7 wide × 5 tall):
 *
 *      col:  0  1  2  3  4  5  6
 *  row 0:    X  1  2  3  #  4  X      tiles: 1, 2, 3, 4, 1, 5, 1
 *  row 1:    5  .  .  .  .  6  X      tiles: 6, 0, 0, 0, 0, 7, 1
 *  row 2:    X  7  8  9  0  .  X      tiles: 1, 8, 9,10, 1, 0, 1
 *  row 3:    X  .  .  X  .  X  X      tiles: 1, 0, 0, 1, 0, 1, 1
 *  row 4:    X  X  X  X  X  X  X      tiles: all 1
 *
 *  Info plane:
 *  row 1, col 1: INFO_SPAWN_PLAYER_E  (player spawn facing east)
 *  row 1, col 3: INFO_TRIGGER_ENDGAME (endgame trigger)
 *
 *  Player spawn: (1.5, 1.5)  facing east
 *  Wall types present: 0 (X/#), 1–9 (digits)
 */
#include "map_manager.h"
#include "raycaster.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846f

/* ── Fake map data ─────────────────────────────────────────────────── */

static const int FAKE_W = 7;
static const int FAKE_H = 5;

static const uint16_t fake_tiles[5][7] = {
    /*        col 0  1  2  3  4  5  6  */
    /* row 0 */ { 1, 2, 3, 4, 1, 5, 1 },
    /* row 1 */ { 6, 0, 0, 0, 0, 7, 1 },
    /* row 2 */ { 1, 8, 9,10, 1, 0, 1 },
    /* row 3 */ { 1, 0, 0, 1, 0, 1, 1 },
    /* row 4 */ { 1, 1, 1, 1, 1, 1, 1 },
};

/* Player spawn position (tile centre of row 1, col 1) */
static const float FAKE_PLAYER_X = 1.5f;
static const float FAKE_PLAYER_Y = 1.5f;

/* ── map_load (fake implementation) ───────────────────────────────── */

bool map_load(Map *map, Player *player, const char *tiles_path,
              const char *info_path)
{
    (void)tiles_path;  /* ignored — always returns the hardcoded map */
    (void)info_path;

    memset(map, 0, sizeof(*map));
    map->w = FAKE_W;
    map->h = FAKE_H;

    for (int r = 0; r < FAKE_H; r++)
        for (int c = 0; c < FAKE_W; c++)
            map->tiles[r][c] = fake_tiles[r][c];

    /* Info plane: spawn and endgame trigger */
    map->info[1][1] = INFO_SPAWN_PLAYER_E;
    map->info[1][3] = INFO_TRIGGER_ENDGAME;

    player->x = FAKE_PLAYER_X;
    player->y = FAKE_PLAYER_Y;

    /* Facing east, with FOV-derived camera plane */
    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    player->dir_x   =  1.0f;
    player->dir_y   =  0.0f;
    player->plane_x  =  0.0f;
    player->plane_y  =  tanf(half_fov);

    return true;
}
