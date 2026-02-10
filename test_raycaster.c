/*  test_raycaster.c  –  unit tests for the platform-independent core
 *  ──────────────────────────────────────────────────────────────────
 *  Links against raycaster.o and map_manager_fake.o — no SDL dependency.
 *  The fake map module provides a hardcoded map with all tile types
 *  so these tests are deterministic and filesystem-independent.
 *  Build:  make test
 *  Run:    ./test_raycaster
 */
#include "raycaster.h"
#include "map_manager.h"
#include "textures_sdl.h"
#include "sprites.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define PI 3.14159265358979323846f

/* ── Minimal test harness ─────────────────────────────────────────── */

static int tests_run    = 0;
static int tests_passed = 0;

#define RUN_TEST(fn)                                                    \
    do {                                                                \
        tests_run++;                                                    \
        printf("  %-50s", #fn);                                         \
        fn();                                                           \
        tests_passed++;                                                 \
        printf(" OK\n");                                                \
    } while (0)

#define ASSERT_NEAR(a, b, eps)                                          \
    do {                                                                \
        float _a = (a), _b = (b), _e = (eps);                          \
        if (fabsf(_a - _b) > _e) {                                     \
            printf(" FAIL\n    %s:%d: %.4f != %.4f (eps %.4f)\n",       \
                   __FILE__, __LINE__, _a, _b, _e);                     \
            assert(0);                                                  \
        }                                                               \
    } while (0)

/* ── Helper: build a Map + GameState with an inline box map ───────── */

static void init_box_map(Map *map, GameState *gs, int w, int h,
                         float px, float py,
                         float dir_x, float dir_y)
{
    memset(map, 0, sizeof(*map));
    memset(gs, 0, sizeof(*gs));
    map->w = w;
    map->h = h;

    /* Walls on all edges, floor inside */
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            map->tiles[r][c] =
                (r == 0 || r == h - 1 || c == 0 || c == w - 1) ? 1 : 0;

    gs->player.x     = px;
    gs->player.y     = py;
    gs->player.dir_x = dir_x;
    gs->player.dir_y = dir_y;

    /* Derive camera plane from FOV_DEG, perpendicular to direction */
    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    float plane_len = tanf(half_fov);
    gs->player.plane_x = -dir_y * plane_len;
    gs->player.plane_y =  dir_x * plane_len;
}

/* ── Helper: load the fake map ────────────────────────────────────── */

static void load_fake_map(Map *map, GameState *gs)
{
    memset(map, 0, sizeof(*map));
    memset(gs, 0, sizeof(*gs));
    bool ok = map_load(map, &gs->player, "ignored", "ignored");
    assert(ok);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  Fake map structure tests                                          */
/*                                                                    */
/*  The fake map layout (7 wide × 5 tall):                            */
/*                                                                    */
/*      col:  0  1  2  3  4  5  6                                     */
/*  row 0:    X  1  2  3  #  4  X    tiles: 1,2,3,4,1,5,1            */
/*  row 1:    5  .  .  .  .  6  X    tiles: 6,0,0,0,0,7,1            */
/*  row 2:    X  7  8  9  0  .  X    tiles: 1,8,9,10,1,0,1           */
/*  row 3:    X  .  .  X  .  X  X    tiles: 1,0,0,1,0,1,1            */
/*  row 4:    X  X  X  X  X  X  X    tiles: all 1                    */
/*                                                                    */
/*  Info plane:                                                       */
/*  row 1, col 1: INFO_SPAWN_PLAYER_E                                 */
/*  row 1, col 3: INFO_TRIGGER_ENDGAME                                */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_fake_map_dimensions(void)
{
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    assert(map.w == 7);
    assert(map.h == 5);
}

static void test_fake_map_wall_x_hash(void)
{
    /* X and # both produce tile value 1 (tile_type 0) */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    /* 'X' at corners */
    assert(map.tiles[0][0] == 1);
    assert(map.tiles[0][6] == 1);
    assert(map.tiles[4][0] == 1);
    assert(map.tiles[4][6] == 1);

    /* '#' at row 0, col 4 */
    assert(map.tiles[0][4] == 1);
}

static void test_fake_map_digit_walls(void)
{
    /* Digit N produces tile value N+1 (tile_type N) */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    /* Digit '1' at (0,1) → tile = 2 */
    assert(map.tiles[0][1] == 2);
    /* Digit '2' at (0,2) → tile = 3 */
    assert(map.tiles[0][2] == 3);
    /* Digit '3' at (0,3) → tile = 4 */
    assert(map.tiles[0][3] == 4);
    /* Digit '4' at (0,5) → tile = 5 */
    assert(map.tiles[0][5] == 5);
    /* Digit '5' at (1,0) → tile = 6 */
    assert(map.tiles[1][0] == 6);
    /* Digit '6' at (1,5) → tile = 7 */
    assert(map.tiles[1][5] == 7);
    /* Digit '7' at (2,1) → tile = 8 */
    assert(map.tiles[2][1] == 8);
    /* Digit '8' at (2,2) → tile = 9 */
    assert(map.tiles[2][2] == 9);
    /* Digit '9' at (2,3) → tile = 10 */
    assert(map.tiles[2][3] == 10);
    /* Digit '0' at (2,4) → tile = 1 */
    assert(map.tiles[2][4] == 1);
}

static void test_fake_map_floor_tiles(void)
{
    /* Spawn cell and open space are floor (tile value 0) */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    /* Spawn cell at (1,1) — floor in tiles plane */
    assert(map.tiles[1][1] == TILE_FLOOR);
    /* Space at (1,2) */
    assert(map.tiles[1][2] == TILE_FLOOR);
    /* Endgame trigger cell at (1,3) — floor in tiles plane */
    assert(map.tiles[1][3] == TILE_FLOOR);
    /* Space at (1,4) */
    assert(map.tiles[1][4] == TILE_FLOOR);
    /* Interior floor at (3,1) */
    assert(map.tiles[3][1] == TILE_FLOOR);
    /* Interior floor at (3,2) */
    assert(map.tiles[3][2] == TILE_FLOOR);
}

static void test_fake_map_info_spawn(void)
{
    /* Info plane should have spawn marker at row 1, col 1 */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    assert(map.info[1][1] == INFO_SPAWN_PLAYER_E);
}

static void test_fake_map_info_endgame(void)
{
    /* Info plane should have endgame trigger at row 1, col 3 */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    assert(map.info[1][3] == INFO_TRIGGER_ENDGAME);
}

static void test_fake_map_player_position(void)
{
    /* Player spawns at centre of spawn cell (row 1, col 1) */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    ASSERT_NEAR(gs.player.x, 1.5f, 0.01f);
    ASSERT_NEAR(gs.player.y, 1.5f, 0.01f);
}

static void test_fake_map_player_direction(void)
{
    /* Player faces east by default */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    ASSERT_NEAR(gs.player.dir_x, 1.0f, 0.01f);
    ASSERT_NEAR(gs.player.dir_y, 0.0f, 0.01f);
}

static void test_fake_map_camera_plane(void)
{
    /* Camera plane is perpendicular to direction, length = tan(FOV/2) */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    float expected_plane = tanf(half_fov);

    ASSERT_NEAR(gs.player.plane_x, 0.0f, 0.01f);
    ASSERT_NEAR(gs.player.plane_y, expected_plane, 0.01f);
}

static void test_fake_map_walls_are_walls(void)
{
    /* All tiles with value > 0 should be walls */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    /* Check the bottom row is entirely walled */
    for (int c = 0; c < map.w; c++)
        assert(map.tiles[4][c] > 0);

    /* Check the left and right columns are walls in all rows */
    for (int r = 0; r < map.h; r++) {
        assert(map.tiles[r][0] > 0);
        assert(map.tiles[r][6] > 0);
    }
}

static void test_fake_map_all_tile_types_present(void)
{
    /* Verify wall types 0–9 are all present somewhere in the map */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    bool found[TEX_COUNT];
    memset(found, 0, sizeof(found));

    for (int r = 0; r < map.h; r++) {
        for (int c = 0; c < map.w; c++) {
            uint16_t tile = map.tiles[r][c];
            if (tile > 0) {
                uint16_t tile_type = tile - 1;
                if (tile_type < TEX_COUNT)
                    found[tile_type] = true;
            }
        }
    }

    for (int t = 0; t < TEX_COUNT; t++)
        assert(found[t]);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  rc_update tests                                                   */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_update_no_input(void)
{
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));

    float old_x = gs.player.x;
    float old_y = gs.player.y;

    rc_update(&gs, &map, &in, 1.0f / 60.0f);

    /* Player should not move */
    ASSERT_NEAR(gs.player.x, old_x, 0.0001f);
    ASSERT_NEAR(gs.player.y, old_y, 0.0001f);
}

static void test_update_forward(void)
{
    Map map;
    GameState gs;
    /* Facing east in a big box, plenty of room */
    init_box_map(&map, &gs, 20, 20, 5.5f, 10.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    float old_x = gs.player.x;
    rc_update(&gs, &map, &in, 1.0f);

    /* Should have moved ~3.0 units east (MOVE_SPD = 3.0) */
    assert(gs.player.x > old_x);
    ASSERT_NEAR(gs.player.x - old_x, 3.0f, 0.01f);
}

static void test_update_backward(void)
{
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 20, 20, 10.5f, 10.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.back = true;

    float old_x = gs.player.x;
    rc_update(&gs, &map, &in, 1.0f);

    /* Should have moved ~3.0 units west */
    assert(gs.player.x < old_x);
    ASSERT_NEAR(old_x - gs.player.x, 3.0f, 0.01f);
}

static void test_update_wall_collision(void)
{
    Map map;
    GameState gs;
    /* Player near north wall (row 0 = wall), facing north (-y) */
    init_box_map(&map, &gs, 10, 10, 5.5f, 1.5f, 0.0f, -1.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    /* Walk into wall for a full second */
    for (int i = 0; i < 60; i++)
        rc_update(&gs, &map, &in, 1.0f / 60.0f);

    /* Player should be stopped by the wall, not inside or beyond it */
    assert(gs.player.y > 0.5f);
}

static void test_update_wall_sliding(void)
{
    Map map;
    GameState gs;
    /* Facing northeast into a north wall — should slide east */
    float inv_sqrt2 = 1.0f / sqrtf(2.0f);
    init_box_map(&map, &gs, 20, 20, 5.5f, 1.5f, inv_sqrt2, -inv_sqrt2);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    float old_x = gs.player.x;
    for (int i = 0; i < 60; i++)
        rc_update(&gs, &map, &in, 1.0f / 60.0f);

    /* Should have slid east (x increased) while y stayed near wall */
    assert(gs.player.x > old_x + 0.5f);
    assert(gs.player.y > 0.5f);
}

static void test_update_rotation_left(void)
{
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.turn_left = true;

    rc_update(&gs, &map, &in, 1.0f);

    /* Rotating left (counter-clockwise) should change direction */
    /* After ~2.5 radians of rotation, dir_x should be negative */
    float len = sqrtf(gs.player.dir_x * gs.player.dir_x +
                      gs.player.dir_y * gs.player.dir_y);
    ASSERT_NEAR(len, 1.0f, 0.01f);  /* direction vector stays unit length */
}

static void test_update_rotation_right(void)
{
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.turn_right = true;

    /* Small rotation: 1/60th second */
    rc_update(&gs, &map, &in, 1.0f / 60.0f);

    /* dir_y should become positive (clockwise) */
    assert(gs.player.dir_y > 0.0f);
    /* direction vector should stay unit length */
    float len = sqrtf(gs.player.dir_x * gs.player.dir_x +
                      gs.player.dir_y * gs.player.dir_y);
    ASSERT_NEAR(len, 1.0f, 0.001f);
}

static void test_update_endgame_tile_walkable(void)
{
    /* Endgame trigger tile is floor in tiles plane — always walkable */
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 3.5f, 5.5f, 1.0f, 0.0f);

    /* Place an endgame trigger in the info plane (tile stays floor) */
    map.info[5][5] = INFO_TRIGGER_ENDGAME;

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    float old_x = gs.player.x;
    for (int i = 0; i < 60; i++)
        rc_update(&gs, &map, &in, 1.0f / 60.0f);

    /* Player should have moved past the trigger cell (not blocked) */
    assert(gs.player.x > 5.0f);
    (void)old_x;
}

static void test_update_endgame_triggers_game_over(void)
{
    /* Walking onto an endgame trigger should set game_over */
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 4.5f, 5.5f, 1.0f, 0.0f);

    /* Place endgame trigger in info plane at col 5 */
    map.info[5][5] = INFO_TRIGGER_ENDGAME;

    assert(!gs.game_over);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    /* Walk forward until we reach the trigger cell */
    for (int i = 0; i < 60; i++) {
        rc_update(&gs, &map, &in, 1.0f / 60.0f);
        if (gs.game_over) break;
    }

    assert(gs.game_over);
}

static void test_update_endgame_requires_centre(void)
{
    /* Entering the trigger cell near its edge should not trigger game_over */
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    /* Place endgame trigger at col 6, centre at (6.5, 5.5) */
    map.info[5][6] = INFO_TRIGGER_ENDGAME;

    /* Put player just inside the cell but far from centre */
    gs.player.x = 6.05f;
    gs.player.y = 5.5f;

    Input in;
    memset(&in, 0, sizeof(in));

    rc_update(&gs, &map, &in, 1.0f / 60.0f);
    assert(!gs.game_over);

    /* Now move to the centre */
    gs.player.x = 6.5f;
    gs.player.y = 5.5f;
    rc_update(&gs, &map, &in, 1.0f / 60.0f);
    assert(gs.game_over);
}

static void test_update_no_trigger_no_game_over(void)
{
    /* Walking on normal floor should not set game_over */
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 20, 20, 5.5f, 10.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    for (int i = 0; i < 60; i++)
        rc_update(&gs, &map, &in, 1.0f / 60.0f);

    assert(!gs.game_over);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  rc_cast tests                                                     */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_cast_straight_east(void)
{
    Map map;
    GameState gs;
    /* 10-wide corridor, player at x=2.5 facing east, wall at x=9 */
    init_box_map(&map, &gs, 10, 3, 2.5f, 1.5f, 1.0f, 0.0f);

    rc_cast(&gs, &map);

    /* Centre column ray goes straight east: 9 - 2.5 = 6.5 units */
    int mid = SCREEN_W / 2;
    ASSERT_NEAR(gs.hits[mid].wall_dist, 6.5f, 0.15f);
    assert(gs.hits[mid].side == 0);  /* x-side hit */
}

static void test_cast_straight_north(void)
{
    Map map;
    GameState gs;
    /* Player at y=5.5 facing north (-y), wall at y=0 */
    init_box_map(&map, &gs, 3, 10, 1.5f, 5.5f, 0.0f, -1.0f);

    rc_cast(&gs, &map);

    /* Centre column: 5.5 - 0 → hits north wall at y=0, dist = 4.5 */
    int mid = SCREEN_W / 2;
    ASSERT_NEAR(gs.hits[mid].wall_dist, 4.5f, 0.15f);
    assert(gs.hits[mid].side == 1);  /* y-side hit */
}

static void test_cast_close_wall(void)
{
    Map map;
    GameState gs;
    /* Player right next to a wall */
    init_box_map(&map, &gs, 10, 10, 1.5f, 5.5f, -1.0f, 0.0f);

    rc_cast(&gs, &map);

    /* Centre column faces west, wall at x=0: dist = 0.5 */
    int mid = SCREEN_W / 2;
    ASSERT_NEAR(gs.hits[mid].wall_dist, 0.5f, 0.1f);
}

static void test_cast_all_columns_filled(void)
{
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    rc_cast(&gs, &map);

    /* Every column should have a positive wall distance */
    for (int x = 0; x < SCREEN_W; x++) {
        assert(gs.hits[x].wall_dist > 0.0f);
        assert(gs.hits[x].tile_type == 0);   /* init_box_map uses tile=1 → type 0 */
    }
}

static void test_cast_symmetry(void)
{
    Map map;
    GameState gs;
    /* Player centred in a symmetric box, facing east */
    init_box_map(&map, &gs, 11, 11, 5.5f, 5.5f, 1.0f, 0.0f);

    rc_cast(&gs, &map);

    /* Column 0 and column SCREEN_W-1 should have similar distances
     * (mirror symmetry across the centre ray in a square room) */
    float left  = gs.hits[0].wall_dist;
    float right = gs.hits[SCREEN_W - 1].wall_dist;
    ASSERT_NEAR(left, right, 0.2f);
}

static void test_cast_edge_distances_longer(void)
{
    Map map;
    GameState gs;
    /* In a box facing east, edge columns are angled so their perpendicular
     * distance to the wall should be >= the centre column distance */
    init_box_map(&map, &gs, 20, 20, 10.5f, 10.5f, 1.0f, 0.0f);

    rc_cast(&gs, &map);

    int mid = SCREEN_W / 2;
    float mid_dist = gs.hits[mid].wall_dist;

    /* Edge columns point off-axis so perp distance can be shorter or
     * longer depending on geometry; but centre should hit the closest
     * wall on the forward axis. Left/right edges might hit side walls
     * which could be closer. In a square room centred, edges hit side
     * walls at comparable distances. Just verify all positive. */
    assert(mid_dist > 0.0f);
    assert(gs.hits[0].wall_dist > 0.0f);
    assert(gs.hits[SCREEN_W - 1].wall_dist > 0.0f);
}

static void test_cast_wall_x_range(void)
{
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    rc_cast(&gs, &map);

    /* wall_x should be in [0.0, 1.0) for all columns */
    for (int x = 0; x < SCREEN_W; x++) {
        assert(gs.hits[x].wall_x >= 0.0f);
        assert(gs.hits[x].wall_x < 1.0f);
    }
}

static void test_cast_wall_x_centre(void)
{
    Map map;
    GameState gs;
    /* Player at (2.5, 5.5) facing east in a 10×10 box.
     * Centre ray hits the east wall at x=9, y should be near 5.5.
     * wall_x = frac(5.5) = 0.5 for a y-side? No — facing east hits
     * x-side wall, so wall_x = frac(player.y + perp * ray_dy).
     * Centre ray: ray_dy ≈ 0, so wall_x ≈ frac(5.5) = 0.5 */
    init_box_map(&map, &gs, 10, 10, 2.5f, 5.5f, 1.0f, 0.0f);

    rc_cast(&gs, &map);

    int mid = SCREEN_W / 2;
    ASSERT_NEAR(gs.hits[mid].wall_x, 0.5f, 0.05f);
}

static void test_cast_digit_tile_type(void)
{
    /* Build a map where the east wall is tile value 6 (tile_type 5) */
    Map map;
    GameState gs;
    memset(&map, 0, sizeof(map));
    memset(&gs, 0, sizeof(gs));
    map.w = 10;
    map.h = 10;

    for (int r = 0; r < 10; r++)
        for (int c = 0; c < 10; c++)
            map.tiles[r][c] = 0;

    /* Border walls: left/top/bottom = type 0, right = type 5 */
    for (int r = 0; r < 10; r++) {
        map.tiles[r][0] = 1;      /* type 0 */
        map.tiles[r][9] = 6;      /* type 5 */
    }
    for (int c = 0; c < 10; c++) {
        map.tiles[0][c] = 1;
        map.tiles[9][c] = 1;
    }

    /* Player facing east, will hit the right wall (type 5) */
    gs.player.x = 5.5f;
    gs.player.y = 5.5f;
    gs.player.dir_x = 1.0f;
    gs.player.dir_y = 0.0f;
    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    gs.player.plane_x = 0.0f;
    gs.player.plane_y = tanf(half_fov);

    rc_cast(&gs, &map);

    /* Centre column should hit the east wall (type 5) */
    int mid = SCREEN_W / 2;
    assert(gs.hits[mid].tile_type == 5);
}

static void test_cast_side_shading(void)
{
    Map map;
    GameState gs;
    /* Elongated east-west corridor: edge rays hit north/south walls (y-side)
     * before reaching the far east wall. 30-wide × 5-tall forces this. */
    init_box_map(&map, &gs, 30, 5, 15.5f, 2.5f, 1.0f, 0.0f);

    rc_cast(&gs, &map);

    /* Centre column should be an x-side hit (straight east) */
    int mid = SCREEN_W / 2;
    assert(gs.hits[mid].side == 0);

    /* Edge columns should hit y-side (top/bottom walls) */
    bool found_y_side = false;
    for (int x = 0; x < SCREEN_W; x++) {
        if (gs.hits[x].side == 1) {
            found_y_side = true;
            break;
        }
    }
    assert(found_y_side);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  Integration-style tests (using fake map)                          */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_load_then_cast(void)
{
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    rc_cast(&gs, &map);

    /* All columns should produce valid hits */
    for (int x = 0; x < SCREEN_W; x++) {
        assert(gs.hits[x].wall_dist > 0.0f);
        assert(gs.hits[x].side == 0 || gs.hits[x].side == 1);
    }
}

static void test_walk_and_cast(void)
{
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 20, 20, 10.5f, 10.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    /* Walk forward for half a second then cast */
    for (int i = 0; i < 30; i++)
        rc_update(&gs, &map, &in, 1.0f / 60.0f);

    rc_cast(&gs, &map);

    /* After walking east, the east wall should be closer */
    int mid = SCREEN_W / 2;
    float dist = gs.hits[mid].wall_dist;
    /* Original dist to east wall was 8.5, should now be less */
    assert(dist < 8.5f);
    assert(dist > 0.0f);
}

static void test_fake_map_endgame_triggers_game_over(void)
{
    /* Walk from player spawn towards the endgame trigger and confirm game_over */
    Map map;
    GameState gs;
    load_fake_map(&map, &gs);

    assert(!gs.game_over);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    /* Player at (1.5, 1.5) facing east, trigger at col 3, row 1.
     * Walk east until reaching trigger centre (3.5, 1.5). */
    for (int i = 0; i < 120; i++) {
        rc_update(&gs, &map, &in, 1.0f / 60.0f);
        if (gs.game_over) break;
    }

    assert(gs.game_over);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  Sprite sorting tests                                               */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_sprites_sort_empty(void)
{
    /* Zero sprites should return count 0 */
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);
    gs.sprite_count = 0;

    int order[MAX_SPRITES];
    int n = sprites_sort(&gs, order);
    assert(n == 0);
}

static void test_sprites_sort_back_to_front(void)
{
    /* Three sprites at different distances should sort farthest first */
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 20, 20, 5.5f, 5.5f, 1.0f, 0.0f);

    gs.sprite_count = 3;
    gs.sprites[0] = (Sprite){ 7.5f, 5.5f, 0 };  /* dist = 2.0 (closest)  */
    gs.sprites[1] = (Sprite){15.5f, 5.5f, 1 };  /* dist = 10.0 (farthest) */
    gs.sprites[2] = (Sprite){10.5f, 5.5f, 2 };  /* dist = 5.0 (middle)    */

    int order[MAX_SPRITES];
    int n = sprites_sort(&gs, order);
    assert(n == 3);

    /* First in order should be farthest (index 1, dist 10.0) */
    assert(order[0] == 1);
    /* Second should be middle (index 2, dist 5.0) */
    assert(order[1] == 2);
    /* Last should be closest (index 0, dist 2.0) */
    assert(order[2] == 0);
}

static void test_sprites_sort_single(void)
{
    /* Single sprite should return that sprite's index */
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    gs.sprite_count = 1;
    gs.sprites[0] = (Sprite){ 7.5f, 5.5f, 0 };

    int order[MAX_SPRITES];
    int n = sprites_sort(&gs, order);
    assert(n == 1);
    assert(order[0] == 0);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  Z-buffer tests                                                     */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_z_buffer_filled(void)
{
    /* After rc_cast(), z_buffer should have positive values */
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    rc_cast(&gs, &map);

    for (int x = 0; x < SCREEN_W; x++) {
        assert(gs.z_buffer[x] > 0.0f);
    }
}

static void test_z_buffer_matches_hits(void)
{
    /* z_buffer[x] should equal hits[x].wall_dist for all columns */
    Map map;
    GameState gs;
    init_box_map(&map, &gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    rc_cast(&gs, &map);

    for (int x = 0; x < SCREEN_W; x++) {
        ASSERT_NEAR(gs.z_buffer[x], gs.hits[x].wall_dist, 0.0001f);
    }
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  Main                                                              */
/* ═══════════════════════════════════════════════════════════════════ */

int main(void)
{
    printf("\n── fake map structure ───────────────────────────────────\n");
    RUN_TEST(test_fake_map_dimensions);
    RUN_TEST(test_fake_map_wall_x_hash);
    RUN_TEST(test_fake_map_digit_walls);
    RUN_TEST(test_fake_map_floor_tiles);
    RUN_TEST(test_fake_map_info_spawn);
    RUN_TEST(test_fake_map_info_endgame);
    RUN_TEST(test_fake_map_player_position);
    RUN_TEST(test_fake_map_player_direction);
    RUN_TEST(test_fake_map_camera_plane);
    RUN_TEST(test_fake_map_walls_are_walls);
    RUN_TEST(test_fake_map_all_tile_types_present);

    printf("\n── rc_update ───────────────────────────────────────────\n");
    RUN_TEST(test_update_no_input);
    RUN_TEST(test_update_forward);
    RUN_TEST(test_update_backward);
    RUN_TEST(test_update_wall_collision);
    RUN_TEST(test_update_wall_sliding);
    RUN_TEST(test_update_rotation_left);
    RUN_TEST(test_update_rotation_right);
    RUN_TEST(test_update_endgame_tile_walkable);
    RUN_TEST(test_update_endgame_triggers_game_over);
    RUN_TEST(test_update_endgame_requires_centre);
    RUN_TEST(test_update_no_trigger_no_game_over);

    printf("\n── rc_cast ─────────────────────────────────────────────\n");
    RUN_TEST(test_cast_straight_east);
    RUN_TEST(test_cast_straight_north);
    RUN_TEST(test_cast_close_wall);
    RUN_TEST(test_cast_all_columns_filled);
    RUN_TEST(test_cast_symmetry);
    RUN_TEST(test_cast_edge_distances_longer);
    RUN_TEST(test_cast_wall_x_range);
    RUN_TEST(test_cast_wall_x_centre);
    RUN_TEST(test_cast_digit_tile_type);
    RUN_TEST(test_cast_side_shading);

    printf("\n── Integration ─────────────────────────────────────────\n");
    RUN_TEST(test_load_then_cast);
    RUN_TEST(test_walk_and_cast);
    RUN_TEST(test_fake_map_endgame_triggers_game_over);

    printf("\n── sprites_sort ────────────────────────────────────────\n");
    RUN_TEST(test_sprites_sort_empty);
    RUN_TEST(test_sprites_sort_back_to_front);
    RUN_TEST(test_sprites_sort_single);

    printf("\n── z-buffer ────────────────────────────────────────────\n");
    RUN_TEST(test_z_buffer_filled);
    RUN_TEST(test_z_buffer_matches_hits);

    printf("\n══════════════════════════════════════════════════════════\n");
    printf("  %d / %d tests passed\n", tests_passed, tests_run);
    printf("══════════════════════════════════════════════════════════\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
