# ──────────────────────────────────────────────────────────────────────
#  Makefile – Portable C Raycaster (SDL3)
#
#  Usage:
#    make              Build for the current platform (Linux / macOS)
#    make OS=windows   Cross-compile with MinGW
#    make clean        Remove build artefacts
# ──────────────────────────────────────────────────────────────────────

TARGET   = raycaster
SRCS     = main.c raycaster.c platform_sdl.c
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
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c raycaster.h platform_sdl.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET) raycaster.exe

.PHONY: all clean
