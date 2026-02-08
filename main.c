/*  main.c  –  entry point & fixed-step game loop
 *  ───────────────────────────────────────────────
 */
#include "raycaster.h"
#include "map_manager.h"
#include "platform_sdl.h"
#include "textures_sdl.h"

#include <stdio.h>
#include <string.h>

#define TICK_RATE  60            /* logic updates per second          */
#define DT         (1.0f / TICK_RATE)
#define MAX_FRAME  0.25f         /* max frame time before clamping (s)*/

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    const char *map_path = "map.txt";
    if (argc > 1) map_path = argv[1];

    /* Initialise */
    Map map;
    memset(&map, 0, sizeof(map));

    GameState gs;
    memset(&gs, 0, sizeof(gs));

    if (!map_load(&map, &gs.player, map_path)) {
        fprintf(stderr, "main: failed to load map '%s'\n", map_path);
        return 1;
    }

    if (!platform_init("Raycaster – SDL3")) {
        return 1;
    }

    if (!tm_init("assets/textures.bmp")) {
        platform_shutdown();
        return 1;
    }

    /* Main loop (fixed timestep with accumulator) */
    Input  input;
    memset(&input, 0, sizeof(input));

    double prev  = platform_get_time();
    float  accum = 0.0f;

    bool running = true;
    while (running) {
        double now   = platform_get_time();
        float  frame = (float)(now - prev);
        prev = now;

        /* Clamp large spikes (e.g. window drag) */
        if (frame > MAX_FRAME) frame = MAX_FRAME;
        accum += frame;

        /* Poll events once per frame */
        running = platform_poll_input(&input);

        /* Fixed-step logic updates */
        while (accum >= DT) {
            rc_update(&gs, &map, &input, DT);
            accum -= DT;
        }

        /* Render at display rate */
        rc_cast(&gs, &map);
        platform_render(&gs);

        /* Player reached the exit cell */
        if (gs.game_over) running = false;
    }

    /* End-game screen */
    if (gs.game_over) {
        platform_render_end_screen();
        bool waiting = true;
        while (waiting) {
            waiting = platform_poll_end_input();
        }
    }

    tm_shutdown();
    platform_shutdown();
    return 0;
}
