# Refactoring Plan: Map Load Reordering & Frontend Renaming

## Overview

This document outlines a three-part refactoring to improve code organization:

1. **Map Load Argument Reordering** — Reorder `map_load()` parameters to group related files logically
2. **Platform → Frontend Renaming** — Rename platform layer to "frontend" to support multiple frontends
3. **Texture Lifecycle Management** — Move texture initialization/shutdown into the frontend layer

---

## Part 1: Reorder `map_load` Arguments

### Current Signature
```c
bool map_load(Map *map, Player *player,
              const char *tiles_path,
              const char *info_path,
              const char *sprites_path);
```

### New Signature
```c
bool map_load(Map *map, Player *player,
              const char *tiles_path,
              const char *sprites_path,
              const char *info_path);
```

### Rationale
- Groups visual assets together (tiles + sprites)
- Places metadata (info) last as it's processed after geometry is loaded
- More logical ordering: geometry first, decoration second, metadata third

### Files to Update

#### 1. `map_manager.h` (line 9-10)
Update function declaration with new parameter order.

#### 2. `map_manager_ascii.c` (line 57-58)
Update function definition with new parameter order.

#### 3. `map_manager_fake.c`
Check if it has the same signature and update accordingly.

#### 4. `main.c` (line 31)
Update call site:
```c
// Before:
if (!map_load(&map, &gs.player, map_tiles_path, map_info_path, map_sprites_path))

// After:
if (!map_load(&map, &gs.player, map_tiles_path, map_sprites_path, map_info_path))
```

#### 5. `test_map_manager_ascii.c`
Update any test call sites to `map_load()`.

### Testing
- Run `ctest --test-dir build` to ensure all tests pass
- Verify the game still loads and runs correctly

---

## Part 2: Rename Platform → Frontend

### Rationale
- "Frontend" better describes the role: rendering and input handling
- Allows for multiple frontends (SDL, terminal, web, etc.)
- "Platform" suggests OS-level abstractions; this is specifically a rendering frontend

### File Renames

| Before | After | Type |
|--------|-------|------|
| `platform_sdl.h` | `frontend.h` | Interface header |
| `platform_sdl.c` | `frontend_sdl.c` | SDL implementation |

Note: `frontend.h` becomes the generic interface header (not `frontend_sdl.h`) since it defines the abstract frontend API.

### Function Renames

All public functions with `platform_` prefix change to `frontend_`:

| Before | After |
|--------|-------|
| `platform_init()` | `frontend_init()` |
| `platform_shutdown()` | `frontend_shutdown()` |
| `platform_poll_input()` | `frontend_poll_input()` |
| `platform_render()` | `frontend_render()` |
| `platform_render_end_screen()` | `frontend_render_end_screen()` |
| `platform_poll_end_input()` | `frontend_poll_end_input()` |
| `platform_get_time()` | `frontend_get_time()` |

### Files to Update

#### 1. Rename Files
```bash
git mv platform_sdl.h frontend.h
git mv platform_sdl.c frontend_sdl.c
```

#### 2. `frontend.h` (was `platform_sdl.h`)

**Header guards (lines 1, 2, 25):**
```c
// Before:
#ifndef PLATFORM_SDL_H
#define PLATFORM_SDL_H
...
#endif /* PLATFORM_SDL_H */

// After:
#ifndef FRONTEND_H
#define FRONTEND_H
...
#endif /* FRONTEND_H */
```

**File-level comments:**
- Update any references to "platform" in comments to "frontend"
- Ensure comments describe this as the abstract frontend interface
- No SDL-specific details should appear in comments (those belong in frontend_sdl.c)

**Function declarations:**
- Update all function declarations with `frontend_` prefix (7 functions total)
- Update any comment documentation above functions that mentions "platform"

#### 3. `frontend_sdl.c` (was `platform_sdl.c`)

**Include directive (line 7):**
```c
// Before:
#include "platform_sdl.h"

// After:
#include "frontend.h"
```

