#include "poti.h"
#if !defined(POTI_NO_MOUSE) && !defined(POTI_NO_INPUT)

static int l_poti_mouse_pos(lua_State *L) {
    int x, y;
    SDL_GetMouseState(&x, &y);

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

static int mouse_down(int btn) {
    return SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(btn);
}

static int l_poti_mouse_down(lua_State *L) {
    int button = luaL_checknumber(L, 1);

    lua_pushboolean(L, mouse_down(button));
    
    return 1;
}

static int l_poti_mouse_up(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, !mouse_down(button));
    
    return 1;
}

int luaopen_mouse(lua_State *L) {
    luaL_Reg reg[] = {
        {"position", l_poti_mouse_pos},
        {"down", l_poti_mouse_down},
        {"up", l_poti_mouse_up},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}

#endif