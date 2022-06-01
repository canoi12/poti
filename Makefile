NAME = poti
PREFIX =
CC = cc
AR = ar
CLEAR_FILES = 

LUA_DIR = external/lua/src
TEA_DIR = external/tea
MOCHA_DIR = external/mocha

LUA_SRC = $(wildcard $(LUA_DIR)/*.c)
LUA_SRC := $(filter-out $(LUA_DIR)/lua.c,$(LUA_SRC))
LUA_SRC := $(filter-out $(LUA_DIR)/luac.c,$(LUA_SRC))

MOCHA_SRC = $(MOCHA_DIR)/mocha.c

SRC = poti.c $(LUA_SRC) $(MOCHA_SRC)
INCL = -Iexternal/ -I$(LUA_DIR) -I$(MOCHA_DIR) -I$(MOCHA_DIR)/external
EXE = .bin

ifeq ($(OS), Windows_NT)
	CC := gcc
	EXE := .exe
	PREFIX := x86_64-w64-mingw32-
	CFLAGS = -Wall -std=c99
	LFLAGS = -mwindows -lpthread -lmingw32 -lSDL2
else
	CFLAGS = -Wall -std=c99 `sdl2-config --cflags`
	LFLAGS = -lm -lpthread -lSDL2 -ldl `sdl2-config --libs`
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
	./$(GEN)

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