**File header comment (line 1):**
```c
// Before:
/*  platform_sdl.c  –  SDL3 front-end / renderer

// After:
/*  frontend_sdl.c  –  SDL3 frontend / renderer
```

**Function definitions:**
- Update all function definitions with `frontend_` prefix (7 functions)
- Update any comments within functions that reference "platform"
- Check error messages: update `"platform_init:"` → `"frontend_init:"` etc.

#### 4. `main.c`
- Update `#include "platform_sdl.h"` → `#include "frontend.h"`
- Update all function calls: `platform_*` → `frontend_*`

#### 5. `CMakeLists.txt` (line 51)
- Update source file: `platform_sdl.c` → `frontend_sdl.c`

#### 6. Documentation Files
- `docs/ARCHITECTURE.md` — Update diagrams, section headers, and text
- `docs/CONVENTIONS.md` — Update function prefix table and examples
- `README.md` — Check for any references to platform layer

### Header Guard and Comment Checklist

Ensure all references to "platform" are updated:

**frontend.h:**
- [ ] Header guard opening: `#ifndef FRONTEND_H`
- [ ] Header guard define: `#define FRONTEND_H`
- [ ] Header guard closing: `#endif /* FRONTEND_H */`
- [ ] File-level comments updated (no "platform" references)
- [ ] Function declaration comments updated

**frontend_sdl.c:**
- [ ] File header comment updated (platform_sdl.c → frontend_sdl.c)
- [ ] Include directive updated: `#include "frontend.h"`
- [ ] Error messages updated (platform_init → frontend_init, etc.)
- [ ] Internal comments checked for "platform" references

**Other files:**
- [ ] main.c include and comments
- [ ] CMakeLists.txt comments (if any)

### Testing
- Rebuild project: `cmake --build build --clean-first`
- Run tests: `ctest --test-dir build`
- Run game: `./build/raycaster`

---

## Part 3: Move Texture Management into Frontend

### Current State (main.c:41-50)
```c
if (!platform_init()) {
    return 1;
}

if (!tm_init_tiles(texture_tiles_path)) {
    platform_shutdown();
    return 1;
}

if (!tm_init_sprites(texture_sprites_path)) {
    tm_shutdown();
    platform_shutdown();
    return 1;
}
```

### New State (main.c)
```c
if (!frontend_init(texture_tiles_path, texture_sprites_path)) {
    return 1;
}
```

### Rationale
- Texture loading depends on SDL being initialized (SDL_LoadBMP, SDL_Surface)
- The frontend owns the rendering pipeline — it should manage texture lifecycle
- Simplifies main.c by encapsulating SDL-specific initialization
- Better separation of concerns: main.c doesn't need to know about `textures_sdl.h`

### Implementation Plan

#### 1. Update `frontend.h`
Change `frontend_init()` signature:
```c
/** Initialize the frontend and load textures.
 *  tiles_path: path to wall texture atlas BMP
 *  sprites_path: path to sprite texture atlas BMP
 *  Returns false on unrecoverable error. */
bool frontend_init(const char *tiles_path, const char *sprites_path);
```

#### 2. Update `frontend_sdl.c`
- Add `#include "textures_sdl.h"` if not already present
- Modify `frontend_init()`:
  - Call `tm_init_tiles(tiles_path)` after SDL initialization
  - Call `tm_init_sprites(sprites_path)` after tiles initialization
  - Handle failures by calling `tm_shutdown()` and `SDL_Quit()` appropriately
- Modify `frontend_shutdown()`:
  - Call `tm_shutdown()` before SDL cleanup

#### 3. Update `main.c`
- Remove `#include "textures_sdl.h"` (no longer needed)
- Remove texture path variables or pass them to `frontend_init()`
- Replace multi-step initialization with single `frontend_init()` call
- Remove explicit `tm_shutdown()` call (handled by `frontend_shutdown()`)

