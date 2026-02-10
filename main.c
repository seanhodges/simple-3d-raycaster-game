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

    const char *tiles_path = "map.txt";
    const char *info_path  = "map_info.txt";
    if (argc > 1) tiles_path = argv[1];
    if (argc > 2) info_path  = argv[2];

    /* Initialise */
    Map map;
    memset(&map, 0, sizeof(map));

    GameState gs;
    memset(&gs, 0, sizeof(gs));

    if (!map_load(&map, &gs.player, tiles_path, info_path)) {
        fprintf(stderr, "main: failed to load map\n");
        return 1;
    }

    if (!platform_init("Raycaster – SDL3")) {
        return 1;
    }

    if (!tm_init("assets/textures.bmp")) {
        platform_shutdown();
        return 1;
    }

    if (!tm_init_sprites("assets/sprites.bmp")) {
        tm_shutdown();
        platform_shutdown();
        return 1;
    }

    /* Place example sprites at different map positions */
    gs.sprite_count = 3;
    gs.sprites[0] = (Sprite){ 5.5f,  3.5f, 0 };  /* red diamond, near start  */
    gs.sprites[1] = (Sprite){ 7.5f,  7.5f, 1 };  /* green circle, mid-map    */
    gs.sprites[2] = (Sprite){ 3.5f, 13.5f, 2 };  /* blue column, far area    */

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

        /* Player reached the endgame trigger */
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
