# Architecture Guide

> A deep-dive into how this raycaster works, written for junior engineers who want to understand every layer — from the game loop to the pixels on screen.

## Table of Contents

- [High-Level Overview](#high-level-overview)
- [Tech Stack & Rationale](#tech-stack--rationale)
- [The Three-Layer Architecture](#the-three-layer-architecture)
- [Data Flow: One Frame, Start to Finish](#data-flow-one-frame-start-to-finish)
- [Core Algorithm: DDA Raycasting](#core-algorithm-dda-raycasting)
- [The Fixed-Timestep Game Loop](#the-fixed-timestep-game-loop)
- [Platform Abstraction Layer](#platform-abstraction-layer)
- [Map System](#map-system)
- [Key Data Structures](#key-data-structures)

---

## High-Level Overview

This project is a **3D-style raycasting engine** — it fakes 3D by casting one ray per screen column from the player's viewpoint, measuring the distance to the nearest wall, and drawing a vertical strip whose height is inversely proportional to that distance. The result is a convincing first-person perspective rendered entirely in 2D math.

The design pattern is the **Layered Architecture** (sometimes called Ports & Adapters / Hexagonal at its conceptual root): the core game logic knows nothing about SDL, windows, or pixels. A thin platform layer translates between the engine and the OS.

```mermaid
graph TB
    subgraph "Layer 3 — Orchestrator"
        MAIN["main.c<br/>Game loop, timestep control"]
    end

    subgraph "Layer 1 — Core Engine"
        RC["raycaster.c / .h<br/>Map, Player, DDA, Physics"]
        SPR["sprites.c / .h<br/>Sprite sorting"]
    end

    subgraph "Layer 2 — Platform"
        PLAT["platform_sdl.c / .h<br/>SDL3 window, input, rendering<br/>(includes sprite renderer)"]
        TEX["textures_sdl.c / .h<br/>Texture atlas manager<br/>(walls + sprites)"]
    end

    subgraph "External"
        SDL3["SDL3 Library"]
        MAP["map.txt + map_info.txt + map_sprites.txt"]
        BMP["assets/textures.bmp<br/>assets/sprites.bmp"]
    end

    MAIN --> RC
    MAIN --> SPR
    MAIN --> PLAT
    MAIN --> TEX
    PLAT --> SDL3
    PLAT --> TEX
    PLAT --> SPR
    TEX --> SDL3
    TEX --> BMP
    subgraph "Layer 1b — Map Loader"
        MAPMOD["map_manager_ascii.c / map_manager.h<br/>ASCII map parser"]
    end

    MAIN --> MAPMOD
    MAPMOD --> MAP

    style RC fill:#1a3a5c,stroke:#4a9eff,color:#fff
    style PLAT fill:#3a1a3a,stroke:#9a4aff,color:#fff
    style TEX fill:#3a1a3a,stroke:#9a4aff,color:#fff
    style MAIN fill:#1a3a1a,stroke:#4aff4a,color:#fff
```

---

## Tech Stack & Rationale

| Technology | Role | Why This Choice |
|---|---|---|
| **C (C11)** | Language | Raycasting is a fundamentally low-level algorithm — pointer arithmetic, fixed-size arrays, tight loops. C11 is simple and gives direct control with zero runtime overhead. |
| **SDL3** | The default frontend for the game; handling windowing, input, rendering | The de facto standard for cross-platform multimedia in C. SDL provides a 2D renderer, keyboard state polling, and high-resolution timers. SDL3 (not SDL2) is used here for its modernized API. |
| **CMake** | Build system | Industry-standard cross-platform build system generator. Handles compiler detection, dependency discovery (via `pkg-config` or native `find_package`), and test integration through CTest. Presets provide reproducible build configurations. |
| **`-Wall -Wextra`** | Compiler discipline | Maximum warnings catch bugs at compile time. Release builds use `-O2` to ensure the tight raycasting loop runs at full speed even on modest hardware. |

---

## The Three-Layer Architecture

### Shared Types (`game_globals.h`)

**Design pattern: Shared Type Definitions** — a standalone header that defines the core data types (`Map`, `GameState`, `Input`, `Player`, `RayHit`) and the constants they depend on (`SCREEN_W`, `MAP_MAX_W`, `MAP_MAX_H`). Included by `raycaster.h` and available to all layers. No function declarations — types only.

### Layer 1: Core Engine (`raycaster.c` / `raycaster.h`)

**Design pattern: Pure Computation Module** — this file has zero platform dependencies. It includes `math.h` and `stdio.h`. That's it. No SDL or non-standard headers. `raycaster.h` re-exports `game_globals.h` so consumers get all types and constants from a single include.

Responsibilities:
- **Player physics** (`rc_update`) — movement, rotation, collision detection
- **Raycasting** (`rc_cast`) — DDA algorithm fills the `RayHit` buffer and collects visible sprites from the map grid during traversal

### Map Loader (`map_manager_ascii.c` / `map_manager.h`)

**Design pattern: File Parser Module** — separated from the core engine to allow test-time substitution with a fake implementation.

Responsibilities:
- **Map loading** (`map_load`) — parse three ASCII map files (tiles plane + info plane + sprites plane) into a standalone `Map` struct and initialise the `Player` position, direction, and camera

For unit tests, `map_manager_fake.c` provides an alternative `map_load` implementation that returns a hardcoded map with all tile types and info entries, removing filesystem dependencies from the core test suite.

This separation is an important architectural decision in the project. It means:
- The core engine can be unit-tested without a display
- The renderer can be swapped (OpenGL, Vulkan, terminal) without touching game logic
- The math is portable to any platform with a C compiler

### Layer 2: Platform Abstraction (`platform_sdl.c` / `platform_sdl.h`)

**Design pattern: Adapter** — translates between the engine's data structures and SDL3's API.

Responsibilities:
- Window creation and teardown
- Keyboard state polling (continuous, not event-based — critical for smooth movement)
- Rendering: converts the `RayHit[]` buffer into textured vertical strips using a streaming framebuffer

The platform layer **reads from** the core but never writes to it, except through the `Input` struct. Data flows in one direction.

### Texture Manager (`textures_sdl.c` / `textures_sdl.h`)

**Design pattern: Asset Manager** — loads and provides pixel-level access to wall textures.

Responsibilities:
- Load a horizontal texture atlas (BMP) containing `TEX_COUNT` (10) square textures of `TEX_SIZE` (64) pixels each
- Provide `tm_get_tile_pixel(tile_type, tex_x, tex_y)` for colour sampling
- Fall back to a solid wall colour (`COL_WALL`) if the BMP file is missing

The texture manager uses SDL for BMP loading but stores pixel data in a flat array for fast access. The renderer calls `tm_get_tile_pixel()` to sample textures — it never accesses the texture data directly. This keeps the renderer decoupled from file-loading logic.

The texture manager also handles sprite textures via `tm_init_sprites()` and `tm_get_sprite_pixel()`. Sprite textures use colour-key transparency: pixels matching `#980088` (magenta, `SPRITE_ALPHA_KEY`) are treated as transparent and skipped during rendering.

### Sprite System (`sprites.c` / `sprites.h`)

**Design pattern: Pure Computation Module** — like `raycaster.c`, this file has zero platform dependencies.

Responsibilities:
- Sort visible sprites back-to-front by perpendicular distance (insertion sort)
- Provide `sprites_sort()` which operates in-place on a pre-collected `Sprite` array

Sprite collection happens during DDA traversal in `rc_cast()`. As each ray steps through floor cells, it checks the map's sprite plane for non-empty cells and adds unseen sprites (with their perpendicular distance to the camera plane) to `GameState.visible_sprites[]`. A per-frame visited bitmap prevents duplicates across the 800 rays. After all rays complete, `sprites_sort()` orders them back-to-front.

The sprite rendering itself lives in `platform_sdl.c` (since it needs framebuffer access). The rendering pass occurs **after** walls are drawn and uses:
- **Billboarding**: sprites are rendered as flat planes perpendicular to the view vector using the inverse camera matrix
- **Z-buffering**: a 1D depth buffer (`GameState.z_buffer[]`) filled during raycasting prevents sprites from showing through walls
- **Colour-key transparency**: pixels matching `SPRITE_ALPHA_KEY` are skipped

Sprite placement is defined in the map's sprites plane (`Map.sprites[][]`), loaded from `map_sprites.txt`. Only sprites whose cells are traversed by at least one ray are collected, avoiding a full grid scan.

### Layer 3: Orchestrator (`main.c`)

**Design pattern: Mediator / Composition Root** — owns the game loop, wires the other two layers together, and manages time.

Responsibilities:
- Initialize subsystems in order (map → platform)
- Run the fixed-timestep loop
- Shut down cleanly on exit

```mermaid
graph LR
    subgraph main.c
        INIT["Init"] --> LOOP["Game Loop"] --> SHUT["Shutdown"]
    end

    subgraph platform_sdl.c
        LOOP -->|"poll"| PLAT["User Input"]
        LOOP -->|"render"| PLAT["Draw Viewport"]
    end
    subgraph raycaster.c
        LOOP -->|"update"| RC["Refresh Game State"]
        LOOP -->|"cast"| RC["Calculate Viewport"]
    end
```

---

## Data Flow: One Frame, Start to Finish

Here's what happens every single frame, in order:

```mermaid
sequenceDiagram
    participant Main as main.c
    participant Plat as platform_sdl.c
    participant Core as raycaster.c

    Note over Main: Measure elapsed time (SDL_GetPerformanceCounter)
    Note over Main: Clamp frame spike to 250ms max

    Main->>Plat: platform_poll_input(&input)
    Note over Plat: SDL_PollEvent (quit/escape check)
    Note over Plat: SDL_GetKeyboardState → fill Input struct
    Plat-->>Main: returns true (keep running) or false (quit)

    loop While accumulator >= DT (1/60s)
        Main->>Core: rc_update(&gs, &map, &input, DT)
        Note over Core: Apply rotation (2D rotation matrix)
        Note over Core: Apply translation (with collision)
        Note over Main: accumulator -= DT
    end

    Main->>Core: rc_cast(&gs, &map)
    Note over Core: For each of 800 screen columns:<br/>Cast ray via DDA, store distance in gs.hits[]<br/>Collect visible sprites from traversed cells
    Note over Core: Sort visible sprites back-to-front

    Main->>Plat: platform_render(&gs)
    Note over Plat: Clear screen (ceiling color)
    Note over Plat: Draw floor rectangle (bottom half)
    Note over Plat: For each column: draw wall strip from hits[]
    Note over Plat: Render pre-sorted visible sprites
    Note over Plat: Draw debug overlay (player coords)
    Note over Plat: SDL_RenderPresent
```

Key insight: **physics and rendering are decoupled**. Physics runs at a fixed 60Hz regardless of display refresh rate. Rendering happens once per frame at whatever rate the display allows. This is the accumulator pattern.

---

## Core Algorithm: DDA Raycasting

DDA (Digital Differential Analyzer) is the heart of the engine. For each of the 800 screen columns, a ray is cast from the player's position into the map:

### Step-by-Step

1. **Compute ray direction** — Combine the player's direction vector with the camera plane, scaled by the column's position on screen (`cam_x` ranges from -1.0 to +1.0):
   ```
   ray_dir = player.dir + player.plane * cam_x
   ```

2. **Initialize DDA** — Calculate `delta_dist` (how far the ray must travel to cross one full grid cell) and `side_dist` (distance from the player to the first grid boundary) for each axis.

3. **Step through the grid** — Advance to whichever axis boundary is closer. Repeat until a wall cell is hit or the ray exits the map.

4. **Calculate perpendicular distance** — Not the Euclidean distance (that would cause fish-eye distortion), but the perpendicular distance to the camera plane:
   ```
   perp_dist = (hit_cell - player_pos + (1 - step) * 0.5) / ray_dir
   ```

5. **Store the result** — Distance, which side was hit (x or y), and the wall type go into `gs->hits[column]`.

```mermaid
graph TD
    A["For each screen column x = 0..799"] --> B["Compute ray direction<br/>from camera plane"]
    B --> C["Init DDA: delta distances<br/>and first side distances"]
    C --> D{"side_dist_x < side_dist_y?"}
    D -->|Yes| E["Step in X direction<br/>side = 0"]
    D -->|No| F["Step in Y direction<br/>side = 1"]
    E --> G{"Hit wall or<br/>out of bounds?"}
    F --> G
    G -->|No| D
    G -->|Yes| H["Calculate perpendicular distance<br/>(avoids fish-eye)"]
    H --> I["Store in hits[x]:<br/>distance, side, tile_type"]
```

### Why Perpendicular Distance?

If you used the actual Euclidean distance from player to wall, walls at the edges of the screen would appear curved (the "fish-eye" effect). By projecting onto the camera plane, walls render as straight lines — which is what the human eye expects.

### Side Shading

When a ray hits a wall on its Y-side (north/south face), the renderer uses `COL_WALL_SHADE` (slightly darker). This simple trick gives a strong sense of 3D depth with zero computational cost.

---

## The Fixed-Timestep Game Loop

The game loop in `main.c` implements the **fixed-timestep with accumulator** pattern. This is the industry-standard approach for decoupling physics from rendering.

```
while running:
    frame_time = time_since_last_frame()
    clamp(frame_time, max=0.25s)        ← prevents "spiral of death"
    accumulator += frame_time

    poll_input()

    while accumulator >= DT:            ← DT = 1/60s
        update_physics(DT)
        accumulator -= DT

    cast_rays()
    render()
```

### Why Not Just Use Frame Delta?

If you pass the raw frame delta to physics (`update(frame_dt)`), behavior changes at different frame rates: a 30fps machine gets different physics than a 144fps machine. Floating-point precision issues accumulate differently. Collisions can be missed at low frame rates.

The fixed-timestep approach guarantees that **physics always steps at exactly 1/60th of a second**, regardless of rendering speed. The accumulator handles the mismatch:
- If rendering is fast (144fps): multiple render frames may occur between physics steps
- If rendering is slow (30fps): multiple physics steps run per render frame

### The 250ms Clamp

```c
if (frame > 0.25f) frame = 0.25f;
```

This prevents the "spiral of death": if the game freezes (e.g., during a window drag), `frame` could become very large, causing dozens of physics updates in one burst. The clamp limits catch-up to at most ~15 ticks, keeping the game responsive after a stall.

---

## Platform Abstraction Layer

### Input Model: Continuous State, Not Events

The input system uses `SDL_GetKeyboardState()` — a snapshot of which keys are currently held — rather than processing individual key-down/key-up events. This is a deliberate design choice:

- **Events** are good for discrete actions (menu selection, chat input)
- **Continuous state** is good for movement (hold W to walk forward)

Events are still polled for quit/escape detection, but movement reads from the state array every frame.

### Rendering Pipeline

The renderer uses a streaming framebuffer texture for pixel-level textured wall rendering:

1. **Lock** the streaming framebuffer texture for direct pixel writes
2. **Fill** the framebuffer with ceiling (top half) and floor (bottom half) colours
3. **Draw walls** — for each of 800 columns:
   - Calculate strip height from `hits[x].wall_dist`
   - Derive `tex_x` from `hits[x].wall_x` (fractional hit position on the wall face)
   - For each pixel in the strip, compute `tex_y` from the vertical position
   - Sample the colour from `tm_get_pixel(tile_type, tex_x, tex_y)`
   - Apply y-side darkening for depth cue (halve RGB components)
4. **Unlock** and blit the framebuffer to the renderer
5. **Debug overlay** — player coordinates rendered as text (top-left corner)
6. **Present** — flip the back buffer to screen

### State Management

Platform state (`SDL_Window*`, `SDL_Renderer*`) is stored in **file-scoped static variables** — essentially a singleton. This is appropriate because:
- There is exactly one window and one renderer
- The platform layer is not reentrant
- The API surface is small (4 functions)

---

## Map System

Maps consist of three ASCII text files parsed at startup — one for geometry (tiles), one for metadata (info), and one for sprite placement (sprites). All files share the same grid dimensions.

### Tiles Plane (`map.txt`)

Defines wall geometry and floor layout:

```
XXXXXXXXXXXXXXXX
X              X
X  XXXXX  XXX  X
X  X          XX
X  X  XXXXXX  X
X     X    X   X
X     X    X   X
X     X        X
XXXX  XXXX  XXXX
...
```

| Character | Meaning | Tile Value |
|---|---|---|
| `X` or `#` | Wall (texture 0) | `1` |
| `0`–`9` | Wall (texture N) | `N + 1` |
| ` ` (space) | Empty floor | `0` (`TILE_FLOOR`) |

Tile values encode wall presence and texture type: `0` = floor (walkable), `> 0` = wall with `tile_type = tile - 1`. The `is_wall()` function simply checks `tile > TILE_FLOOR`.

### Info Plane (`map_info.txt`)

Defines metadata — player spawn position/direction and triggers. The file uses an `X` border to visually frame the grid and aid readability:

```
XXXXXXXXXXXXXXXX
X   F          X
X              X
X              X
X              X
X              X
X   >          X
X              X
...
XXXXXXXXXXXXXXXX
```

| Character | Meaning | Info Value |
|---|---|---|
| ` ` (space) | Empty (no metadata) | `0` (`INFO_EMPTY`) |
| `X` | Border / decoration (ignored) | `0` (`INFO_EMPTY`) |
| `^` | Player spawn, facing north | `1` (`INFO_SPAWN_PLAYER_N`) |
| `>` | Player spawn, facing east | `2` (`INFO_SPAWN_PLAYER_E`) |
| `V` | Player spawn, facing south | `3` (`INFO_SPAWN_PLAYER_S`) |
| `<` | Player spawn, facing west | `4` (`INFO_SPAWN_PLAYER_W`) |
| `F` or `f` | Endgame trigger | `5` (`INFO_TRIGGER_ENDGAME`) |

Any unrecognised character (including the `X` border) is treated as `INFO_EMPTY`. The `X` border is a visual convention that mirrors the wall border in `map.txt`, making the two files easy to compare side-by-side.

The player spawns at the **center** of the spawn cell (`col + 0.5, row + 0.5`) facing the direction indicated by the arrow character. The camera plane is derived from `FOV_DEG`, perpendicular to the facing direction.

When the player reaches the centre of an `INFO_TRIGGER_ENDGAME` cell, `game_over` is set to `true` and the game displays a congratulations screen.

### Sprites Plane (`map_sprites.txt`)

Places sprite objects on the map grid. Each non-empty cell spawns a billboarded sprite at the centre of that cell during rendering:

```
................
.....1..........
................
................
................
................
................
..........2.....
................
................
................
................
..3.............
................
................
................
................
```

| Character | Meaning | Grid Value |
|---|---|---|
| `.` or ` ` | No sprite | `0` (`SPRITE_EMPTY`) |
| `1`–`9` | Sprite (texture N-1) | `N` |

Grid values store `texture_id + 1`, so a grid value of `1` means texture index 0. At render time, `sprites_collect_and_sort()` scans the grid, builds `Sprite` instances at cell centres (`col + 0.5, row + 0.5`), and sorts them back-to-front by distance to the player.

The sprites file is optional — if not provided, the sprites plane stays empty (all `SPRITE_EMPTY`).

### Constraints

- Maximum size: 64×64 (`MAP_MAX_W` / `MAP_MAX_H`)
- The info plane must contain exactly one spawn cell (parser uses the last one found if multiple exist)
- Edges should be walled off (out-of-bounds is treated as wall, but open edges look wrong)

---

## Key Data Structures

```mermaid
classDiagram
    class GameState {
        Player player
        RayHit hits[800]
        float z_buffer[800]
        Sprite visible_sprites[256]
        int visible_sprite_count
        bool game_over
    }

    class Map {
        uint16 tiles[64][64]
        uint16 info[64][64]
        uint16 sprites[64][64]
        int w
        int h
    }

    class Player {
        float x, y
        float dir_x, dir_y
        float plane_x, plane_y
    }

    class RayHit {
        float wall_dist
        float wall_x
        int side
        uint16 tile_type
    }

    class Input {
        bool forward
        bool back
        bool turn_left
        bool turn_right
    }

    class Sprite {
        float x, y
        float perp_dist
        uint16 texture_id
    }

    GameState *-- Player
    GameState *-- RayHit
    GameState *-- Sprite

    note for GameState "Player state and ray buffer.\nDefined in game_globals.h.\nAllocated on main()'s stack."
    note for Map "Standalone map data (tiles + info + sprites planes).\nDefined in game_globals.h.\nOwned by main(), passed as\nconst Map* to engine functions."
    note for RayHit "Filled every frame by rc_cast().\nConsumed by platform_render().\nOne entry per screen column."
    note for Input "Written by platform layer.\nRead by core engine.\nBridge between layers."
```

### Ownership Model

- `GameState` holds player state and the ray buffer — allocated on `main()`'s stack, passed by pointer everywhere
- `Map` is a **standalone struct** with three planes (tiles + info + sprites) — allocated on `main()`'s stack, initialised by `map_load()`, passed as `const Map *` to engine functions
- `Input` is the **bridge** — written by the platform layer, read by the core engine
- `RayHit[SCREEN_W]` is the **frame buffer** — filled by `rc_cast()`, consumed by `platform_render()`
- Platform state (window, renderer) is **private** to `platform_sdl.c` via file-scoped statics

No heap allocation. No global mutable state (except the platform singletons). No dynamic arrays. This is a codebase where you can trace every byte's lifetime by reading the code.
