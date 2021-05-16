CC := cc

LUA_DIR = external/lua/src
TEA_DIR = external/tea
MOCHA_DIR = external/mocha

LUA_SRC = $(wildcard $(LUA_DIR)/*.c)
LUA_SRC := $(filter-out $(LUA_DIR)/lua.c,$(LUA_SRC))
LUA_SRC := $(filter-out $(LUA_DIR)/luac.c,$(LUA_SRC))

TEA_SRC = $(TEA_DIR)/tea.c
MOCHA_SRC = $(MOCHA_DIR)/mocha.c

SRC = poti.c $(LUA_SRC) $(TEA_SRC) $(MOCHA_SRC)
INCL = -Iinclude/ -Iexternal/ -I$(LUA_DIR) -I$(TEA_DIR) -I$(TEA_DIR)/external -I$(MOCHA_DIR) -I$(MOCHA_DIR)/external
OUT = poti

CFLAGS = -Wall -std=c99 `sdl2-config --cflags`
LFLAGS = -lm -lpthread -lSDL2 -ldl `sdl2-config --libs`
CDEFS = 

OBJ = $(SRC:%.c=%.o)

.PHONY: all build

all: build

build: $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(INCL) $(CFLAGS) $(LFLAGS) $(CDEFS)

%.o: %.c
	$(CC) -c $< -o $@ -fPIC $(INCL) $(CFLAGS) $(CDEFS)

clean:
	rm -f $(OBJ)
	rm -f $(OUT)
