/*  test_map_manager_ascii.c  –  tests for the real map file loader
 *  ──────────────────────────────────────────────────────────────────
 *  Links against raycaster.o and map_manager_ascii.o — no SDL dependency.
 *  Tests that map_load() can successfully read map.txt without
 *  making assumptions about the map's specific contents.
 *  Run from the project root (needs map.txt in the working directory).
 *  Build:  make test
 *  Run:    ./test_map_manager_ascii
 */
#include "raycaster.h"
#include "map_manager.h"

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

/* ═══════════════════════════════════════════════════════════════════ */
/*  map_load tests (real file loader)                                  */
/* ═══════════════════════════════════════════════════════════════════ */

static void test_load_map_succeeds(void)
{
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));

    bool ok = map_load(&map, &player, "map.txt");
    assert(ok);
}

static void test_load_map_has_dimensions(void)
{
    /* The loaded map should have non-zero width and height */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "map.txt");

    assert(map.w > 0);
    assert(map.h > 0);
    assert(map.w <= MAP_MAX_W);
    assert(map.h <= MAP_MAX_H);
}

static void test_load_map_has_player(void)
{
    /* Player position should be within the map bounds */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "map.txt");

    assert(player.x > 0.0f);
    assert(player.y > 0.0f);
    assert(player.x < (float)map.w);
    assert(player.y < (float)map.h);
}

static void test_load_map_player_at_cell_centre(void)
{
    /* Player should be placed at the centre of a cell (fractional part = 0.5) */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "map.txt");

    float frac_x = player.x - (int)player.x;
    float frac_y = player.y - (int)player.y;
    ASSERT_NEAR(frac_x, 0.5f, 0.01f);
    ASSERT_NEAR(frac_y, 0.5f, 0.01f);
}

static void test_load_map_player_on_floor(void)
{
    /* The cell where the player spawns should be floor (CELL_FLOOR) */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "map.txt");

    int px = (int)player.x;
    int py = (int)player.y;
    assert(map.cells[py][px] == CELL_FLOOR);
}

static void test_load_map_player_direction(void)
{
    /* Player should face east with unit-length direction vector */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "map.txt");

    ASSERT_NEAR(player.dir_x, 1.0f, 0.01f);
    ASSERT_NEAR(player.dir_y, 0.0f, 0.01f);

    float len = sqrtf(player.dir_x * player.dir_x +
                      player.dir_y * player.dir_y);
    ASSERT_NEAR(len, 1.0f, 0.01f);
}

static void test_load_map_camera_plane(void)
{
    /* Camera plane should be perpendicular to direction and derived from FOV */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "map.txt");

    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    float expected_plane = tanf(half_fov);

    ASSERT_NEAR(player.plane_x, 0.0f, 0.01f);
    ASSERT_NEAR(player.plane_y, expected_plane, 0.01f);
}

static void test_load_map_missing_file(void)
{
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    bool ok = map_load(&map, &player, "nonexistent.txt");
    assert(!ok);
}

static void test_load_map_cells_in_range(void)
{
    /* All cells should be valid values: floor (0), wall (>0), or exit (-1) */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "map.txt");

    for (int r = 0; r < map.h; r++) {
        for (int c = 0; c < map.w; c++) {
            int cell = map.cells[r][c];
            assert(cell >= CELL_EXIT);   /* minimum valid value */
            assert(cell <= 10);          /* digit '9' → cell = 10 max */
        }
    }
}

static void test_load_map_game_state_unaffected(void)
{
    /* map_load should not touch GameState — only Map and Player */
    GameState gs;
    Map map;
    memset(&gs, 0, sizeof(gs));
    memset(&map, 0, sizeof(map));
    map_load(&map, &gs.player, "map.txt");

    assert(!gs.game_over);
}

/* ═══════════════════════════════════════════════════════════════════ */
/*  Main                                                              */
/* ═══════════════════════════════════════════════════════════════════ */

int main(void)
{
    printf("\n── map_load (real file loader) ─────────────────────────\n");
    RUN_TEST(test_load_map_succeeds);
    RUN_TEST(test_load_map_has_dimensions);
    RUN_TEST(test_load_map_has_player);
    RUN_TEST(test_load_map_player_at_cell_centre);
    RUN_TEST(test_load_map_player_on_floor);
    RUN_TEST(test_load_map_player_direction);
    RUN_TEST(test_load_map_camera_plane);
    RUN_TEST(test_load_map_missing_file);
    RUN_TEST(test_load_map_cells_in_range);
    RUN_TEST(test_load_map_game_state_unaffected);

    printf("\n══════════════════════════════════════════════════════════\n");
    printf("  %d / %d tests passed\n", tests_passed, tests_run);
    printf("══════════════════════════════════════════════════════════\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
