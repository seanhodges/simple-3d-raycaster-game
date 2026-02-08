# ──────────────────────────────────────────────────────────────────────
#  Makefile – Portable C Raycaster (SDL3)
#
#  Usage:
#    make              Build for the current platform (Linux / macOS)
#    make OS=windows   Cross-compile with MinGW
#    make clean        Remove build artefacts
# ──────────────────────────────────────────────────────────────────────

TARGET   = raycaster
SRCS     = main.c raycaster.c platform_sdl.c texture.c
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

%.o: %.c raycaster.h platform_sdl.h texture.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET) raycaster.exe texture.o

# ── Test ───────────────────────────────────────────────────────────────
TEST_TARGET = test_raycaster
TEST_SRCS   = test_raycaster.c raycaster.c
TEST_OBJS   = $(TEST_SRCS:.c=.o)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): test_raycaster.o raycaster.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

test_raycaster.o: test_raycaster.c raycaster.h
	$(CC) $(CFLAGS) -c $< -o $@

all: $(TARGET)

.PHONY: all test clean
