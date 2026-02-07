# CLAUDE.md

## Project Overview

A lightweight 3D raycasting engine (Wolfenstein 3D-style) written in C with SDL3. Educational/hobbyist project demonstrating DDA raycasting, fixed-timestep game loops, and clean platform abstraction.

## Build & Run

```bash
make              # Build on Linux/macOS (requires gcc, SDL3, pkg-config)
make OS=windows   # Cross-compile for Windows with MinGW
make clean        # Remove build artifacts
./raycaster       # Run the game
```

**Dependency:** SDL3 must be installed and discoverable via `pkg-config`.

## Architecture

Three-layer design with strict separation of concerns:

```
main.c              → Game loop (fixed 60Hz timestep, accumulator pattern)
raycaster.c/.h      → Core logic: map loading, player update, DDA raycasting (no SDL dependency)
platform_sdl.c/.h   → SDL3 rendering, input polling, window management
map.txt             → ASCII map file (X/# = walls, P = player spawn, space = floor)
```

- `raycaster.c` is platform-independent — it never includes SDL headers
- `platform_sdl.c` handles all SDL3-specific code
- `main.c` wires them together in a fixed-timestep loop

## Key Constants (raycaster.h)

| Constant | Value | Purpose |
|----------|-------|---------|
| `SCREEN_W/H` | 800×600 | Window resolution |
| `FOV_DEG` | 60.0 | Field of view in degrees |
| `MAP_MAX_W/H` | 64×64 | Maximum map dimensions |
| `TICK_RATE` | 60 | Physics/input update rate (Hz) |
| `MOVE_SPD` | 3.0 | Movement speed (units/sec) |
| `ROT_SPD` | 2.5 | Rotation speed (rad/sec) |
| `COL_CEIL/FLOOR/WALL` | RGBA hex | Rendering colors |

## Code Conventions

- **C11 standard** (`-std=c11`)
- Strict warnings enabled (`-Wall -Wextra`)
- `rc_` prefix for core raycasting functions (`rc_load_map`, `rc_update`, `rc_cast`)
- `platform_` prefix for SDL abstraction functions
- Descriptive variable names, especially for math operations
- Section comments with box-drawing characters to delineate logical blocks

## Map Format

ASCII text file where:
- `X` or `#` = wall tile
- `P` = player spawn position
- Space = empty floor
- Map must be enclosed by walls on all edges

## Testing

```bash
make test         # Build and run all 20 unit tests (no SDL required)
```

Tests live in `test_raycaster.c` and link only against `raycaster.o`. They cover map loading, player movement/collision, DDA raycasting distances, and integration scenarios. Run from the project root (tests load `map.txt`).

## Controls

WASD or arrow keys for movement and rotation. Close window or press Escape to quit.
