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
SRCS     = main.c raycaster.c map.c platform_sdl.c texture.c
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

%.o: %.c raycaster.h platform_sdl.h texture.h map.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET) raycaster.exe \
	      test_raycaster test_raycaster.o fake_map.o \
	      test_map_loader test_map_loader.o

# ── Tests ─────────────────────────────────────────────────────────────

# Unit tests link against fake_map.o (hardcoded test map, no filesystem)
TEST_TARGET     = test_raycaster
TEST_ML_TARGET  = test_map_loader

test: $(TEST_TARGET) $(TEST_ML_TARGET)
	./$(TEST_TARGET)
	./$(TEST_ML_TARGET)

$(TEST_TARGET): test_raycaster.o raycaster.o fake_map.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(TEST_ML_TARGET): test_map_loader.o raycaster.o map.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

test_raycaster.o: test_raycaster.c raycaster.h map.h
	$(CC) $(CFLAGS) -c $< -o $@

test_map_loader.o: test_map_loader.c raycaster.h map.h
	$(CC) $(CFLAGS) -c $< -o $@

fake_map.o: fake_map.c raycaster.h map.h
	$(CC) $(CFLAGS) -c $< -o $@

all: $(TARGET)

.PHONY: all test clean
