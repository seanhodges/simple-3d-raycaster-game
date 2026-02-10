#ifndef GAME_GLOBALS_H
#define GAME_GLOBALS_H

#include <stdbool.h>
#include <stdint.h>

/* ── Display constants (needed by GameState) ──────────────────────── */
#define SCREEN_W  800
#define SCREEN_H  600

/* ── Map limits ────────────────────────────────────────────────────── */
#define MAP_MAX_W 64
#define MAP_MAX_H 64

/* ── Sprite limits ────────────────────────────────────────────────── */
#define MAX_SPRITES 64           /* maximum sprites in the map         */

/* ── Per-column ray result ─────────────────────────────────────────── */
typedef struct RayHit {
    float    wall_dist;     /* perpendicular distance to wall          */
    float    wall_x;        /* where on the wall face the ray hit 0-1  */
    int      side;          /* 0 = x-side hit, 1 = y-side hit          */
    uint16_t tile_type;     /* texture index (0 .. TEX_COUNT-1)        */
} RayHit;

/* ── Player state ──────────────────────────────────────────────────── */
typedef struct Player {
    float x, y;          /* position in map units                    */
    float dir_x, dir_y;  /* direction vector                         */
    float plane_x, plane_y; /* camera plane (perpendicular to dir)   */
} Player;

/* ── Sprite instance ──────────────────────────────────────────────── */
typedef struct Sprite {
    float    x, y;        /* position in map units                    */
    uint16_t texture_id;  /* index into sprite texture atlas          */
} Sprite;

/* ── World map ─────────────────────────────────────────────────────── */
typedef struct Map {
    uint16_t  tiles[MAP_MAX_H][MAP_MAX_W]; /* geometry: 0=floor, >0=wall */
    uint16_t  info[MAP_MAX_H][MAP_MAX_W];  /* metadata: spawn, triggers  */
    int       w, h;
} Map;

/* ── Game state (excludes map — managed separately) ───────────────── */
typedef struct GameState {
    Player  player;
    RayHit  hits[SCREEN_W];      /* filled every frame by rc_cast()    */
    float   z_buffer[SCREEN_W];  /* 1D depth buffer for sprite clipping*/
    Sprite  sprites[MAX_SPRITES];/* sprite instances in the world      */
    int     sprite_count;        /* number of active sprites           */
    bool    game_over;           /* true when player reaches endgame   */
} GameState;

/* ── Input flags (set by platform layer) ───────────────────────────── */
typedef struct Input {
    bool forward, back;
    bool turn_left, turn_right;
} Input;

#endif /* GAME_GLOBALS_H */
