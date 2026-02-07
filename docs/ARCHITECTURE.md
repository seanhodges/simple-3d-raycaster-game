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

This project is a **Wolfenstein 3D-style raycasting engine** — it fakes 3D by casting one ray per screen column from the player's viewpoint, measuring the distance to the nearest wall, and drawing a vertical strip whose height is inversely proportional to that distance. The result is a convincing first-person perspective rendered entirely in 2D math.

The design pattern is the **Layered Architecture** (sometimes called Ports & Adapters / Hexagonal at its conceptual root): the core game logic knows nothing about SDL, windows, or pixels. A thin platform layer translates between the engine and the OS.

```mermaid
graph TB
    subgraph "Layer 3 — Orchestrator"
        MAIN["main.c<br/>Game loop, timestep control"]
    end

    subgraph "Layer 1 — Core Engine"
        RC["raycaster.c / .h<br/>Map, Player, DDA, Physics"]
    end

    subgraph "Layer 2 — Platform"
        PLAT["platform_sdl.c / .h<br/>SDL3 window, input, rendering"]
    end

    subgraph "External"
        SDL3["SDL3 Library"]
        MAP["map.txt"]
    end

    MAIN --> RC
    MAIN --> PLAT
    PLAT --> SDL3
    RC --> MAP

    style RC fill:#1a3a5c,stroke:#4a9eff,color:#fff
    style PLAT fill:#3a1a3a,stroke:#9a4aff,color:#fff
    style MAIN fill:#1a3a1a,stroke:#4aff4a,color:#fff
```

---

## Tech Stack & Rationale

| Technology | Role | Why This Choice |
|---|---|---|
| **C (C11)** | Language | Raycasting is a fundamentally low-level algorithm — pointer arithmetic, fixed-size arrays, tight loops. C gives direct control with zero runtime overhead. C11 adds `bool`, `stdbool.h`, and guarantees that make the code cleaner than C89/C99. |
| **SDL3** | Windowing, input, rendering | The de facto standard for cross-platform multimedia in C. SDL provides a 2D renderer, keyboard state polling, and high-resolution timers — exactly what a raycaster needs, nothing more. SDL3 (not SDL2) is used here for its modernized API. |
| **GNU Make** | Build system | For a 3-file C project, Make is the right tool. No CMake boilerplate, no generator step. The `Makefile` handles Linux, macOS, and Windows cross-compilation in 54 lines. |
| **pkg-config** | Dependency discovery | Locates SDL3 headers and libraries portably. Avoids hardcoded paths. |
| **`-Wall -Wextra -O2`** | Compiler discipline | Maximum warnings catch bugs at compile time. `-O2` ensures the tight raycasting loop runs at full speed even on modest hardware. |

**What's deliberately absent:** No game framework, no entity-component system, no scripting language. This is a teaching codebase — every abstraction earns its place.

---

## The Three-Layer Architecture

### Layer 1: Core Engine (`raycaster.c` / `raycaster.h`)

**Design pattern: Pure Computation Module** — this file has zero platform dependencies. It includes `math.h`, `stdio.h`, and `string.h`. That's it. No SDL headers.

Responsibilities:
- **Map loading** (`rc_load_map`) — parse ASCII map file into the `Map` grid
- **Player physics** (`rc_update`) — movement, rotation, collision detection
- **Raycasting** (`rc_cast`) — DDA algorithm fills the `RayHit` buffer

This separation is the most important architectural decision in the project. It means:
- The core engine can be unit-tested without a display
- The renderer can be swapped (OpenGL, Vulkan, terminal) without touching game logic
- The math is portable to any platform with a C compiler

### Layer 2: Platform Abstraction (`platform_sdl.c` / `platform_sdl.h`)

**Design pattern: Adapter** — translates between the engine's data structures and SDL3's API.

Responsibilities:
- Window creation and teardown
- Keyboard state polling (continuous, not event-based — critical for smooth movement)
- Rendering: converts the `RayHit[]` buffer into colored vertical strips

