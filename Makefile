NAME = poti
PREFIX =
CC = cc
AR = ar
CLEAR_FILES =
NO_EMBED = 0

RELEASE = 0

USEJIT = 0

LUA_DIR = external/lua/src
# LUA_DIR = external/luajit/src
GLAD_DIR = external/glad

GEN_CC = cc
GEN_EXE =


LUA_SRC = $(wildcard $(LUA_DIR)/*.c)
LUA_SRC := $(filter-out $(LUA_DIR)/lua.c,$(LUA_SRC))
LUA_SRC := $(filter-out $(LUA_DIR)/luac.c,$(LUA_SRC))

SRC = poti.c impl.c $(LUA_SRC)
SRC += $(wildcard src/*.c)
INCL = -I. -Iexternal/ -I$(LUA_DIR) -I$(GLAD_DIR)/include
EXE = .bin
CFLAGS = 
ifeq ($(RELEASE), 1)
	CFLAGS += -O2
else
	CFLAGS += -g
endif

CDEFS =
ifeq ($(NO_EMBED), 1)
	CDEFS += -DNO_EMBED
endif

ifeq ($(OS), Windows_NT)
	GEN_EXE = .exe
	GEN_CC = x86_64-w64-mingw32-gcc
	TARGET ?= Windows
else
	GEN_EXE = .bin
	GEN_CC = gcc
endif

ifeq ($(TARGET), Windows)
	CC := gcc
	EXE := .exe
	PREFIX := x86_64-w64-mingw32-
	CFLAGS += -Wall -std=c99
	INCL += -Iexternal/SDL2/include
	LFLAGS =-Lexternal/SDL2/lib -Wl,-Bdynamic -lSDL2 -Wl,-Bstatic -mwindows -lpthread -lmingw32 -lopengl32
	PATH := $(PATH):external/SDL2/bin
else
	ifeq ($(TARGET), Web)
		CC := emcc
		CFLAGS += -Wall -std=gnu99 -s USE_SDL=2
		GEN_CFLAGS = -Wall -std=gnu99
		LFLAGS = -lm -ldl -s ALLOW_MEMORY_GROWTH -s FORCE_FILESYSTEM -s WASM=1 -s FULL_ES2 
		EXE := .html
		CLEAR_FILES = *.wasm *.js
		GEN_CC := cc
	else
		CFLAGS += -Wall -std=c99 `sdl2-config --cflags`
		LFLAGS = -lm -lpthread -lSDL2 -ldl `sdl2-config --libs` -lGL
	endif
endif

OUT = $(NAME)$(EXE)
GEN = gen$(GEN_EXE)
OBJ = $(SRC:%.c=%.o)
EMBED = embed/boot.lua embed/font.ttf embed/audio_bank.lua embed/shader_factory.lua

.PHONY: all build
.SECONDARY: $(EMBED)

all: build

build: $(OUT)

embed.h: $(GEN) $(EMBED)
	@echo "********************************************************"
	@echo "** GENERATING $@"
	@echo "********************************************************"
	./$(GEN) -n EMBED_H -t embed.h $(EMBED)

$(GEN): gen.o
	@echo "********************************************************"
	@echo "** BUILDING $@"
	@echo "********************************************************"
	$(GEN_CC) $< -o $@ $(GEN_CFLAGS)

$(OUT): poti.h embed.h $(OBJ)
	@echo "********************************************************"
	@echo "** BUILDING $@"
	@echo "********************************************************"
	@echo $(OS)
	$(PREFIX)$(CC) $(OBJ) -o $(OUT) $(INCL) $(LFLAGS) $(CFLAGS) $(CDEFS)

gen.o: gen.c
	@echo "********************************************************"
	@echo "** $@: COMPILING SOURCE $<"
	@echo "********************************************************"
	$(GEN_CC) -c $< -o $@ -fPIC $(INCL) $(GEN_CFLAGS) $(CDEFS)

poti.o: poti.c poti.h embed.h
	@echo "********************************************************"
	@echo "** $@: COMPILING SOURCE $<"
	@echo "********************************************************"
	$(PREFIX)$(CC) -c $< -o $@ -fPIC $(INCL) $(CFLAGS) $(CDEFS)

%.o: %.c
	@echo "********************************************************"
	@echo "** $@: COMPILING SOURCE $<"
	@echo "********************************************************"
	$(PREFIX)$(CC) -c $< -o $@ -fPIC $(INCL) $(CFLAGS) $(CDEFS)

clean:
	rm -f $(OBJ)
	rm -f $(OUT)
	rm -f gen.o $(GEN)
	rm -f $(CLEAR_FILES)
