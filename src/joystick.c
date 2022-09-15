#include "poti.h"
#if !defined(POTI_NO_JOYSTICK) && !defined(POTI_NO_INPUT)

/* LIB FUNCTIONS */
static int l_poti_joystick(lua_State* L) {
    Joystick* joy = SDL_JoystickOpen(luaL_checknumber(L, 1));
    if (joy) {
        Joystick** j = lua_newuserdata(L, sizeof(*j));
        luaL_setmetatable(L, JOYSTICK_META);
        *j = joy;
    }
    else {
        lua_pushnil(L);
    }

    return 1;
}

static int l_poti_joystick_num(lua_State *L) {
    lua_pushinteger(L, SDL_NumJoysticks());
    return 1;
}

/* META FUNCTIONS */
static int l_poti_joystick__name(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushstring(L, SDL_JoystickName(*j));
    return 1;
}

static int l_poti_joystick__vendor(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushnumber(L, SDL_JoystickGetVendor(*j));
    return 1;
}

static int l_poti_joystick__product(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushnumber(L, SDL_JoystickGetProduct(*j));
    return 1;
}

static int l_poti_joystick__product_version(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushnumber(L, SDL_JoystickGetProductVersion(*j));
    return 1;
}

const char *joy_types[] = {
    [SDL_JOYSTICK_TYPE_UNKNOWN] = "unknown",
    [SDL_JOYSTICK_TYPE_GAMECONTROLLER] = "gamepad",
    [SDL_JOYSTICK_TYPE_WHEEL] = "wheel",
    [SDL_JOYSTICK_TYPE_ARCADE_STICK] = "arcade_stick",
    [SDL_JOYSTICK_TYPE_FLIGHT_STICK] = "flight_stick",
    [SDL_JOYSTICK_TYPE_DANCE_PAD] = "dance_pad",
    [SDL_JOYSTICK_TYPE_GUITAR] = "guitar",
    [SDL_JOYSTICK_TYPE_DRUM_KIT] = "drum_kit",
    [SDL_JOYSTICK_TYPE_ARCADE_PAD] = "arcade_pad",
    [SDL_JOYSTICK_TYPE_THROTTLE] = "throttle",
};

static int l_poti_joystick__type(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushstring(L, joy_types[SDL_JoystickGetType(*j)]);
    return 1;
}


static int l_poti_joystick__num_axes(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushnumber(L, SDL_JoystickNumAxes(*j));
    return 1;
}

static int l_poti_joystick__num_balls(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushnumber(L, SDL_JoystickNumBalls(*j));
    return 1;
}

static int l_poti_joystick__num_hats(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushnumber(L, SDL_JoystickNumHats(*j));
    return 1;
}

static int l_poti_joystick__num_buttons(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushnumber(L, SDL_JoystickNumButtons(*j));
    return 1;
}
static int l_poti_joystick__axis(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    int axis = luaL_checknumber(L, 2);
    lua_pushnumber(L, SDL_JoystickGetAxis(*j, axis) / SDL_JOYSTICK_AXIS_MAX);
    return 1;
}

static int l_poti_joystick__ball(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    int ball = luaL_checknumber(L, 2);
    int dx, dy;
    SDL_JoystickGetBall(*j, ball, &dx, &dy);
    lua_pushnumber(L, dx);
    lua_pushnumber(L, dy);
    return 2;
}

static int l_poti_joystick__hat(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    int hat = luaL_checknumber(L, 2);
    lua_pushnumber(L, SDL_JoystickGetHat(*j, hat));
    return 1;
}

static int l_poti_joystick__button(lua_State* L) {
    Joystick** j = luaL_checkudata(L, 1, JOYSTICK_META);
    int button = luaL_checknumber(L, 2);
    int res = SDL_JoystickGetButton(*j, button);

    lua_pushboolean(L, res);
    return 1;
}

static int l_poti_joystick__rumble(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    u16 low = luaL_checknumber(L, 2);
    u16 high = luaL_checknumber(L, 3);
    u32 freq = luaL_optnumber(L, 4, 100);
    lua_pushboolean(L, SDL_JoystickRumble(*j, low, high, freq) == 0);
    return 1;
}

const char *joy_powerlevels[] = {
    "unknown", "empty", "low", "medium", "high", "full", "wired"
};

static int l_poti_joystick__powerlevel(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    lua_pushstring(L, joy_powerlevels[SDL_JoystickCurrentPowerLevel(*j) + 1]);
    return 1;
}

static int l_poti_joystick__close(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_META);
    if (!*j) return 0;
    Joystick* jj = *j;
    if (SDL_JoystickGetAttached(jj)) {
        SDL_JoystickClose(jj);
    }
    return 0;
}

static int l_poti_joystick__gc(lua_State* L) {
    l_poti_joystick__close(L);
    return 0;
}

int luaopen_joystick(lua_State *L) {
    luaL_Reg reg[] = {
        {"open", l_poti_joystick},
        {"num", l_poti_joystick_num},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    luaL_Reg meta[] = {
        {"num_axes", l_poti_joystick__num_axes},
        {"num_hats", l_poti_joystick__num_hats},
        {"num_balls", l_poti_joystick__num_balls},
        {"num_buttons", l_poti_joystick__num_buttons},
        {"axis", l_poti_joystick__axis},
        {"button", l_poti_joystick__button},
        {"hat", l_poti_joystick__hat},
        {"ball", l_poti_joystick__ball},
        {"name", l_poti_joystick__name},
        {"vendor", l_poti_joystick__vendor},
        {"product", l_poti_joystick__product},
        {"product_version", l_poti_joystick__product_version},
        {"type", l_poti_joystick__type},
        {"rumble", l_poti_joystick__rumble},
        {"powerlevel", l_poti_joystick__powerlevel},
        {"__gc", l_poti_joystick__gc},
        {NULL, NULL}
    };

    luaL_newmetatable(L, JOYSTICK_META);
    luaL_setfuncs(L, meta, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    return 1;
}

#endif
