#include "poti.h"
#include "embed.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

int l_context_reg;

int l_error_reg;
int l_init_reg;
int l_deinit_reg;

int l_step_reg;

static int _running = 1;

SDL_Window* _window;
SDL_GLContext _gl_ctx;
lua_State* _L;

static int l_poti_version(lua_State* L) {
    lua_pushstring(L, POTI_VER);
    return 1;
}

static int l_poti_running(lua_State* L) {
    i32 top = lua_gettop(L);
    if (top > 0) {
	_running = lua_toboolean(L, 1);
    }
    lua_pushboolean(L, _running);
    return 1;
}

static int l_poti_error(lua_State* L) {
    const i8* msg = luaL_checkstring(L, 1);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_error_reg);
    lua_pushstring(L, msg);
    lua_pcall(L, 1, 0, 0);
    return 0;
}

static int l_poti_set_func(lua_State* L) {
    const i8* func = luaL_checkstring(L, 1);
    if (lua_type(L, 2) != LUA_TFUNCTION) {
	luaL_error(L, "poti step must be a function");
	return 1;
    }
    lua_pushvalue(L, -1);
    int* reg = NULL;
    if (!strcmp(func, "step")) reg = &l_step_reg;
    else if (!strcmp(func, "error")) reg = &l_error_reg;
    lua_rawsetp(L, LUA_REGISTRYINDEX, reg);
    return 0;
}

int luaopen_poti(lua_State* L) {
    luaL_Reg reg[] = {
	{"version", l_poti_version},
	{"running", l_poti_running},
	{"error", l_poti_error},
	{"set_func", l_poti_set_func},
	{"gl", NULL},
	{NULL, NULL}
    };
    luaL_newlib(L, reg);

    struct { i8* name; lua_CFunction fn; } libs[] = {
#if !defined(POTI_NO_AUDIO)
	{"audio", luaopen_audio},
#endif
#if !defined(POTI_NO_EVENT)
	{"event", luaopen_event},
#endif
#if !defined(POTI_NO_FILESYSTEM)
	{"filesystem", luaopen_filesystem},
#endif
#if !defined(POTI_NO_GRAPHICS) && !defined(POTI_NO_WINDOW)
	{"graphics", luaopen_graphics},
#endif
#if !defined(POTI_NO_INPUT)
#if !defined(POTI_NO_JOYSTICK)
	{"gamepad", luaopen_gamepad},
	{"joystick", luaopen_joystick},
#endif
#if !defined(POTI_NO_KEYBOARD)
	{"keyboard", luaopen_keyboard},
#endif
#if !defined(POTI_NO_MOUSE)
	{"mouse", luaopen_mouse},
#endif
#endif
#if !defined(POTI_NO_TIMER)
	{"timer", luaopen_timer},
#endif
#if !defined(POTI_NO_WINDOW)
	{"window", luaopen_window},
#endif
	{NULL, NULL}
    };

    i32 i;
    for (i = 0; libs[i].name; i++) {
	if (libs[i].fn) {
	    libs[i].fn(L);
	    lua_setfield(L, -2, libs[i].name);
	}
    }
    return 1;
}