### Detailed Code Changes

**frontend.h:**
```c
bool frontend_init(const char *tiles_path, const char *sprites_path);
```

**frontend_sdl.c:**
```c
bool frontend_init(const char *tiles_path, const char *sprites_path)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "frontend_init: SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(WINDOW_TITLE, SCREEN_W, SCREEN_H, 0);
    if (!window) {
        fprintf(stderr, "frontend_init: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        fprintf(stderr, "frontend_init: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    fb_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               SCREEN_W, SCREEN_H);
    if (!fb_tex) {
        fprintf(stderr, "frontend_init: SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    /* Load textures */
    if (!tm_init_tiles(tiles_path)) {
        fprintf(stderr, "frontend_init: failed to load tile textures\n");
        SDL_DestroyTexture(fb_tex);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    if (!tm_init_sprites(sprites_path)) {
        fprintf(stderr, "frontend_init: failed to load sprite textures\n");
        tm_shutdown();
        SDL_DestroyTexture(fb_tex);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    return true;
}

void frontend_shutdown(void)
{
    tm_shutdown();
    if (fb_tex)   SDL_DestroyTexture(fb_tex);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    SDL_Quit();
}
```

**main.c:**
```c
/* Remove: #include "textures_sdl.h" */
/* Keep texture path constants or inline them */

/* Replace the multi-step init with: */
if (!frontend_init(texture_tiles_path, texture_sprites_path)) {
    return 1;
}

/* At shutdown, remove tm_shutdown() — it's now in frontend_shutdown(): */
frontend_shutdown();
```

### Testing
- Rebuild and run game
- Verify textures load correctly
- Test failure cases (missing BMP files) to ensure fallback works
- Run all tests

---

## Execution Order

1. **Part 1: Map Load Reordering** (independent, can be done first)
   - Low risk, mechanical change
   - Easy to verify with tests

2. **Part 2: Platform → Frontend Renaming** (independent of Part 1)
   - Purely cosmetic/organizational
   - Can be done before or after Part 1

3. **Part 3: Texture Lifecycle** (depends on Part 2 completion)
   - Requires `frontend_*` names to be in place
   - More complex logic changes

**Recommended order: 1 → 2 → 3**

---

## Validation Checklist

After completing all three parts:

- [ ] All files compile without warnings (`-Wall -Wextra`)
- [ ] `ctest --test-dir build` passes all tests
- [ ] `./build/raycaster` runs and displays correctly
- [ ] Textures load (wall and sprite textures visible)
- [ ] Endgame trigger still works
- [ ] Game exits cleanly (no segfaults, no memory leaks with valgrind if available)
- [ ] Documentation is updated and consistent

---

## Rollback Plan

Each part can be rolled back independently:
- **Part 1**: Revert parameter order changes
- **Part 2**: Rename files back, revert function names
- **Part 3**: Move texture init/shutdown back to main.c

All changes will be committed separately to allow easy reversion.

---

## Notes for Implementation

### Naming Conventions (from docs/CONVENTIONS.md)
- Function prefixes: `frontend_` for the frontend layer (was `platform_`)
- Error messages: Include function name in `fprintf(stderr, ...)` messages
- File headers: Update box-drawing section comments to match new names

### Architecture Impact (from docs/ARCHITECTURE.md)
- Update Layer 2 description to refer to "Frontend" instead of "Platform"
- Update diagrams showing `frontend_sdl.c / .h` instead of `platform_sdl.c / .h`
- Note that texture management is now part of the frontend layer

### Git Commits

Suggested commit structure:
1. `refactor: reorder map_load arguments to group related files`
2. `refactor: rename platform layer to frontend`
3. `refactor: move texture lifecycle into frontend layer`
4. `docs: update architecture and conventions for frontend renaming`

---

## End of Plan

This refactoring improves code organization and sets the foundation for supporting multiple frontends (e.g., terminal, web, headless) in the future.
