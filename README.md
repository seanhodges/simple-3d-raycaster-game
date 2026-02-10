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

**Prerequisites:** CMake >= 3.20, a C11 compiler, and SDL3 development libraries installed and discoverable via `pkg-config` or CMake's `find_package`.

```bash
# Configure and build
cmake -B build
cmake --build build

# Run
./build/raycaster              # uses map.txt (copied to build dir automatically)
./build/raycaster map.txt      # load specific map file

# Run tests
ctest --test-dir build
```

Alternatively, use CMake presets:

```bash
cmake --preset default          # Configure release build
cmake --build --preset default  # Build
ctest --preset default          # Run tests
```

## Customisations

- **Colours:** Edit the `COL_*` defines in `platform_sdl.h` and `textures_sdl.h` (RGBA8888 format).
- **FOV:** Change `FOV_DEG` in `raycaster.h` (default 60°).
- **Speed:** Tweak `MOVE_SPD` and `ROT_SPD` in `raycaster.c`.
- **Resolution:** Change `SCREEN_W` / `SCREEN_H` in `game_globals.h`.
