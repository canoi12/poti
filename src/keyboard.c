#include "poti.h"
#if !defined(POTI_NO_KEYBOARD) && !defined(POTI_NO_INPUT)

static const Uint8* keys;
extern SDL_Window* _window;

static int l_poti_keyboard_init(lua_State* L) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_context_reg);
    lua_getfield(L, -1, "window");
    _window = (SDL_Window*)lua_touserdata(L, -1);
    lua_pop(L, 2);
    return 0;
}

static int l_poti_keyboard_down(lua_State* L) {
    const char *key = luaL_checkstring(L, 1);

    int code = SDL_GetScancodeFromName(key);
    lua_pushboolean(L, keys[code]);
    
    return 1;
}

static int l_poti_keyboard_up(lua_State* L) {
    const char *key = luaL_checkstring(L, 1);

    int code = SDL_GetScancodeFromName(key);
    lua_pushboolean(L, !keys[code]);
    
    return 1;
}

static int l_poti_keyboard_has_screen_support(lua_State* L) {
    lua_pushboolean(L, SDL_HasScreenKeyboardSupport());
    return 1;
}

static int l_poti_keyboard_is_screen_shown(lua_State* L) {
    lua_pushboolean(L, SDL_IsScreenKeyboardShown(_window));
    return 1;
}

int luaopen_keyboard(lua_State* L) {
    keys = SDL_GetKeyboardState(NULL);
    luaL_Reg reg[] = {
        {"down", l_poti_keyboard_down},
        {"up", l_poti_keyboard_up},
        {"has_screen_support", l_poti_keyboard_has_screen_support},
        {"is_screen_shown", l_poti_keyboard_is_screen_shown},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}

#endif
