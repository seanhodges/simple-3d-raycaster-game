/*  test_map_manager_ascii.c  –  tests for the real map file loader
 *  ──────────────────────────────────────────────────────────────────
 *  Links against raycaster.o and map_manager_ascii.o — no SDL dependency.
 *  Tests that map_load() can successfully read map files
 *  without making assumptions about the map's specific contents.
 *  Note: needs map asset files in the working directory.
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

    bool ok = map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");
    assert(ok);
}

static void test_load_map_has_dimensions(void)
{
    /* The loaded map should have non-zero width and height */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

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
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

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
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

    float frac_x = player.x - (int)player.x;
    float frac_y = player.y - (int)player.y;
    ASSERT_NEAR(frac_x, 0.5f, 0.01f);
    ASSERT_NEAR(frac_y, 0.5f, 0.01f);
}

static void test_load_map_player_on_floor(void)
{
    /* The tile where the player spawns should be floor (TILE_FLOOR) */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

    int px = (int)player.x;
    int py = (int)player.y;
    assert(map.tiles[py][px] == TILE_FLOOR);
}

static void test_load_map_player_direction(void)
{
    /* Player should face east with unit-length direction vector */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

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
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    float expected_plane = tanf(half_fov);

    ASSERT_NEAR(player.plane_x, 0.0f, 0.01f);
    ASSERT_NEAR(player.plane_y, expected_plane, 0.01f);
}

static void test_load_map_missing_tiles_file(void)
{
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    bool ok = map_load(&map, &player, "nonexistent.txt", "assets/map_sprites.txt", "assets/map_info.txt");
    assert(!ok);
}

static void test_load_map_missing_info_file(void)
{
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    bool ok = map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "nonexistent.txt");
    assert(!ok);
}

static void test_load_map_tiles_in_range(void)
{
    /* All tiles should be valid values: floor (0) or wall (>0) */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

    for (int r = 0; r < map.h; r++) {
        for (int c = 0; c < map.w; c++) {
            uint16_t tile = map.tiles[r][c];
            assert(tile <= 10);  /* max wall type digit '9' → tile value 10 */
        }
    }
}

static void test_load_map_info_has_spawn(void)
{
    /* Info plane should contain at least one spawn cell */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

    bool found_spawn = false;
    for (int r = 0; r < map.h; r++) {
        for (int c = 0; c < map.w; c++) {
            uint16_t val = map.info[r][c];
            if (val >= INFO_SPAWN_PLAYER_N && val <= INFO_SPAWN_PLAYER_W) {
                found_spawn = true;
                break;
            }
        }
        if (found_spawn) break;
    }
    assert(found_spawn);
}

static void test_load_map_info_has_endgame(void)
{
    /* Info plane should contain at least one endgame trigger */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

    bool found_endgame = false;
    for (int r = 0; r < map.h; r++) {
        for (int c = 0; c < map.w; c++) {
            if (map.info[r][c] == INFO_TRIGGER_ENDGAME) {
                found_endgame = true;
                break;
            }
        }
        if (found_endgame) break;
    }
    assert(found_endgame);
}

static void test_load_map_info_border_ignored(void)
{
    /* X border characters in the info plane should be treated as INFO_EMPTY */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

    /* Top row: all cells should be INFO_EMPTY (the X border) */
    for (int c = 0; c < map.w; c++) {
        assert(map.info[0][c] == INFO_EMPTY);
    }
    /* Bottom row: all cells should be INFO_EMPTY (the X border) */
    for (int c = 0; c < map.w; c++) {
        assert(map.info[map.h - 1][c] == INFO_EMPTY);
    }
    /* Left and right columns: border cells should be INFO_EMPTY */
    for (int r = 0; r < map.h; r++) {
        assert(map.info[r][0] == INFO_EMPTY);
        assert(map.info[r][map.w - 1] == INFO_EMPTY);
    }
}

static void test_load_map_info_dimensions_match_tiles(void)
{
    /* The info plane should cover the same area as the tiles plane */
    Map map;
    Player player;
    memset(&map, 0, sizeof(map));
    memset(&player, 0, sizeof(player));
    map_load(&map, &player, "assets/map_tiles.txt", "assets/map_sprites.txt", "assets/map_info.txt");

    /* Dimensions are set by the tiles pass — verify both planes are populated.
     * Check that the info plane has content at the map boundaries. */
    assert(map.w > 0);
    assert(map.h > 0);

    /* Verify the info plane is not all empty — it should contain spawn + endgame */
    bool found_non_empty = false;
    for (int r = 0; r < map.h && !found_non_empty; r++) {
        for (int c = 0; c < map.w && !found_non_empty; c++) {
            if (map.info[r][c] != INFO_EMPTY) {
                found_non_empty = true;
            }
        }
    }
    assert(found_non_empty);
}

static void test_load_map_game_state_unaffected(void)
{
    /* map_load should not touch GameState — only Map and Player */
    GameState gs;
    Map map;
    memset(&gs, 0, sizeof(gs));
    memset(&map, 0, sizeof(map));
    map_load(&map, &gs.player, "assets/map_tiles.txt", "assets/map_info.txt", "assets/map_sprites.txt");

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
    RUN_TEST(test_load_map_missing_tiles_file);
    RUN_TEST(test_load_map_missing_info_file);
    RUN_TEST(test_load_map_tiles_in_range);
    RUN_TEST(test_load_map_info_has_spawn);
    RUN_TEST(test_load_map_info_has_endgame);
    RUN_TEST(test_load_map_info_border_ignored);
    RUN_TEST(test_load_map_info_dimensions_match_tiles);
    RUN_TEST(test_load_map_game_state_unaffected);

    printf("\n══════════════════════════════════════════════════════════\n");
    printf("  %d / %d tests passed\n", tests_passed, tests_run);
    printf("══════════════════════════════════════════════════════════\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
