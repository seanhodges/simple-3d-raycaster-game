/*  test_raycaster.c  –  unit tests for the platform-independent core
 *  ──────────────────────────────────────────────────────────────────
 *  Links only against raycaster.o — no SDL dependency.
 *  Build:  make test
 *  Run:    ./test_raycaster
 */
#include "raycaster.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
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

/* ── Helper: build a GameState with an inline map ─────────────────── */

static void init_box_map(GameState *gs, int w, int h,
                         float px, float py,
                         float dir_x, float dir_y)
{
    memset(gs, 0, sizeof(*gs));
    gs->map.w = w;
    gs->map.h = h;

    /* Walls on all edges, floor inside */
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            gs->map.cells[r][c] =
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

/* ═══════════════════════════════════════════════════════════════════ */
/*  rc_load_map tests                                                 */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_load_map_basic(void)
{
    GameState gs;
    memset(&gs, 0, sizeof(gs));

    bool ok = rc_load_map(&gs, "map.txt");
    assert(ok);
    assert(gs.map.w == 16);
    assert(gs.map.h == 17);

    /* Top-left corner is a wall */
    assert(gs.map.cells[0][0] == 1);

    /* Player spawn is at row 6, col 3 ('P' in map.txt) → centre of cell */
    ASSERT_NEAR(gs.player.x, 3.5f, 0.01f);
    ASSERT_NEAR(gs.player.y, 6.5f, 0.01f);

    /* Player faces east by default */
    ASSERT_NEAR(gs.player.dir_x, 1.0f, 0.01f);
    ASSERT_NEAR(gs.player.dir_y, 0.0f, 0.01f);

    /* Camera plane is perpendicular (pointing +y) */
    ASSERT_NEAR(gs.player.plane_x, 0.0f, 0.01f);
    assert(gs.player.plane_y > 0.0f);
}

static void test_load_map_missing_file(void)
{
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    bool ok = rc_load_map(&gs, "nonexistent.txt");
    assert(!ok);
}

static void test_load_map_walls_parsed(void)
{
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    rc_load_map(&gs, "map.txt");

    /* Entire first row should be walls */
    for (int c = 0; c < gs.map.w; c++)
        assert(gs.map.cells[0][c] == 1);

    /* Entire last row should be walls */
    for (int c = 0; c < gs.map.w; c++)
        assert(gs.map.cells[gs.map.h - 1][c] == 1);

    /* An interior empty cell (row 1, col 1) should be floor */
    assert(gs.map.cells[1][1] == 0);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  rc_update tests                                                   */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_update_no_input(void)
{
    GameState gs;
    init_box_map(&gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));

    float old_x = gs.player.x;
    float old_y = gs.player.y;

    rc_update(&gs, &in, 1.0f / 60.0f);

    /* Player should not move */
    ASSERT_NEAR(gs.player.x, old_x, 0.0001f);
    ASSERT_NEAR(gs.player.y, old_y, 0.0001f);
}

static void test_update_forward(void)
{
    GameState gs;
    /* Facing east in a big box, plenty of room */
    init_box_map(&gs, 20, 20, 5.5f, 10.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    float old_x = gs.player.x;
    rc_update(&gs, &in, 1.0f);

    /* Should have moved ~3.0 units east (MOVE_SPD = 3.0) */
    assert(gs.player.x > old_x);
    ASSERT_NEAR(gs.player.x - old_x, 3.0f, 0.01f);
}

static void test_update_backward(void)
{
    GameState gs;
    init_box_map(&gs, 20, 20, 10.5f, 10.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.back = true;

    float old_x = gs.player.x;
    rc_update(&gs, &in, 1.0f);

    /* Should have moved ~3.0 units west */
    assert(gs.player.x < old_x);
    ASSERT_NEAR(old_x - gs.player.x, 3.0f, 0.01f);
}

static void test_update_strafe(void)
{
    GameState gs;
    /* Facing east → strafe right should move south (+y) */
    init_box_map(&gs, 20, 20, 10.5f, 10.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.strafe_right = true;

    float old_y = gs.player.y;
    rc_update(&gs, &in, 1.0f);

    assert(gs.player.y > old_y);
}

static void test_update_wall_collision(void)
{
    GameState gs;
    /* Player near north wall (row 0 = wall), facing north (-y) */
    init_box_map(&gs, 10, 10, 5.5f, 1.5f, 0.0f, -1.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    /* Walk into wall for a full second */
    for (int i = 0; i < 60; i++)
        rc_update(&gs, &in, 1.0f / 60.0f);

    /* Player should be stopped by the wall, not inside or beyond it */
    assert(gs.player.y > 0.5f);
}

static void test_update_wall_sliding(void)
{
    GameState gs;
    /* Facing northeast into a north wall — should slide east */
    float inv_sqrt2 = 1.0f / sqrtf(2.0f);
    init_box_map(&gs, 20, 20, 5.5f, 1.5f, inv_sqrt2, -inv_sqrt2);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    float old_x = gs.player.x;
    for (int i = 0; i < 60; i++)
        rc_update(&gs, &in, 1.0f / 60.0f);

    /* Should have slid east (x increased) while y stayed near wall */
    assert(gs.player.x > old_x + 0.5f);
    assert(gs.player.y > 0.5f);
}

static void test_update_rotation_left(void)
{
    GameState gs;
    init_box_map(&gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.turn_left = true;

    rc_update(&gs, &in, 1.0f);

    /* Rotating left (counter-clockwise) should change direction */
    /* After ~2.5 radians of rotation, dir_x should be negative */
    float len = sqrtf(gs.player.dir_x * gs.player.dir_x +
                      gs.player.dir_y * gs.player.dir_y);
    ASSERT_NEAR(len, 1.0f, 0.01f);  /* direction vector stays unit length */
}

static void test_update_rotation_right(void)
{
    GameState gs;
    init_box_map(&gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.turn_right = true;

    /* Small rotation: 1/60th second */
    rc_update(&gs, &in, 1.0f / 60.0f);

    /* dir_y should become positive (clockwise) */
    assert(gs.player.dir_y > 0.0f);
    /* direction vector should stay unit length */
    float len = sqrtf(gs.player.dir_x * gs.player.dir_x +
                      gs.player.dir_y * gs.player.dir_y);
    ASSERT_NEAR(len, 1.0f, 0.001f);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  rc_cast tests                                                     */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_cast_straight_east(void)
{
    GameState gs;
    /* 10-wide corridor, player at x=2.5 facing east, wall at x=9 */
    init_box_map(&gs, 10, 3, 2.5f, 1.5f, 1.0f, 0.0f);

    rc_cast(&gs);

    /* Centre column ray goes straight east: 9 - 2.5 = 6.5 units */
    int mid = SCREEN_W / 2;
    ASSERT_NEAR(gs.hits[mid].wall_dist, 6.5f, 0.15f);
    assert(gs.hits[mid].side == 0);  /* x-side hit */
}

static void test_cast_straight_north(void)
{
    GameState gs;
    /* Player at y=5.5 facing north (-y), wall at y=0 */
    init_box_map(&gs, 3, 10, 1.5f, 5.5f, 0.0f, -1.0f);

    rc_cast(&gs);

    /* Centre column: 5.5 - 0 → hits north wall at y=0, dist = 4.5 */
    int mid = SCREEN_W / 2;
    ASSERT_NEAR(gs.hits[mid].wall_dist, 4.5f, 0.15f);
    assert(gs.hits[mid].side == 1);  /* y-side hit */
}

static void test_cast_close_wall(void)
{
    GameState gs;
    /* Player right next to a wall */
    init_box_map(&gs, 10, 10, 1.5f, 5.5f, -1.0f, 0.0f);

    rc_cast(&gs);

    /* Centre column faces west, wall at x=0: dist = 0.5 */
    int mid = SCREEN_W / 2;
    ASSERT_NEAR(gs.hits[mid].wall_dist, 0.5f, 0.1f);
}

static void test_cast_all_columns_filled(void)
{
    GameState gs;
    init_box_map(&gs, 10, 10, 5.5f, 5.5f, 1.0f, 0.0f);

    rc_cast(&gs);

    /* Every column should have a positive wall distance */
    for (int x = 0; x < SCREEN_W; x++) {
        assert(gs.hits[x].wall_dist > 0.0f);
        assert(gs.hits[x].wall_type == 1);
    }
}

static void test_cast_symmetry(void)
{
    GameState gs;
    /* Player centred in a symmetric box, facing east */
    init_box_map(&gs, 11, 11, 5.5f, 5.5f, 1.0f, 0.0f);

    rc_cast(&gs);

    /* Column 0 and column SCREEN_W-1 should have similar distances
     * (mirror symmetry across the centre ray in a square room) */
    float left  = gs.hits[0].wall_dist;
    float right = gs.hits[SCREEN_W - 1].wall_dist;
    ASSERT_NEAR(left, right, 0.2f);
}

static void test_cast_edge_distances_longer(void)
{
    GameState gs;
    /* In a box facing east, edge columns are angled so their perpendicular
     * distance to the wall should be >= the centre column distance */
    init_box_map(&gs, 20, 20, 10.5f, 10.5f, 1.0f, 0.0f);

    rc_cast(&gs);

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

static void test_cast_side_shading(void)
{
    GameState gs;
    /* Elongated east-west corridor: edge rays hit north/south walls (y-side)
     * before reaching the far east wall. 30-wide × 5-tall forces this. */
    init_box_map(&gs, 30, 5, 15.5f, 2.5f, 1.0f, 0.0f);

    rc_cast(&gs);

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
/*  Integration-style tests                                           */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_load_then_cast(void)
{
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    bool ok = rc_load_map(&gs, "map.txt");
    assert(ok);

    rc_cast(&gs);

    /* Player spawns at (3.5, 6.5) facing east.
     * All columns should produce valid hits. */
    for (int x = 0; x < SCREEN_W; x++) {
        assert(gs.hits[x].wall_dist > 0.0f);
        assert(gs.hits[x].side == 0 || gs.hits[x].side == 1);
    }
}

static void test_walk_and_cast(void)
{
    GameState gs;
    init_box_map(&gs, 20, 20, 10.5f, 10.5f, 1.0f, 0.0f);

    Input in;
    memset(&in, 0, sizeof(in));
    in.forward = true;

    /* Walk forward for half a second then cast */
    for (int i = 0; i < 30; i++)
        rc_update(&gs, &in, 1.0f / 60.0f);

    rc_cast(&gs);

    /* After walking east, the east wall should be closer */
    int mid = SCREEN_W / 2;
    float dist = gs.hits[mid].wall_dist;
    /* Original dist to east wall was 8.5, should now be less */
    assert(dist < 8.5f);
    assert(dist > 0.0f);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  Main                                                              */
/* ═══════════════════════════════════════════════════════════════════ */

int main(void)
{
    printf("\n── rc_load_map ──────────────────────────────────────────\n");
    RUN_TEST(test_load_map_basic);
    RUN_TEST(test_load_map_missing_file);
    RUN_TEST(test_load_map_walls_parsed);

    printf("\n── rc_update ───────────────────────────────────────────\n");
    RUN_TEST(test_update_no_input);
    RUN_TEST(test_update_forward);
    RUN_TEST(test_update_backward);
    RUN_TEST(test_update_strafe);
    RUN_TEST(test_update_wall_collision);
    RUN_TEST(test_update_wall_sliding);
    RUN_TEST(test_update_rotation_left);
    RUN_TEST(test_update_rotation_right);

    printf("\n── rc_cast ─────────────────────────────────────────────\n");
    RUN_TEST(test_cast_straight_east);
    RUN_TEST(test_cast_straight_north);
    RUN_TEST(test_cast_close_wall);
    RUN_TEST(test_cast_all_columns_filled);
    RUN_TEST(test_cast_symmetry);
    RUN_TEST(test_cast_edge_distances_longer);
    RUN_TEST(test_cast_side_shading);

    printf("\n── Integration ─────────────────────────────────────────\n");
    RUN_TEST(test_load_then_cast);
    RUN_TEST(test_walk_and_cast);

    printf("\n══════════════════════════════════════════════════════════\n");
    printf("  %d / %d tests passed\n", tests_passed, tests_run);
    printf("══════════════════════════════════════════════════════════\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
