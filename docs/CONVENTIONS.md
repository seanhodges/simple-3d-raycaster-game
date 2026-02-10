# Coding Conventions Guide

> The rules this codebase follows — and should continue to follow. If you're contributing, read this first.

## Table of Contents

- [Naming Conventions](#naming-conventions)
- [File Organization](#file-organization)
- [Formatting & Style](#formatting--style)
- [Error Handling Strategy](#error-handling-strategy)
- [State Management Rules](#state-management-rules)
- [Memory Management](#memory-management)
- [Testing Patterns](#testing-patterns)
- [Collision & Physics Conventions](#collision--physics-conventions)
- [Inconsistencies & Recommended Standards](#inconsistencies--recommended-standards)

---

## Naming Conventions

### Function Prefixes

Functions are namespaced by their layer using prefixes. This is the C equivalent of modules.

| Prefix | Layer | Examples |
|---|---|---|
| `rc_` | Core raycasting engine | `rc_update`, `rc_cast` |
| `map_` | Map file loader | `map_load` |
| `platform_` | SDL3 platform abstraction | `platform_init`, `platform_shutdown`, `platform_poll_input`, `platform_render` |
| `tm_` | Texture manager | `tm_init`, `tm_shutdown`, `tm_get_pixel` |
| *(none)* | `main()` and static helpers | `main`, `is_wall` (static in raycaster.c) |

**Rule:** Every public function must carry its layer prefix. Static (file-private) helper functions do not need a prefix.

### Variables

- **Descriptive names** for domain concepts: `wall_dist`, `draw_start`, `line_h`, `cam_x`
- **Short names** only for well-understood math: `c`/`s` for cos/sin, `dx`/`dy` for deltas, `p` for player pointer, `m` for map pointer
- **Struct members** use `snake_case`: `dir_x`, `plane_y`, `wall_type`

### Constants

- **Preprocessor defines** in `UPPER_SNAKE_CASE`: `SCREEN_W`, `FOV_DEG`, `MAP_MAX_H`, `COL_WALL`
- **Tile plane constants** prefixed with `TILE_`: `TILE_FLOOR`
- **Info plane constants** prefixed with `INFO_`: `INFO_EMPTY`, `INFO_SPAWN_PLAYER_N/E/S/W`, `INFO_TRIGGER_ENDGAME`
- **Color constants** prefixed with `COL_`: `COL_CEIL`, `COL_FLOOR`, `COL_WALL`, `COL_WALL_SHADE`
- **Physics constants** use descriptive suffixes: `MOVE_SPD`, `ROT_SPD`, `TICK_RATE`

### Types

- **Typedef'd structs** in `PascalCase`: `GameState`, `Player`, `Map`, `RayHit`, `Input`
- No `_t` suffix (avoids conflict with POSIX reserved names)

---

## File Organization

### Header Files

Every `.c` file has a corresponding `.h` file (except `main.c`). Shared data types live in `game_globals.h` and are re-exported via `raycaster.h`. Headers follow this structure:

```c
#ifndef RAYCASTER_H          // Include guard: FILENAME_H
#define RAYCASTER_H

#include <stdbool.h>          // Standard library includes first

/* Section comments */
// Type definitions
// Function declarations with doc comments

#endif /* RAYCASTER_H */      // Closing comment repeats guard name
```

### Source Files

Each `.c` file opens with a comment block identifying its purpose:

```c
/*  filename.c  –  one-line description
 *  ───────────────────────────────────
 *  Optional longer explanation.
 */
```

Include order:
1. Own header (`#include "raycaster.h"`)
2. Other project headers
3. System/library headers (`<math.h>`, `<SDL3/SDL.h>`)

### Section Comments

Logical blocks within a file are separated by box-drawing section headers:

```c
/* ── Section Name ──────────────────────────────────────────────────── */
```

These use the `──` (U+2500) box-drawing character, not ASCII dashes. The line extends to column 72. This is a consistent visual convention throughout the codebase — **do not use plain `//` or `/* --- */` style separators**.

---

## Formatting & Style

### Language Standard

- **C11** (`-std=c11`), enforced by the Makefile
- `bool` via `<stdbool.h>` (not `int` for boolean values)
- No C++ features, no compiler extensions (GNU or otherwise)

### Indentation & Braces

- **4-space indentation** (no tabs in source files)
- **K&R brace style** for functions:
  ```c
  bool map_load(Map *map, Player *player, const char *tiles_path,
                const char *info_path)
  {
      // body
  }
  ```
- **Egyptian braces** for control flow:
  ```c
  if (condition) {
      // body
  } else {
      // body
  }
  ```
- **Single-line bodies** allowed without braces for simple if/return:
  ```c
  if (mx < 0 || my < 0 || mx >= m->w || my >= m->h) return true;
  ```

### Alignment

Deliberate column-alignment is used for related declarations:

```c
float dir_x, dir_y;          /* direction vector                  */
float plane_x, plane_y;      /* camera plane (perpendicular)      */
```

```c
in->forward      = ks[SDL_SCANCODE_W] || ks[SDL_SCANCODE_UP];
in->back         = ks[SDL_SCANCODE_S] || ks[SDL_SCANCODE_DOWN];
```

This visual alignment is intentional and should be maintained when adding new fields.

### Comments

- **Inline comments** on the right, column-aligned: `/* description */`
- **Doc comments** above function declarations in headers: `/** Brief description. */`
- **No Doxygen or Javadoc** — the codebase uses plain C comments
- Comment the *why*, not the *what* — the code is straightforward enough that most lines are self-documenting

### Compiler Warnings

```makefile
CFLAGS = -std=c11 -Wall -Wextra -O2
```

**Rule: Zero warnings.** The codebase compiles cleanly with `-Wall -Wextra`. Unused parameters are silenced explicitly:

```c
(void)argc; (void)argv;
```

---

## Error Handling Strategy

The codebase uses a **fail-fast, propagate-boolean** strategy. There are no exceptions (it's C), no error codes, and no `errno` inspection.

### Pattern: Return `bool`, Print to `stderr`

```c
bool map_load(Map *map, Player *player, const char *tiles_path,
              const char *info_path)
{
    FILE *fp = fopen(tiles_path, "r");
    if (!fp) {
        fprintf(stderr, "map_load: cannot open '%s'\n", tiles_path);
        return false;
    }
    // ...
}
```

Every error message includes the **function name** and the **offending value** for debuggability.

### Caller Responsibility

```c
if (!map_load(&map, &gs.player, tiles_path, info_path)) {
    fprintf(stderr, "main: failed to load map\n");
    return 1;
}

if (!platform_init("Raycaster – SDL3")) {
    return 1;    // platform_init already printed the SDL error
}
```

**Rule:** The callee prints the specific diagnostic. The caller decides whether to continue or abort.

### What's NOT Handled

- No recovery from SDL failures (renderer creation, etc.) — the program exits
- No graceful degradation — if the map is missing, the game doesn't start
- No runtime assertions or `assert()` usage

This is appropriate for a small game. For a larger project, consider adding `assert()` for invariants in debug builds.

---

## State Management Rules

### Single Source of Truth

`GameState` holds player state and the ray buffer. `Map` is a standalone struct. Both live on `main()`'s stack and are passed by pointer to functions that need them.

```mermaid
graph LR
    MAIN["main() stack"]
    GS["GameState"]
    MAIN --> GS

    RC["rc_update / rc_cast"]
    PLAT["platform_render"]
    INPUT["platform_poll_input"]

    GS -->|"&gs (read/write)"| RC
    GS -->|"&gs (read-only)"| PLAT
    INPUT -->|"&input (write)"| PLAT
```

**Rules:**
1. `GameState` is never copied — always passed by pointer
2. Only `rc_update` and `rc_cast` mutate `GameState`
3. `platform_render` receives `const GameState *` — it is read-only
4. `Input` flows one way: platform writes it, core reads it

### No Global Mutable State (in the core)

`raycaster.c` has zero global or file-scoped variables. All state is in the `GameState` struct.

`platform_sdl.c` has two file-scoped statics (`window`, `renderer`). This is the one exception, and it's encapsulated behind the platform API — no other file can access them.

### Initialization

All structs are zero-initialized before use:

```c
GameState gs;
memset(&gs, 0, sizeof(gs));

Input input;
memset(&input, 0, sizeof(input));
```

**Rule:** Never leave a struct partially initialized. Use `memset` to zero the entire struct, then set specific fields.

---

## Memory Management

**There is no dynamic allocation.** Zero calls to `malloc`, `calloc`, `realloc`, or `free` in the entire codebase.

All data lives in:
- `GameState` and `Map` on `main()`'s stack (Map includes the 64×64 grid; GameState includes the 800-element ray buffer)
- File-scoped statics in `platform_sdl.c` (SDL handles)
- Local variables in functions

This is a deliberate design choice:
- No memory leaks possible
- No use-after-free possible
- No null pointer dereference from allocation failure
- Deterministic memory footprint (~20 KB total)

**Rule:** If you add a feature, prefer fixed-size arrays or stack allocation. Only introduce `malloc` if the data size is truly dynamic and large.

---

## Testing Patterns

### Running Tests

```bash
make test        # Build and run all tests (no SDL required)
```

Tests live in two files:
- `test_raycaster.c` — links against `raycaster.o` and `map_manager_fake.o` (a hardcoded test map). Filesystem-independent.
- `test_map_manager_ascii.c` — links against `raycaster.o` and `map_manager_ascii.o` (the real file loader). Requires `map.txt` and `map_info.txt` in the working directory.

Both have no SDL dependency. Run from the project root with `make test`.

### Test Architecture

The tests use a **minimal custom harness** — no external test framework. The harness provides:
- `RUN_TEST(fn)` — runs a void function, prints pass/fail, increments counters
- `ASSERT_NEAR(a, b, eps)` — floating-point comparison with epsilon tolerance
- Standard `assert()` for boolean conditions

Exit code is `0` on all-pass, `1` on any failure — compatible with CI.

### Test Fixture Pattern

Tests build game state programmatically using `init_box_map()` — a helper that creates a walled box of any size with the player at a specified position and direction. The camera plane is auto-derived from `FOV_DEG`:

```c
static void init_box_map(Map *map, GameState *gs, int w, int h,
                         float px, float py,
                         float dir_x, float dir_y)
```

This avoids depending on `map.txt` for most tests. Only `test_load_map_*` and `test_load_then_cast` use the real map file.

### Writing New Tests

1. Add a `static void test_your_name(void)` function in the appropriate section
2. Use `init_box_map()` for geometry setup, or `rc_load_map()` for file-based tests
3. Use `ASSERT_NEAR()` for distance comparisons (raycasting has float imprecision)
4. Add `RUN_TEST(test_your_name);` to `main()`
5. For tests that need a known map, use the fake map module (`map_manager_fake.c`) via `map_load()` in `test_raycaster.c`
6. For tests that exercise the real file parser, add them to `test_map_manager_ascii.c` — avoid assuming specific map contents
7. Run `make test` — zero warnings required, zero failures expected

---

## Collision & Physics Conventions

### Axis-Independent Collision

Movement is tested per-axis with a named margin constant:

```c
#define COL_MARGIN 0.15f   /* wall collision margin (map units) */

if (!is_wall(m, p->x + dx + (dx > 0 ? COL_MARGIN : -COL_MARGIN), p->y))
    p->x += dx;
if (!is_wall(m, p->x, p->y + dy + (dy > 0 ? COL_MARGIN : -COL_MARGIN)))
    p->y += dy;
```

**Rule:** Always test X and Y independently. This enables "wall sliding" — the player moves along a wall instead of stopping dead.

Note that the X-axis check happens first, and the Y-axis check uses the **updated** `p->x`. This ordering is correct — it prevents corner-cutting through diagonal walls.

### Coordinate System

- Map coordinates: `(col, row)` where `(0,0)` is the top-left
- Player position: floating-point, center of a cell is `(col + 0.5, row + 0.5)`
- Map tiles indexed as `tiles[row][col]` (row-major, Y-first)
- Map info indexed as `info[row][col]` (same layout as tiles)

### Units

| Quantity | Unit | Reference |
|---|---|---|
| Position | Map units (1 unit = 1 cell) | `player.x`, `player.y` |
| Distance | Map units | `RayHit.wall_dist` |
| Speed | Map units / second | `MOVE_SPD = 3.0` |
| Rotation | Radians / second | `ROT_SPD = 2.5` |
| Time step | Seconds | `DT = 1/60` |
| FOV | Degrees (converted internally) | `FOV_DEG = 60.0` |

---

## Recommended Standards

These are the codified standards this project follows. They were established by identifying the strongest patterns in the codebase and applying them consistently.

### 1. Platform Isolation in `main.c`

`main.c` must not include any platform-specific headers (e.g., `<SDL3/SDL.h>`). All platform interaction goes through the `platform_` API. The timer is accessed via `platform_get_time()`, which returns a `double` in seconds:

```c
// main.c — no SDL includes
double prev = platform_get_time();
// ...
double now = platform_get_time();
float frame = (float)(now - prev);
```

If someone ports the engine to a different backend, only `platform_*.c` files should change.

### 2. `const` Correctness

Functions that only read a struct take `const *`. Functions that write take non-const `*`. This is enforced consistently:

```c
void rc_update(GameState *gs, const Map *map, const Input *in, float dt);  // writes gs, reads map & in
void platform_render(const GameState *gs);                  // reads gs only
bool platform_poll_input(Input *in);                        // writes in
```

**Rule:** Always be explicit about mutability. Never cast away `const`.

### 3. Named Constants Over Magic Numbers

All tunable values must be named `#define` constants with a comment explaining their purpose and unit:

```c
#define COL_MARGIN 0.15f   /* wall collision margin (map units) */
#define MAX_FRAME  0.25f   /* max frame time before clamping (s)*/
#define MOVE_SPD   3.0f    /* map-units / second                */
```

No bare numeric literals in logic code — if a number controls behavior, it gets a name.

### 4. Private Constants Scope

Constants used by only one file are `#define`d in that `.c` file, not in a header. Only constants needed by multiple files belong in headers:

| Location | Examples | Why |
|---|---|---|
| `game_globals.h` | `SCREEN_W`, `MAP_MAX_W`, `MAP_MAX_H` | Constants needed by shared type definitions |
| `raycaster.h` | `SCREEN_H`, `FOV_DEG`, `COL_WALL`, `TEX_SIZE`, `TEX_COUNT`, `TILE_FLOOR`, `INFO_*` | Used by both core and platform |
| `raycaster.c` | `PI`, `MOVE_SPD`, `ROT_SPD`, `COL_MARGIN` | Implementation detail of the core |
| `main.c` | `TICK_RATE`, `DT`, `MAX_FRAME` | Implementation detail of the game loop |

This minimizes header pollution and keeps the public API surface small.

### 5. Error Message Format

All `fprintf(stderr, ...)` messages follow a consistent, grep-able format:

```
<function_name>: <what failed> [: <detail>]
```

Examples:
```c
fprintf(stderr, "map_load: cannot open '%s'\n", path);
fprintf(stderr, "platform_init: SDL_Init failed: %s\n", SDL_GetError());
fprintf(stderr, "main: failed to load map '%s'\n", map_path);
```

Every error message must include the originating function name so developers can locate the source with `grep`.

### 6. Tagged Typedefs

All struct typedefs include a matching tag name to support forward declarations:

```c
typedef struct Player {
    float x, y;
    float dir_x, dir_y;
    float plane_x, plane_y;
} Player;
```

This allows files to forward-declare `struct Player;` without including the full header — essential as the codebase grows and circular dependencies may emerge.
