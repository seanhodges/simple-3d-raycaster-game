# Simple 3D Raycaster Game

A lightweight 3D-style raycaster engine.

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

## Customisations

- **Colours:** Edit the `COL_*` defines in `raycaster.h` (RGBA8888 format).
- **FOV:** Change `FOV_DEG` in `raycaster.h` (default 60°).
- **Speed:** Tweak `MOVE_SPD` and `ROT_SPD` in `raycaster.c`.
- **Resolution:** Change `SCREEN_W` / `SCREEN_H` in `raycaster.h`.

