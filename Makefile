NAME = poti
PREFIX =
CC = cc
AR = ar
CLEAR_FILES = 

LUA_DIR = external/lua/src

TARGET ?= Linux
ifeq ($(OS), Windows_NT)
	TARGET := Window
endif

LUA_SRC = $(wildcard $(LUA_DIR)/*.c)
LUA_SRC := $(filter-out $(LUA_DIR)/lua.c,$(LUA_SRC))
LUA_SRC := $(filter-out $(LUA_DIR)/luac.c,$(LUA_SRC))

SRC = poti.c impl.c $(LUA_SRC)
INCL = -Iexternal/ -I$(LUA_DIR)
EXE = .bin

ifeq ($(TARGET), Windows)
	CC := gcc
	EXE := .exe
	PREFIX := x86_64-w64-mingw32-
	CFLAGS = -Wall -std=c99
	INCL += -Iexternal/SDL2/include
	LFLAGS = -mwindows -lpthread -lmingw32 -Lexternal/SDL2/lib -lSDL2
else
	ifeq ($(TARGET), Web)
		CC := emcc
		CFLAGS = -Wall -std=gnu99 -sALLOW_MEMORY_GROWTH -sFORCE_FILESYSTEM -s WASM=1 -s USE_SDL=2
		LFLAGS = -lm -ldl
		EXE := .html
		CLEAR_FILES = *.wasm *.js
	else
		CFLAGS = -Wall -std=c99 `sdl2-config --cflags`
		LFLAGS = -lm -lpthread -lSDL2 -ldl `sdl2-config --libs`
	endif
endif
CDEFS = 

OUT = $(NAME)$(EXE)
GEN = gen$(EXE)
OBJ = $(SRC:%.c=%.o)

.PHONY: all build

all: build

build: $(OUT)

poti.h: $(GEN)
	@echo "********************************************************"
	@echo "** GENERATING $@"
	@echo "********************************************************"
# 	./$(GEN)

$(GEN): gen.o boot.lua
	@echo "********************************************************"
	@echo "** BUILDING $@"
	@echo "********************************************************"
	$(PREFIX)$(CC) $< -o $@ $(CFLAGS)

$(OUT): poti.h $(OBJ)
	@echo "********************************************************"
	@echo "** BUILDING $@"
	@echo "********************************************************"
	@echo $(OS)
	$(PREFIX)$(CC) $(OBJ) -o $(OUT) $(INCL) $(CFLAGS) $(LFLAGS) $(CDEFS)

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