int poti_init(int argc, char** argv) {
    _L = luaL_newstate();
    lua_State* L = _L;
    _window = NULL;

    luaL_openlibs(L);

    luaL_requiref(L, "poti",  luaopen_poti, 1);

    lua_getglobal(L, "poti");
    lua_newtable(L);
    for (i32 i = 0; i < argc; i++) {
	lua_pushstring(L, argv[i]);
	lua_rawseti(L, -2, i+1);
    }
    lua_setfield(L, -2, "args");
    lua_pop(L, 1);

    i32 flags = SDL_INIT_SENSOR;
#if !defined(POTI_NO_AUDIO)
    flags |= SDL_INIT_AUDIO;
#endif
#if !defined(POTI_NO_WINDOW)
    flags = SDL_INIT_VIDEO;
#endif

#if !defined(POTI_NO_INPUT) && !defined(POTI_NO_JOYSTICK)
    flags |= SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER;
#endif

#if !defined(POTI_NO_TIMER)
    flags |= SDL_INIT_TIMER;
#endif

#if !defined(POTI_NO_EVENT)
    flags |= SDL_INIT_EVENTS;
#endif

    if (SDL_Init(flags)) {
	fprintf(stderr, "Failed to init SDL2: %s\n", SDL_GetError());
	exit(EXIT_FAILURE);
    }

#ifndef DEBUG_EMBED_LUA
    if (luaL_dostring(L, _embed_boot_lua) != LUA_OK) {
	const i8* error_buf = lua_tostring(L, -1);
	fprintf(stderr, "Failed to load poti lua boot: %s\n", error_buf);
	exit(EXIT_FAILURE);
    }
#else
    if (luaL_dofile(L, "embed/boot.lua") != LUA_OK) {
	const i8* error_buf = lua_tostring(L, -1);
	fprintf(stderr, "Failed to load poti lua boot: %s\n", error_buf);
	exit(EXIT_FAILURE);
    }
#endif
    lua_rawsetp(L, LUA_REGISTRYINDEX, &l_error_reg);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &l_init_reg);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &l_deinit_reg);

    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_init_reg);
    if (lua_pcall(L, 0, 2, 0) != LUA_OK) {
	const i8* error_buf = lua_tostring(L, -1);
	fprintf(stderr, "Failed to load poti init function: %s\n", error_buf);
	exit(EXIT_FAILURE);
    }
    _window = lua_touserdata(L, -1);
    _gl_ctx = lua_touserdata(L, -2);
    lua_pop(L, 2);

#if 0
#if !defined(POTI_NO_WINDOW)
    lua_pushcfunction(L, poti_init_window);
    if (lua_pcall(L, 0, 1, 0) != 0) {
	fprintf(stderr, "Failed to init Window: %s\n", lua_tostring(L, -1));
	exit(EXIT_FAILURE);
    }
    _window = lua_touserdata(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, _window);
    #if !defined(POTI_NO_GRAPHICS)

    lua_pushcfunction(L, poti_init_graphics);
    lua_pushlightuserdata(L, _window);
    if (lua_pcall(L, 1, 1, 0) != 0) {
	fprintf(stderr, "Failed to init graphics: %s\n", lua_tostring(L, -1));
	exit(EXIT_FAILURE);
    }
    _gl_ctx = lua_touserdata(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, _gl_ctx);
    #endif
#endif

#if !defined(POTI_NO_AUDIO)
    lua_pushcfunction(L, poti_init_audio);
    if (lua_pcall(L, 0, 0, 0) != 0) {
	fprintf(stderr, "Failed to init audio: %s\n", lua_tostring(L, -1));
	exit(EXIT_FAILURE);
    }
#endif

    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_step_reg);
    i32 err = lua_pcall(L, 0, 1, 0);
    if (err != 0) {
	const i8* str = lua_tostring(L, -1);
	fprintf(stderr, "Lua error: %s\n", str);
	exit(EXIT_FAILURE);
    }
    lua_rawsetp(L, LUA_REGISTRYINDEX, &l_step_reg);
#endif
    return 1;
}

static void poti_step(void) {
    lua_State* L = _L;
    //poti_graphics_begin(L);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_step_reg);
    lua_pcall(L, 0, 0, 0);
    //poti_graphics_end(L);
    SDL_GL_SwapWindow(_window);
}

int poti_loop(void) {
#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(poti_step, 0, 1);
#else
    while (_running) poti_step();
#endif
    return 0;
}

int poti_quit(void) {
    lua_State* L = _L;
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_deinit_reg);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
	const i8* error_buf = lua_tostring(L, -1);
	fprintf(stderr, "Failed to call lua deinit function: %s\n", error_buf);
	exit(EXIT_FAILURE);
    }
    SDL_Quit();
    return 0;
}

int main(int argc, char** argv) {
    poti_init(argc, argv);
    poti_loop();
    poti_quit();
    return 0;
}
