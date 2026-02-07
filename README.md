# Portable C Raycaster (SDL3)

A lightweight, Wolfenstein 3D-style raycaster engine in pure C using SDL3.

## Architecture

```
┌─────────────┐     ┌───────────────┐     ┌──────────────┐
│   main.c    │────▶│  raycaster.c  │────▶│ platform_sdl │
│  game loop  │     │  DDA core     │     │  SDL3 render │
│  (entry pt) │     │  (no SDL!)    │     │  input poll  │
└─────────────┘     └───────────────┘     └──────────────┘
```

- **raycaster.h / .c** — Map loading, player movement, DDA raycasting. Fills a `RayHit` buffer. Contains zero platform code.
- **platform_sdl.h / .c** — SDL3 window, keyboard input (WASD + arrows), renders wall strips from the hit buffer.
- **main.c** — Fixed-timestep game loop tying it all together.

## Controls

| Key             | Action        |
|-----------------|---------------|
| W / ↑           | Move forward  |
| S / ↓           | Move backward |
| A               | Strafe left   |
| D               | Strafe right  |
| ← →             | Turn          |
| Escape          | Quit          |

## Building

**Prerequisites:** SDL3 development libraries installed and discoverable via `pkg-config`.

```bash
# Linux / macOS
make

# Windows (MinGW)
make OS=windows

# Run
./raycaster              # uses map.txt in current directory
./raycaster mymap.txt    # custom map
```

## Map Format (`map.txt`)

```
XXXXXXX
X P   X
X XXX X
X     X
XXXXXXX
```

- `X` or `#` — Wall
- `P` — Player start (exactly one required)
- Space — Empty floor

## Customisation

- **Colours:** Edit the `COL_*` defines in `raycaster.h` (RGBA8888 format).
- **FOV:** Change `FOV_DEG` in `raycaster.h` (default 60°).
- **Speed:** Tweak `MOVE_SPD` and `ROT_SPD` in `raycaster.c`.
- **Resolution:** Change `SCREEN_W` / `SCREEN_H` in `raycaster.h`.

## Extending

The `wall_type` field in `RayHit` is ready for multi-colour walls — just assign different integers in the map loader and add a colour lookup in `platform_sdl.c`.
