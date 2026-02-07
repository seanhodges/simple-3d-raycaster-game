# Simple 3D Raycaster Game

A lightweight 3D-style raycaster engine.

## Architecture

- **raycaster.c** — Map loading, player movement, DDA raycasting. Fills a `RayHit` buffer.
- **platform_sdl.c** — frontend for SDL3: window, keyboard input, renders wall strips from the hit buffer.
- **main.c** — Main game loop tying it all together.

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
./raycaster map.txt      # load specific map file
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

