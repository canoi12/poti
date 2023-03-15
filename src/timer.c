#include "poti.h"
#if !defined(POTI_NO_TIMER)
struct Timer {
    double last;
    double delta;
    int fps, frames;
};

static struct Timer timer;

static int l_poti_timer_delta(lua_State* L) {
    lua_pushnumber(L, timer.delta);
    return 1;
}

static int l_poti_timer_ticks(lua_State* L) {
    lua_pushinteger(L, SDL_GetTicks());
    return 1;
}

static int l_poti_timer_delay(lua_State* L) {
    Uint32 ms = luaL_checkinteger(L, 1);
    SDL_Delay(ms);
    return 0;
}

static int l_poti_timer_performance_counter(lua_State* L) {
    lua_pushinteger(L, SDL_GetPerformanceCounter());
    return 1;
}

static int l_poti_timer_performance_freq(lua_State* L) {
    lua_pushinteger(L, SDL_GetPerformanceFrequency());
    return 1;
}

int luaopen_timer(lua_State* L) {
    memset(&timer, 0, sizeof(struct Timer));
    luaL_Reg reg[] = {
        {"delta", l_poti_timer_delta},
        {"tick", l_poti_timer_ticks},
        {"delay", l_poti_timer_delay},
        {"performance_counter", l_poti_timer_performance_counter},
        {"performance_freq", l_poti_timer_performance_freq},
        { NULL, NULL }
    };

    luaL_newlib(L, reg);
    return 1;
}
#endif
