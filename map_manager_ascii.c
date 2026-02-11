/*  map_manager_ascii.c  –  map file parser (tiles + info + sprites planes)
 *  ────────────────────────────────────────────────────────────────────────
 *  Loads ASCII map files into a Map struct and sets the Player spawn.
 *  The tiles file describes wall geometry; the info file describes metadata
 *  such as player spawn (with direction) and endgame triggers; the sprites
 *  file places sprite objects on the map grid.
 *  No SDL headers.  Pure C + math.
 */
#include "map_manager.h"
#include "raycaster.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846f

/* ── Helper: strip trailing newline/carriage-return ───────────────── */

static int strip_line(char *line)
{
    int len = (int)strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
    if (len > 0 && line[len - 1] == '\r') line[--len] = '\0';
    return len;
}

/* ── Helper: set player direction and camera plane from spawn dir ─── */

static void set_player_facing(Player *player, uint16_t spawn_type)
{
    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    float plane_len = tanf(half_fov);

    switch (spawn_type) {
    case INFO_SPAWN_PLAYER_N:
        player->dir_x   =  0.0f;  player->dir_y   = -1.0f;
        player->plane_x =  plane_len; player->plane_y =  0.0f;
        break;
    case INFO_SPAWN_PLAYER_S:
        player->dir_x   =  0.0f;  player->dir_y   =  1.0f;
        player->plane_x = -plane_len; player->plane_y =  0.0f;
        break;
    case INFO_SPAWN_PLAYER_W:
        player->dir_x   = -1.0f;  player->dir_y   =  0.0f;
        player->plane_x =  0.0f;  player->plane_y  = -plane_len;
        break;
    default: /* INFO_SPAWN_PLAYER_E */
        player->dir_x   =  1.0f;  player->dir_y   =  0.0f;
        player->plane_x =  0.0f;  player->plane_y  =  plane_len;
        break;
    }
}

/* ── Map loading ───────────────────────────────────────────────────── */

bool map_load(Map *map, Player *player, const char *tiles_path,
              const char *info_path, const char *sprites_path)
{
    memset(map, 0, sizeof(*map));

    /* ── Pass 1: tiles plane ─────────────────────────────────────── */
    FILE *fp = fopen(tiles_path, "r");
    if (!fp) {
        fprintf(stderr, "map_load: cannot open '%s'\n", tiles_path);
        return false;
    }

    char line[MAP_MAX_W + 2];          /* +newline +NUL */
    int  row = 0;

    while (fgets(line, sizeof(line), fp) && row < MAP_MAX_H) {
        int len = strip_line(line);
        if (len > map->w) map->w = len;

        for (int col = 0; col < len; col++) {
            char c = line[col];
            if (c == 'X' || c == '#') {
                map->tiles[row][col] = 1;              /* wall, type 0 */
            } else if (c >= '0' && c <= '9') {
                map->tiles[row][col] = (c - '0') + 1;  /* wall, type N */
            } else {
                map->tiles[row][col] = TILE_FLOOR;      /* empty */
            }
        }
        row++;
    }
    map->h = row;
    fclose(fp);

    /* ── Pass 2: info plane ──────────────────────────────────────── */
    fp = fopen(info_path, "r");
    if (!fp) {
        fprintf(stderr, "map_load: cannot open '%s'\n", info_path);
        return false;
    }

    row = 0;
    bool     player_set  = false;
    uint16_t spawn_type  = INFO_SPAWN_PLAYER_E;

    while (fgets(line, sizeof(line), fp) && row < MAP_MAX_H) {
        int len = strip_line(line);

        for (int col = 0; col < len; col++) {
            char c = line[col];
            uint16_t val = INFO_EMPTY;

            if (c == '^') {
                val = INFO_SPAWN_PLAYER_N;
            } else if (c == '>') {
                val = INFO_SPAWN_PLAYER_E;
            } else if (c == 'V') {
                val = INFO_SPAWN_PLAYER_S;
            } else if (c == '<') {
                val = INFO_SPAWN_PLAYER_W;
            } else if (c == 'F' || c == 'f') {
                val = INFO_TRIGGER_ENDGAME;
            }

            map->info[row][col] = val;

            if (val >= INFO_SPAWN_PLAYER_N && val <= INFO_SPAWN_PLAYER_W) {
                player->x  = col + 0.5f;
                player->y  = row + 0.5f;
                spawn_type = val;
                player_set = true;
            }
        }
        row++;
    }
    fclose(fp);

    if (!player_set) {
        fprintf(stderr, "map_load: no player spawn found in '%s'\n",
                info_path);
        return false;
    }

    set_player_facing(player, spawn_type);

    /* ── Pass 3: sprites plane (optional) ────────────────────────── */
    if (sprites_path) {
        fp = fopen(sprites_path, "r");
        if (!fp) {
            fprintf(stderr, "map_load: cannot open '%s'\n", sprites_path);
            return false;
        }

        row = 0;
        while (fgets(line, sizeof(line), fp) && row < MAP_MAX_H) {
            int len = strip_line(line);

            for (int col = 0; col < len; col++) {
                char c = line[col];
                if (c >= '1' && c <= '9') {
                    map->sprites[row][col] = (uint16_t)(c - '0');
                    /* value N means texture_id N-1 */
                } else {
                    map->sprites[row][col] = SPRITE_EMPTY;
                }
            }
            row++;
        }
        fclose(fp);
    }

    return true;
}
