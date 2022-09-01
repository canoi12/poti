#include "poti.h"
#if !defined(POTI_NO_JOYSTICK) && !defined(POTI_NO_INPUT)

static int l_poti_gamepad(lua_State *L) {
    GameController* controller = SDL_GameControllerOpen(luaL_checknumber(L, 1));
    if (controller) {
        GameController** g = lua_newuserdata(L, sizeof(*g));
        *g = controller;
        luaL_setmetatable(L, GAMEPAD_META);
    }
    else {
        lua_pushnil(L);
    }
    
    return 1;
}

static int l_poti_is_gamepad(lua_State *L) {
    lua_pushboolean(L, SDL_IsGameController(luaL_checknumber(L, 1)));
    return 1;
}

static int l_poti_gamepad_name(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    lua_pushstring(L, SDL_GameControllerName(*g));
    return 1;
}

static int l_poti_gamepad_vendor(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    lua_pushnumber(L, SDL_GameControllerGetVendor(*g));
    return 1;
}

static int l_poti_gamepad_product(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    lua_pushnumber(L, SDL_GameControllerGetProduct(*g));
    return 1;
}

static int l_poti_gamepad_product_version(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    lua_pushnumber(L, SDL_GameControllerGetProductVersion(*g));
    return 1;
}

const char *gpad_axes[] = {
    "leftx", "lefty", "rightx", "righty", "triggerleft", "triggerright"
};

static int l_poti_gamepad_axis(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    const char *str = luaL_checkstring(L, 2);
    int axis = SDL_GameControllerGetAxisFromString(str);
    if (axis < 0) {
        luaL_argerror(L, 2, "invalid axis");
    }
    f32 val = (f32)SDL_GameControllerGetAxis(*g, axis) / SDL_JOYSTICK_AXIS_MAX;
    lua_pushnumber(L, val);
    return 1;
}

static int l_poti_gamepad_button(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    const char *str = luaL_checkstring(L, 2);
    int button = SDL_GameControllerGetButtonFromString(str);
    if (button < 0) {
        luaL_argerror(L, 2, "invalid button");
    }
    lua_pushboolean(L, SDL_GameControllerGetButton(*g, button));
    return 1;
}

static int l_poti_gamepad_rumble(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    u16 low = (u16)luaL_checknumber(L, 2);
    u16 high = (u16)luaL_checknumber(L, 3);
    u32 freq = (u32)luaL_optnumber(L, 4, 100);
    lua_pushboolean(L, SDL_GameControllerRumble(*g, low, high, freq) == 0);
    return 1;
}

static int l_poti_gamepad_close(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    if (SDL_GameControllerGetAttached(*g)) {
        SDL_GameControllerClose(*g);
    }
    return 0;
}

static int l_poti_gamepad__gc(lua_State* L) {
    l_poti_gamepad_close(L);
    return 0;
}

int luaopen_gamepad(lua_State *L) {
    luaL_Reg reg[] = {
        {"open", l_poti_gamepad},
        {"is", l_poti_is_gamepad},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    luaL_Reg meta[] = {
        {"axis", l_poti_gamepad_axis},
        {"button", l_poti_gamepad_button},
        {"name", l_poti_gamepad_name},
        {"vendor", l_poti_gamepad_vendor},
        {"product", l_poti_gamepad_product},
        {"product_version", l_poti_gamepad_product_version},
        {"rumble", l_poti_gamepad_rumble},
        // {"powerlevel", l_poti_gamepad_powerlevel},
        {"__gc", l_poti_gamepad__gc},
        {NULL, NULL}
    };
    luaL_newmetatable(L, GAMEPAD_META);
    luaL_setfuncs(L, meta, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    return 1;
}

#endif