# CLAUDE.md

## Project Overview

A lightweight 3D-style raycasting engine written in C with SDL3. Educational/hobbyist project demonstrating DDA raycasting, fixed-timestep game loops, and clean platform abstraction.

## Build & Run

```bash
make              # Build on Linux/macOS (requires gcc, SDL3, pkg-config)
make OS=windows   # Cross-compile for Windows with MinGW
make clean        # Remove build artifacts
./raycaster       # Run the game
```

**Dependency:** SDL3 must be installed and discoverable via `pkg-config`.

## Architecture and Conventions

See docs/ARCHITECTURE.md for the project structure, architecture, key design patterns and rationale.
See docs/CONVENTIONS.md for the contribution rules for this codebase.

Always review these files and follow the instructions you find there. If you need to deviate then ask first and ensure you amend the relevant architecture and conventions files to reflect the new design and rules.

## Testing

```bash
make test         # Build and run all unit tests (no SDL required)
```

Tests are split across two files:
- `test_raycaster.c` — links against `raycaster.o` and `map_manager_fake.o`. Uses a hardcoded test map with all tile types. Covers map structure validation, player movement/collision, DDA raycasting, and integration scenarios. No filesystem dependency.
- `test_map_manager_ascii.c` — links against `raycaster.o` and `map_manager_ascii.o`. Tests the real file parser with `map.txt` and `map_info.txt` without assuming specific map contents.

Run from the project root (test_map_manager_ascii needs `map.txt` and `map_info.txt` in the working directory).

## Controls

WASD or arrow keys for movement and rotation. Close window or press Escape to quit.
