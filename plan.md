# Plan: Split Map into Tiles and Info Planes

## Overview

Separate the single `cells` grid into two named planes:
- **tiles**: wall/floor geometry (renamed from `cells`)
- **info**: metadata like player spawn (with direction) and exit trigger

The tiles plane replaces old `P` and `F` cells with floor (`0`). A new ASCII file `map_info.txt` represents the info plane.

All constants are renamed to reflect their plane: `CELL_` prefix becomes `TILE_` for the tiles plane, and `INFO_` for the info plane.

## Constant Renaming

### Tiles plane (`TILE_` prefix)
| Old Name     | New Name     | Value |
|---|---|---|
| `CELL_FLOOR` | `TILE_FLOOR` | `0`   |
| `CELL_EXIT`  | *(removed)*  | —     |

Wall cells remain encoded as `>0` (wall_type = cell - 1). No `TILE_WALL` constant needed since wall type is variable.

### Info plane (`INFO_` prefix)
| Name                      | Value | Meaning                          |
|---|---|---|
| `INFO_EMPTY`              | `0`   | No info at this cell             |
| `INFO_SPAWN_PLAYER_N`     | `1`   | Player spawn, facing north       |
| `INFO_SPAWN_PLAYER_E`     | `2`   | Player spawn, facing east        |
| `INFO_SPAWN_PLAYER_S`     | `3`   | Player spawn, facing south       |
| `INFO_SPAWN_PLAYER_W`     | `4`   | Player spawn, facing west        |
| `INFO_TRIGGER_ENDGAME`    | `5`   | Exit / endgame trigger           |

## Info Plane ASCII Format (`map_info.txt`)

| Character | Meaning                        |
|---|---|
| ` ` (space) | Empty (INFO_EMPTY)           |
| `^`         | Spawn facing north           |
| `>`         | Spawn facing east            |
| `V`         | Spawn facing south           |
| `<`         | Spawn facing west            |
| `F`         | Endgame trigger              |

## Files to Change

### 1. `game_globals.h`
- Rename `cells` to `tiles` in the `Map` struct
- Add `info[MAP_MAX_H][MAP_MAX_W]` plane to `Map`

### 2. `raycaster.h`
- Rename `CELL_FLOOR` → `TILE_FLOOR`
- Remove `CELL_EXIT` (no longer exists in tiles plane)
- Add info plane constants: `INFO_EMPTY`, `INFO_SPAWN_PLAYER_N/E/S/W`, `INFO_TRIGGER_ENDGAME`

### 3. `raycaster.c`
- Update `is_wall()` to use `m->tiles` instead of `m->cells`
- Simplify `is_wall()`: just `cell > 0` (no more `< 65535` special case since `CELL_EXIT` is gone from tiles)
- Update `rc_update()`: exit detection reads from `map->info` plane (`INFO_TRIGGER_ENDGAME`) instead of `map->tiles == CELL_EXIT`
- Update `rc_cast()`: use `map->tiles` for wall detection and texture lookup. Remove `CELL_EXIT` special case from the DDA loop — simplify to just `> TILE_FLOOR`.
- Rename `CELL_FLOOR` → `TILE_FLOOR`, `CELL_EXIT` → `INFO_TRIGGER_ENDGAME` in all references

### 4. `map_manager.h`
- Change signature: `bool map_load(Map *map, Player *player, const char *tiles_path, const char *info_path);`

### 5. `map_manager_ascii.c`
- Parse `tiles_path` into `map->tiles[][]` — only wall chars (`X`, `#`, `0`-`9`) and spaces. `P` and `F` no longer appear in the tiles file.
- Parse `info_path` into `map->info[][]` — reads `^/>/<` and `V` for spawn direction, `F` for endgame trigger
- Set player spawn position from the info plane spawn cell (center of cell)
- Set player direction from spawn direction (`N`=north `(0,-1)`, `E`=east `(1,0)`, `S`=south `(0,1)`, `W`=west `(-1,0)`)
- Derive camera plane perpendicular to direction, scaled by `tan(FOV/2)`
- Validate: exactly one spawn cell must exist in the info plane
- Use `TILE_FLOOR` instead of `CELL_FLOOR`

### 6. `map_manager_fake.c`
- Update fake map to populate both `tiles` and `info` planes
- Move exit to info: `info[1][3] = INFO_TRIGGER_ENDGAME`
- Move player spawn to info: `info[1][1] = INFO_SPAWN_PLAYER_E`
- Tiles at old P/F positions become `TILE_FLOOR` (0)
- Use `map->tiles` instead of `map->cells`
- Remove `-1` (old CELL_EXIT) from `fake_cells` array, replace with `0`

### 7. `main.c`
- Update `map_load()` call to pass both paths: `map_load(&map, &gs.player, "map.txt", "map_info.txt")`

### 8. `map.txt`
- Replace `P` at row 6, col 4 with a space (floor)
- Replace `F` at row 1, col 4 with a space (floor)

### 9. `map_info.txt` (new file)
- Same dimensions as `map.txt` (16 wide × 17 tall)
- All spaces except:
  - `>` at row 6, col 4 (spawn facing east)
  - `F` at row 1, col 4 (endgame trigger)

### 10. `test_raycaster.c`
- Update `load_fake_map` — no signature change needed (fake map ignores paths)
- Update all `map.cells[y][x]` → `map.tiles[y][x]`
- Update `init_box_map` to use `map->tiles`
- Update `CELL_FLOOR` → `TILE_FLOOR` everywhere
- Update exit cell tests to check `map.info[y][x] == INFO_TRIGGER_ENDGAME` instead of `map.tiles[y][x] == CELL_EXIT`
- Update exit tests: place `INFO_TRIGGER_ENDGAME` in `map->info` instead of `CELL_EXIT` in tiles
- Remove `test_fake_map_exit_cell` (exit no longer in tiles) — replace with test checking `map.info` for `INFO_TRIGGER_ENDGAME`
- Remove `test_fake_map_exit_is_walkable` (no longer meaningful since exit is just floor in tiles)
- Update `test_fake_map_all_wall_types_present` — remove `CELL_EXIT` filtering (tiles only have walls and floor now)
- Update `test_update_exit_cell_walkable` — exit cell is just floor in tiles, place `INFO_TRIGGER_ENDGAME` in info plane
- Update `test_update_exit_triggers_game_over` — use info plane
- Update `test_update_exit_requires_centre` — use info plane
- Add test for fake map info plane spawn cell (`INFO_SPAWN_PLAYER_E` at expected position)

### 11. `test_map_manager_ascii.c`
- Update `map_load` calls to pass both paths: `"map.txt", "map_info.txt"`
- Update `map.cells` references → `map.tiles`
- Update `CELL_FLOOR` → `TILE_FLOOR`, `CELL_EXIT` → remove
- Update cell range validation: tiles plane values are `0` to some max wall value (no more 65535)
- Add test for info plane having a spawn cell
- Add test for info plane having an endgame trigger cell

### 12. `Makefile`
- No structural changes needed (same .o files, same linking)

### 13. `docs/ARCHITECTURE.md` and `docs/CONVENTIONS.md`
- Update Map System section to describe the two-plane architecture
- Update cell value table: remove `P` and `F` from tiles, add info plane table
- Update Map struct documentation to show both planes
- Update constant naming conventions to reflect `TILE_` and `INFO_` prefixes
- Update `map_load` signature documentation
