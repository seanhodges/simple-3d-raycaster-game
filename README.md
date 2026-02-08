# Simple 3D Maze Game

A lightweight raycaster engine featuring a simple 3D-style maze game.

## Controls

| Key             | Action        |
|-----------------|---------------|
| W / ↑           | Move forward  |
| S / ↓           | Move backward |
| A / ←           | Turn left     |
| D / →           | Turn right    |
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

## Customisations

- **Colours:** Edit the `COL_*` defines in `raycaster.h` (RGBA8888 format).
- **FOV:** Change `FOV_DEG` in `raycaster.h` (default 60°).
- **Speed:** Tweak `MOVE_SPD` and `ROT_SPD` in `raycaster.c`.
- **Resolution:** Change `SCREEN_W` / `SCREEN_H` in `raycaster.h`.

