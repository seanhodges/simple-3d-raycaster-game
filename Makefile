# ──────────────────────────────────────────────────────────────────────
#  Makefile – Simple 3D Raycaster Game
#
#  Usage:
#    make              Build for the current platform (Linux / macOS)
#    make OS=windows   Cross-compile with MinGW
#    make test         Build and run all unit tests
#    make clean        Remove build artefacts
# ──────────────────────────────────────────────────────────────────────

TARGET   = raycaster
SRCS     = main.c raycaster.c map_manager_ascii.c platform_sdl.c textures_sdl.c
OBJS     = $(SRCS:.c=.o)

CC       ?= gcc
CFLAGS   = -std=c11 -Wall -Wextra -O2
LDFLAGS  =

# ── Platform detection ────────────────────────────────────────────────
OS ?= $(shell uname -s 2>/dev/null || echo windows)

ifeq ($(OS),Linux)
  CFLAGS  += $(shell pkg-config --cflags sdl3 2>/dev/null)
  LDFLAGS += $(shell pkg-config --libs   sdl3 2>/dev/null || echo -lSDL3) -lm
endif

ifeq ($(OS),Darwin)
  CFLAGS  += $(shell pkg-config --cflags sdl3 2>/dev/null)
  LDFLAGS += $(shell pkg-config --libs   sdl3 2>/dev/null || echo -lSDL3) -lm
endif

# MinGW / Windows
ifneq (,$(findstring windows,$(OS)))
  CC       = x86_64-w64-mingw32-gcc
  TARGET   = raycaster.exe
  LDFLAGS += -lSDL3 -lm
endif
ifneq (,$(findstring MINGW,$(OS)))
  TARGET   = raycaster.exe
  LDFLAGS += -lSDL3 -lm
endif

# ── Rules ─────────────────────────────────────────────────────────────
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c game_globals.h raycaster.h platform_sdl.h textures_sdl.h map_manager.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET) raycaster.exe \
	      test_raycaster test_raycaster.o map_manager_fake.o \
	      test_map_manager_ascii test_map_manager_ascii.o

# ── Tests ─────────────────────────────────────────────────────────────

# Unit tests link against map_manager_fake.o (hardcoded test map, no filesystem)
TEST_TARGET     = test_raycaster
TEST_ML_TARGET  = test_map_manager_ascii

test: $(TEST_TARGET) $(TEST_ML_TARGET)
	./$(TEST_TARGET)
	./$(TEST_ML_TARGET)

$(TEST_TARGET): test_raycaster.o raycaster.o map_manager_fake.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(TEST_ML_TARGET): test_map_manager_ascii.o raycaster.o map_manager_ascii.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

test_raycaster.o: test_raycaster.c game_globals.h raycaster.h map_manager.h
	$(CC) $(CFLAGS) -c $< -o $@

test_map_manager_ascii.o: test_map_manager_ascii.c game_globals.h raycaster.h map_manager.h
	$(CC) $(CFLAGS) -c $< -o $@

map_manager_fake.o: map_manager_fake.c game_globals.h raycaster.h map_manager.h
	$(CC) $(CFLAGS) -c $< -o $@

all: $(TARGET)

.PHONY: all test clean
