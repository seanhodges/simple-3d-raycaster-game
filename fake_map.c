/*  fake_map.c  –  test-only map loader with hardcoded map
 *  ─────────────────────────────────────────────────────
 *  Provides a map_load() implementation that ignores the file path
 *  and returns a small hardcoded map containing every cell type.
 *  Link against this instead of map.o for unit tests that need a
 *  known, deterministic map without touching the filesystem.
 *
 *  Hardcoded map layout (7 wide × 5 tall):
 *
 *      col:  0  1  2  3  4  5  6
 *  row 0:    X  1  2  3  #  4  X      walls: X(1), 1(2), 2(3), 3(4), #(1), 4(5), X(1)
 *  row 1:    5  P     F     6  X      5(6), player(0), floor(0), exit(-1), floor(0), 6(7), X(1)
 *  row 2:    X  7  8  9  0     X      X(1), 7(8), 8(9), 9(10), 0(1), floor(0), X(1)
 *  row 3:    X        X     X  X      X(1), floor(0), floor(0), X(1), floor(0), X(1), X(1)
 *  row 4:    X  X  X  X  X  X  X      all walls
 *
 *  Player spawn: (1.5, 1.5)  facing east
 *  Exit cell:    (3, 1)
 *  Wall types present: 0 (X/#), 1–9 (digits)
 */
#include "map.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846f

/* ── Fake map data ─────────────────────────────────────────────────── */

static const int FAKE_W = 7;
static const int FAKE_H = 5;

static const int fake_cells[5][7] = {
    /*        col 0  1  2  3  4  5  6  */
    /* row 0 */ { 1, 2, 3, 4, 1, 5, 1 },
    /* row 1 */ { 6, 0, 0,-1, 0, 7, 1 },
    /* row 2 */ { 1, 8, 9,10, 1, 0, 1 },
    /* row 3 */ { 1, 0, 0, 1, 0, 1, 1 },
    /* row 4 */ { 1, 1, 1, 1, 1, 1, 1 },
};

/* Player spawn position (cell centre of row 1, col 1) */
static const float FAKE_PLAYER_X = 1.5f;
static const float FAKE_PLAYER_Y = 1.5f;

/* ── map_load (fake implementation) ───────────────────────────────── */

bool map_load(GameState *gs, const char *path)
{
    (void)path;   /* ignored — always returns the hardcoded map */

    memset(&gs->map, 0, sizeof(gs->map));
    gs->map.w = FAKE_W;
    gs->map.h = FAKE_H;

    for (int r = 0; r < FAKE_H; r++)
        for (int c = 0; c < FAKE_W; c++)
            gs->map.cells[r][c] = fake_cells[r][c];

    gs->player.x = FAKE_PLAYER_X;
    gs->player.y = FAKE_PLAYER_Y;

    /* Default facing direction: east, with FOV-derived camera plane */
    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    gs->player.dir_x   =  1.0f;
    gs->player.dir_y   =  0.0f;
    gs->player.plane_x  =  0.0f;
    gs->player.plane_y  =  tanf(half_fov);

    return true;
}
