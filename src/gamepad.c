#include "poti.h"
#if !defined(POTI_NO_JOYSTICK) && !defined(POTI_NO_INPUT)

/* LIB FUNCTIONS */

static int l_poti_gamepad_open(lua_State *L) {
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

static int l_poti_gamepad_is(lua_State *L) {
    lua_pushboolean(L, SDL_IsGameController(luaL_checknumber(L, 1)));
    return 1;
}

/* META FUNCTIONS */

static int l_poti_gamepad__name(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    lua_pushstring(L, SDL_GameControllerName(*g));
    return 1;
}

static int l_poti_gamepad__vendor(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    lua_pushnumber(L, SDL_GameControllerGetVendor(*g));
    return 1;
}

static int l_poti_gamepad__product(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    lua_pushnumber(L, SDL_GameControllerGetProduct(*g));
    return 1;
}

static int l_poti_gamepad__product_version(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    lua_pushnumber(L, SDL_GameControllerGetProductVersion(*g));
    return 1;
}

const char *gpad_axes[] = {
    "leftx", "lefty", "rightx", "righty", "triggerleft", "triggerright"
};

static int l_poti_gamepad__axis(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    const char *str = luaL_checkstring(L, 2);
    int axis = SDL_GameControllerGetAxisFromString(str);
    if (axis < 0) {
        return luaL_argerror(L, 2, "Invalid axis name");
    }
    float val = (float)SDL_GameControllerGetAxis(*g, axis) / SDL_JOYSTICK_AXIS_MAX;
    lua_pushnumber(L, val);
    return 1;
}

static int l_poti_gamepad__button(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    const char *str = luaL_checkstring(L, 2);
    int button = SDL_GameControllerGetButtonFromString(str);
    if (button < 0) {
        luaL_argerror(L, 2, "invalid button");
    }
    lua_pushboolean(L, SDL_GameControllerGetButton(*g, button));
    return 1;
}

static int l_poti_gamepad__rumble(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    Uint16 low = (Uint16)luaL_checknumber(L, 2);
    Uint16 high = (Uint16)luaL_checknumber(L, 3);
    Uint32 freq = (Uint32)luaL_optnumber(L, 4, 100);
    lua_pushboolean(L, SDL_GameControllerRumble(*g, low, high, freq) == 0);
    return 1;
}

const char *gpad_powerlevels[] = {
    "unknown", "empty", "low", "medium", "high", "full", "wired"
};

static int l_poti_gamepad__powerlevel(lua_State* L) {
	GameController** g = luaL_checkudata(L, 1, GAMEPAD_META);
	if (!(*g)) {
		return luaL_error(L, "Closed joystick");
	}
	SDL_Joystick *j = SDL_GameControllerGetJoystick(*g);
	lua_pushstring(L, gpad_powerlevels[SDL_JoystickCurrentPowerLevel(j)+1]);
	return 1;
}

static int l_poti_gamepad__close(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_META);
    if (SDL_GameControllerGetAttached(*g)) SDL_GameControllerClose(*g);
    return 0;
}

static int l_poti_gamepad__gc(lua_State* L) {
    l_poti_gamepad__close(L);
    return 0;
}

int luaopen_gamepad(lua_State *L) {
    luaL_Reg reg[] = {
        {"open", l_poti_gamepad_open},
        {"is", l_poti_gamepad_is},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);

    luaL_Reg meta[] = {
        {"axis", l_poti_gamepad__axis},
        {"button", l_poti_gamepad__button},
        {"name", l_poti_gamepad__name},
        {"vendor", l_poti_gamepad__vendor},
        {"product", l_poti_gamepad__product},
        {"product_version", l_poti_gamepad__product_version},
        {"rumble", l_poti_gamepad__rumble},
        {"powerlevel", l_poti_gamepad__powerlevel},
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
