#include "poti.h"
#if !defined(POTI_NO_WINDOW)

static int l_poti_window_title(lua_State* L) {
    const i8* title = NULL;
    if (!lua_isnil(L, 1)) {
        title = luaL_checkstring(L, 1);
        SDL_SetWindowTitle(POTI()->window, title);
    }

    lua_pushstring(L, SDL_GetWindowTitle(POTI()->window));
    return 1;
}

static int l_poti_window_width(lua_State* L) {
    i32 width = 0;
    if (!lua_isnil(L, 1)) {
        i32 height;
        width = luaL_checkinteger(L, 1);
        SDL_GetWindowSize(POTI()->window, NULL, &height);
        SDL_SetWindowSize(POTI()->window, width, height);
    }
    else SDL_GetWindowSize(POTI()->window, &width, NULL);

    lua_pushinteger(L, width);
    return 1;
}

static int l_poti_window_height(lua_State *L) {
    int height = 0;
    if (!lua_isnil(L, 1)) {
        int width;
        height = luaL_checkinteger(L, 1);
        SDL_GetWindowSize(POTI()->window, &width, NULL);
        SDL_SetWindowSize(POTI()->window, width, height);
    }
    else SDL_GetWindowSize(POTI()->window, NULL, &height);

    lua_pushinteger(L, height);
    return 1;
}

static int l_poti_window_size(lua_State *L) {
    int size[2];

    if (lua_gettop(L) <= 0) SDL_GetWindowSize(POTI()->window, &size[0], &size[1]);

    for (int i = 0; i < lua_gettop(L); i++) {
        size[i] = luaL_checkinteger(L, i+1);
    }
    

    lua_pushinteger(L, size[0]);
    lua_pushinteger(L, size[1]);
    return 2;
}

static int l_poti_window_position(lua_State *L) {
    int pos[2];

    SDL_GetWindowPosition(POTI()->window, &pos[0], &pos[1]);
    int top = lua_gettop(L);
    
    if (top > 0) {
        for (int i = 0; i < top; i++) {
            pos[i] = luaL_checkinteger(L, i+1);
        }
        SDL_SetWindowPosition(POTI()->window, pos[0], pos[1]);
    }

    lua_pushinteger(L, pos[0]);
    lua_pushinteger(L, pos[1]);
    return 2;
}

static int l_poti_window_resizable(lua_State *L) {
    int resizable = 0;
    Uint32 flags = SDL_GetWindowFlags(POTI()->window);

    resizable = flags & SDL_WINDOW_RESIZABLE;

    if (!lua_isnil(L, 1)) {
        resizable = lua_toboolean(L, 1);
        SDL_SetWindowResizable(POTI()->window, resizable);
    }

    lua_pushboolean(L, resizable);

    return 1;
}

static int l_poti_window_bordered(lua_State *L) {
    int bordered = 0;
    Uint32 flags = SDL_GetWindowFlags(POTI()->window);

    bordered = ~flags & SDL_WINDOW_BORDERLESS;

    if (!lua_isnil(L, 1)) {
        bordered = lua_toboolean(L, 1);
        SDL_SetWindowBordered(POTI()->window, bordered);
    }

    lua_pushboolean(L, bordered);
    return 1;
}

static int l_poti_window_border_size(lua_State *L) {

    int borders[4];

    SDL_GetWindowBordersSize(POTI()->window, &borders[0], &borders[1], &borders[2], &borders[3]);

    for (int i = 0; i < 4; i++) lua_pushinteger(L, borders[i]);

    return 4;
}

static int l_poti_window_maximize(lua_State *L) {
    SDL_MaximizeWindow(POTI()->window);
    return 0;
}

static int l_poti_window_minimize(lua_State *L) {
    SDL_MinimizeWindow(POTI()->window);
    return 0;
}

static int l_poti_window_restore(lua_State *L) {
    SDL_RestoreWindow(POTI()->window);
    return 0;
}

static struct {
    const char *name;
    int value;
} message_box_types[] = {
    {"error", SDL_MESSAGEBOX_ERROR},
    {"warning", SDL_MESSAGEBOX_WARNING},
    {"information", SDL_MESSAGEBOX_INFORMATION}
};

static int l_poti_window_simple_message_box(lua_State *L) {
    const char *type = luaL_checkstring(L, 1);
    const char *title = luaL_checkstring(L, 2);
    const char *message = luaL_checkstring(L, 3);

    int flags = 0;
    for (int i = 0; i < 3; i++) {
        if (!strcmp(type, message_box_types[i].name)) {
            flags = message_box_types[i].value;
            break;
        }
    }
    SDL_ShowSimpleMessageBox(flags, title, message, POTI()->window);

    return 0;
}

int luaopen_window(lua_State* L) {
    luaL_Reg reg[] = {
        {"title", l_poti_window_title},
        {"width", l_poti_window_width},
        {"height", l_poti_window_height},
        {"size", l_poti_window_size},
        {"position", l_poti_window_position},
        {"resizable", l_poti_window_resizable},
        {"bordered", l_poti_window_bordered},
        {"border_size", l_poti_window_border_size},
        {"maximize", l_poti_window_maximize},
        {"minimize", l_poti_window_minimize},
        {"restore", l_poti_window_restore},
        {"simple_message_box", l_poti_window_simple_message_box},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}
#endif