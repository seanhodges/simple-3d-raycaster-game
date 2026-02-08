/*  map.c  –  map file parser
 *  ─────────────────────────
 *  Loads ASCII map files into the GameState.
 *  No SDL headers.  Pure C + math.
 */
#include "map.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846f

/* ── Map loading ───────────────────────────────────────────────────── */

bool map_load(GameState *gs, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "map_load: cannot open '%s'\n", path);
        return false;
    }

    memset(&gs->map, 0, sizeof(gs->map));

    char line[MAP_MAX_W + 2];          /* +newline +NUL */
    int  row = 0;
    bool player_set = false;

    while (fgets(line, sizeof(line), fp) && row < MAP_MAX_H) {
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        if (len > 0 && line[len - 1] == '\r') line[--len] = '\0';

        if (len > gs->map.w) gs->map.w = len;

        for (int col = 0; col < len; col++) {
            char c = line[col];
            if (c == 'X' || c == '#') {
                gs->map.cells[row][col] = 1;          /* wall, type 0 */
            } else if (c >= '0' && c <= '9') {
                gs->map.cells[row][col] = (c - '0') + 1; /* wall, type N */
            } else if (c == 'P' || c == 'p') {
                gs->map.cells[row][col] = CELL_FLOOR;  /* floor */
                gs->player.x = col + 0.5f;
                gs->player.y = row + 0.5f;
                player_set = true;
            } else if (c == 'F' || c == 'f') {
                gs->map.cells[row][col] = CELL_EXIT;   /* exit trigger */
            } else {
                gs->map.cells[row][col] = CELL_FLOOR;  /* empty */
            }
        }
        row++;
    }
    gs->map.h = row;
    fclose(fp);

    if (!player_set) {
        fprintf(stderr, "map_load: no player start ('P') found\n");
        return false;
    }

    /* Default facing direction: east, with FOV-derived camera plane */
    float half_fov = (FOV_DEG * 0.5f) * (PI / 180.0f);
    gs->player.dir_x   =  1.0f;
    gs->player.dir_y   =  0.0f;
    gs->player.plane_x  =  0.0f;
    gs->player.plane_y  =  tanf(half_fov);

    return true;
}
