# CLAUDE.md

## Project Overview

A lightweight 3D-style raycasting engine written in C with SDL3. Educational/hobbyist project demonstrating DDA raycasting, fixed-timestep game loops, and clean platform abstraction.

## Build & Run

```bash
cmake -B build            # Configure (detects SDL3 via pkg-config or CMake)
cmake --build build       # Build
./build/raycaster         # Run the game
```

**Presets** (optional, requires CMake >= 3.20):

```bash
cmake --preset default          # Configure release build in build/
cmake --build --preset default  # Build
cmake --preset debug            # Configure debug build in build-debug/
cmake --build --preset debug    # Build debug
```

**Dependency:** SDL3 must be installed and discoverable via `pkg-config` or CMake's `find_package`.

## Architecture and Conventions

See docs/ARCHITECTURE.md for the project structure, architecture, key design patterns and rationale.
See docs/CONVENTIONS.md for the contribution rules for this codebase.

Always review these files and follow the instructions you find there. If you need to deviate then ask first and ensure you amend the relevant architecture and conventions files to reflect the new design and rules.

## Testing

```bash
cmake --build build             # Build everything (including tests)
ctest --test-dir build          # Run all tests
```

Or with presets:

```bash
ctest --preset default          # Run tests for the default (release) build
```

Tests are split across two files:
- `test_raycaster.c` — links against `raycaster.o` and `map_manager_fake.o`. Uses a hardcoded test map with all tile types. Covers map structure validation, player movement/collision, DDA raycasting, and integration scenarios. No filesystem dependency.
- `test_map_manager_ascii.c` — links against `raycaster.o` and `map_manager_ascii.o`. Tests the real file parser with `map.txt`, `map_info.txt`, and `map_sprites.txt` without assuming specific map contents.

Run from the project root (test_map_manager_ascii needs `map.txt`, `map_info.txt`, and `map_sprites.txt` in the working directory).

The build system copies map files into the build directory automatically so tests can be run from there.

## Controls

WASD or arrow keys for movement and rotation. Close window or press Escape to quit.
