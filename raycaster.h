#ifndef RAYCASTER_H
#define RAYCASTER_H

#include <stdbool.h>

/* ── Display constants ─────────────────────────────────────────────── */
#define SCREEN_W  800
#define SCREEN_H  600
#define FOV_DEG   60.0f

/* ── Map limits ────────────────────────────────────────────────────── */
#define MAP_MAX_W 64
#define MAP_MAX_H 64

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

/* ── Per-column ray result ─────────────────────────────────────────── */
typedef struct RayHit {
    float wall_dist;     /* perpendicular distance to wall          */
    float wall_x;        /* where on the wall face the ray hit 0-1  */
    int   side;          /* 0 = x-side hit, 1 = y-side hit          */
    int   wall_type;     /* texture index (0 .. TEX_COUNT-1)        */
} RayHit;

/* ── Player state ──────────────────────────────────────────────────── */
typedef struct Player {
    float x, y;          /* position in map units                    */
    float dir_x, dir_y;  /* direction vector                         */
    float plane_x, plane_y; /* camera plane (perpendicular to dir)   */
} Player;

/* ── World map ─────────────────────────────────────────────────────── */
typedef struct Map {
    int  cells[MAP_MAX_H][MAP_MAX_W];
    int  w, h;
} Map;

/* ── Complete game state ───────────────────────────────────────────── */
typedef struct GameState {
    Map     map;
    Player  player;
    RayHit  hits[SCREEN_W];   /* filled every frame by rc_cast()     */
    bool    game_over;        /* true when player reaches exit cell   */
} GameState;

/* ── Input flags (set by platform layer) ───────────────────────────── */
typedef struct Input {
    bool forward, back, strafe_left, strafe_right;
    bool turn_left, turn_right;
} Input;

/* ── Public API ────────────────────────────────────────────────────── */

/**  Load map from file. Returns false on failure. */
bool rc_load_map(GameState *gs, const char *path);

/**  Update player position/rotation from input.  dt in seconds. */
void rc_update(GameState *gs, const Input *in, float dt);

/**  Cast all rays and fill gs->hits[]. */
void rc_cast(GameState *gs);

#endif /* RAYCASTER_H */