The platform layer **reads from** the core but never writes to it, except through the `Input` struct. Data flows in one direction.

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

    LOOP -->|"poll"| PLAT["platform_sdl"]
    LOOP -->|"update"| RC["raycaster"]
    LOOP -->|"cast"| RC
    LOOP -->|"render"| PLAT
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
        Main->>Core: rc_update(&gs, &input, DT)
        Note over Core: Apply rotation (2D rotation matrix)
        Note over Core: Apply translation (with collision)
        Note over Main: accumulator -= DT
    end

    Main->>Core: rc_cast(&gs)
    Note over Core: For each of 800 screen columns:<br/>Cast ray via DDA, store distance in gs.hits[]

    Main->>Plat: platform_render(&gs)
    Note over Plat: Clear screen (ceiling color)
    Note over Plat: Draw floor rectangle (bottom half)
    Note over Plat: For each column: draw wall strip from hits[]
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
    H --> I["Store in hits[x]:<br/>distance, side, wall_type"]
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

The renderer is intentionally simple — no textures, no sprites, no transparency:

1. **Clear** the screen to `COL_CEIL` (ceiling color fills the entire screen)
2. **Draw floor** — a single rectangle covering the bottom half
3. **Draw walls** — for each of 800 columns, calculate strip height from `hits[x].wall_dist`, draw a 1px-wide filled rectangle
4. **Debug overlay** — player coordinates rendered as text (top-left corner)
5. **Present** — flip the back buffer to screen

### State Management

Platform state (`SDL_Window*`, `SDL_Renderer*`) is stored in **file-scoped static variables** — essentially a singleton. This is appropriate because:
- There is exactly one window and one renderer
- The platform layer is not reentrant
- The API surface is small (4 functions)

---

## Map System

Maps are ASCII text files parsed at startup. The format is simple by design:

```
XXXXXXXXXXXXXXXX
X              X
X  XXXXX  XXX  X
X  X          XX
X  X  XXXXXX  X
X     X    X   X
X  P  X    X   X      ← P marks player spawn
X     X        X
XXXX  XXXX  XXXX
...
```

| Character | Meaning | Cell Value |
|---|---|---|
| `X` or `#` | Wall | `1` |
| `P` or `p` | Player spawn (floor) | `0` |
| ` ` (space) | Empty floor | `0` |

Constraints:
- Maximum size: 64×64 (`MAP_MAX_W` / `MAP_MAX_H`)
- Must contain exactly one `P` (parser uses the last one found if multiple exist)
- Edges should be walled off (out-of-bounds is treated as wall, but open edges look wrong)

The player spawns at the **center** of the `P` cell (`col + 0.5, row + 0.5`) facing east, with the camera plane derived from `FOV_DEG`.

---

## Key Data Structures

```mermaid
classDiagram
    class GameState {
        Map map
        Player player
        RayHit hits[800]
    }

    class Map {
        int cells[64][64]
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
        int side
        int wall_type
    }

    class Input {
        bool forward
        bool back
        bool strafe_left
        bool strafe_right
        bool turn_left
        bool turn_right
    }

    GameState *-- Map
    GameState *-- Player
    GameState *-- RayHit

    note for GameState "The single source of truth.\nPassed by pointer to all functions.\nAllocated on main()'s stack."
    note for RayHit "Filled every frame by rc_cast().\nConsumed by platform_render().\nOne entry per screen column."
    note for Input "Written by platform layer.\nRead by core engine.\nBridge between layers."
```

### Ownership Model

- `GameState` is the **single source of truth** — allocated on `main()`'s stack, passed by pointer everywhere
- `Input` is the **bridge** — written by the platform layer, read by the core engine
- `RayHit[SCREEN_W]` is the **frame buffer** — filled by `rc_cast()`, consumed by `platform_render()`
- Platform state (window, renderer) is **private** to `platform_sdl.c` via file-scoped statics

No heap allocation. No global mutable state (except the platform singletons). No dynamic arrays. This is a codebase where you can trace every byte's lifetime by reading the code.
