/*  main.c  –  entry point & fixed-step game loop
 *  ───────────────────────────────────────────────
 */
#include "raycaster.h"
#include "map_manager.h"
#include "frontend.h"

#include <stdio.h>
#include <string.h>

#define TICK_RATE  60            /* logic updates per second          */
#define DT         (1.0f / TICK_RATE)
#define MAX_FRAME  0.25f         /* max frame time before clamping (s)*/

int main()
{
    const char *map_tiles_path       = "assets/map_tiles.txt";
    const char *map_sprites_path     = "assets/map_sprites.txt";
    const char *map_info_path        = "assets/map_info.txt";
    const char *texture_tiles_path   = "assets/texture_tiles.bmp";
    const char *texture_sprites_path = "assets/texture_sprites.bmp";

    /* Initialise */
    Map map;
    memset(&map, 0, sizeof(map));

    GameState gs;
    memset(&gs, 0, sizeof(gs));

    if (!map_load(&map, &gs.player, map_tiles_path, map_sprites_path, map_info_path)) {
        fprintf(stderr, "main: failed to load map\n");
        return 1;
    }

    /* Initialize frontend and textures */
    if (!frontend_init(texture_tiles_path, texture_sprites_path)) {
        return 1;
    }

    /* Main loop (fixed timestep with accumulator) */
    Input  input;
    memset(&input, 0, sizeof(input));

    double prev  = frontend_get_time();
    float  accum = 0.0f;

    bool running = true;
    while (running) {
        double now   = frontend_get_time();
        float  frame = (float)(now - prev);
        prev = now;

        /* Clamp large spikes (e.g. window drag) */
        if (frame > MAX_FRAME) frame = MAX_FRAME;
        accum += frame;

        /* Poll events once per frame */
        running = frontend_poll_input(&input);

        /* Fixed-step logic updates */
        while (accum >= DT) {
            rc_update(&gs, &map, &input, DT);
            accum -= DT;
        }

        /* Render at display rate */
        rc_cast(&gs, &map);
        frontend_render(&gs);

        /* Player reached the endgame trigger */
        if (gs.game_over) running = false;
    }

    /* End-game screen */
    if (gs.game_over) {
        frontend_render_end_screen();
        bool waiting = true;
        while (waiting) {
            waiting = frontend_poll_end_input();
        }
    }

    frontend_shutdown();
    return 0;
}
