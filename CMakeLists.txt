# ──────────────────────────────────────────────────────────────────────
#  CMakeLists.txt – Simple 3D Raycaster Game
#
#  Usage:
#    cmake -B build && cmake --build build          Build (default)
#    cmake -B build && cmake --build build -t test   Build & run tests
# ──────────────────────────────────────────────────────────────────────
cmake_minimum_required(VERSION 3.20)
project(raycaster LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ── Compiler warnings ────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ── SDL3 dependency ──────────────────────────────────────────────────
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL3 IMPORTED_TARGET sdl3)
endif()

if(NOT SDL3_FOUND)
    find_package(SDL3 QUIET CONFIG)
endif()

if(SDL3_FOUND AND TARGET PkgConfig::SDL3)
    set(SDL3_LINK_TARGET PkgConfig::SDL3)
elseif(TARGET SDL3::SDL3)
    set(SDL3_LINK_TARGET SDL3::SDL3)
else()
    message(WARNING
        "SDL3 not found — the main executable will not be built. "
        "Tests (which do not require SDL) will still be available. "
        "Install SDL3 and ensure it is discoverable via pkg-config "
        "or CMake's find_package (SDL3Config.cmake).")
endif()

# ── Main executable (requires SDL3) ─────────────────────────────────
if(SDL3_LINK_TARGET)
    add_executable(raycaster
        main.c
        raycaster.c
        sprites.c
        map_manager_ascii.c
        frontend_sdl.c
        textures_sdl.c
    )

    target_link_libraries(raycaster PRIVATE ${SDL3_LINK_TARGET} m)

    # Copy runtime assets next to the executable so it can be run from
    # the build directory without extra setup.
    add_custom_command(TARGET raycaster POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets    $<TARGET_FILE_DIR:raycaster>/assets
        COMMENT "Copying game assets to build directory"
    )
endif()

# ── Tests (no SDL required) ──────────────────────────────────────────
enable_testing()

# test_raycaster — links against map_manager_fake (hardcoded test map)
add_executable(test_raycaster
    test_raycaster.c
    raycaster.c
    sprites.c
    map_manager_fake.c
)
target_link_libraries(test_raycaster PRIVATE m)
add_test(NAME test_raycaster COMMAND test_raycaster)

# test_map_manager_ascii — links against the real file parser
add_executable(test_map_manager_ascii
    test_map_manager_ascii.c
    raycaster.c
    sprites.c
    map_manager_ascii.c
)
target_link_libraries(test_map_manager_ascii PRIVATE m)

# This test needs map assets present 
add_custom_command(TARGET test_map_manager_ascii POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets    $<TARGET_FILE_DIR:test_map_manager_ascii>/assets
    COMMENT "Copying game assets to test build directory"
)
add_test(
    NAME test_map_manager_ascii
    COMMAND test_map_manager_ascii
    WORKING_DIRECTORY $<TARGET_FILE_DIR:test_map_manager_ascii>
)
