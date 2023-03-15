#include "poti.h"
#if !defined(POTI_NO_WINDOW)

extern SDL_Window* _window;

// int poti_init_window(lua_State* L) {
int l_poti_window_init(lua_State* L) {
#if 1
    lua_getfield(L, 1, "window");
    lua_getfield(L, -1, "title");
    const char* title = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "width");
    int width = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, -1, "height");
    int height = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    int window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
    lua_getfield(L, -1, "resizable");
    if (lua_toboolean(L, -1)) window_flags |= SDL_WINDOW_RESIZABLE;
    lua_pop(L, 1);
    lua_getfield(L, -1, "fullscreen");
    if (lua_toboolean(L, -1)) window_flags |= SDL_WINDOW_FULLSCREEN;
    lua_pop(L, 1);
    lua_getfield(L, -1, "borderless");
    if (lua_toboolean(L, -1)) window_flags |= SDL_WINDOW_BORDERLESS;
    lua_pop(L, 1);
    lua_getfield(L, -1, "always_on_top");
    if (lua_toboolean(L, -1)) window_flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    lua_pop(L, 2);
#else
    const char* title = "poti "POTI_VER;
    int width, height;
    width = 640;
    height = 380;
    int window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
#endif
    _window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
    lua_pushlightuserdata(L, _window);
    return 1;
}

static int l_poti_window_deinit(lua_State* L) {
    SDL_DestroyWindow(_window);
    return 0;
}

static int l_poti_window_title(lua_State* L) {
    const char* title = NULL;
    if (!lua_isnil(L, 1)) {
        title = luaL_checkstring(L, 1);
        SDL_SetWindowTitle(_window, title);
    }

    lua_pushstring(L, SDL_GetWindowTitle(_window));
    return 1;
}

static int l_poti_window_width(lua_State* L) {
    int width = 0;
    if (!lua_isnil(L, 1)) {
        int height;
        width = (int)luaL_checkinteger(L, 1);
        SDL_GetWindowSize(_window, NULL, &height);
        SDL_SetWindowSize(_window, width, height);
    }
    else SDL_GetWindowSize(_window, &width, NULL);

    lua_pushinteger(L, width);
    return 1;
}

static int l_poti_window_height(lua_State *L) {
    int height = 0;
    if (!lua_isnil(L, 1)) {
        int width;
        height = (int)luaL_checkinteger(L, 1);
        SDL_GetWindowSize(_window, &width, NULL);
        SDL_SetWindowSize(_window, width, height);
    }
    else SDL_GetWindowSize(_window, NULL, &height);

    lua_pushinteger(L, height);
    return 1;
}

static int l_poti_window_size(lua_State *L) {
    int size[2];

    if (lua_gettop(L) <= 0) SDL_GetWindowSize(_window, &size[0], &size[1]);

    for (int i = 0; i < lua_gettop(L); i++) {
        size[i] = (int)luaL_checkinteger(L, i+1);
    }
    

    lua_pushinteger(L, size[0]);
    lua_pushinteger(L, size[1]);
    return 2;
}

static int l_poti_window_position(lua_State *L) {
    int pos[2];

    SDL_GetWindowPosition(_window, &pos[0], &pos[1]);
    int top = lua_gettop(L);
    
    if (top > 0) {
        for (int i = 0; i < top; i++) {
            pos[i] = (int)luaL_checkinteger(L, i+1);
        }
        SDL_SetWindowPosition(_window, pos[0], pos[1]);
    }

    lua_pushinteger(L, pos[0]);
    lua_pushinteger(L, pos[1]);
    return 2;
}

static int l_poti_window_resizable(lua_State *L) {
    int resizable = 0;
    Uint32 flags = SDL_GetWindowFlags(_window);

    resizable = flags & SDL_WINDOW_RESIZABLE;

    if (!lua_isnil(L, 1)) {
        resizable = lua_toboolean(L, 1);
        SDL_SetWindowResizable(_window, resizable);
    }
    lua_pushboolean(L, resizable);
    return 1;
}

static int l_poti_window_bordered(lua_State *L) {
    int bordered = 0;
    Uint32 flags = SDL_GetWindowFlags(_window);

    bordered = ~flags & SDL_WINDOW_BORDERLESS;
    if (!lua_isnil(L, 1)) {
        bordered = lua_toboolean(L, 1);
        SDL_SetWindowBordered(_window, bordered);
    }

    lua_pushboolean(L, bordered);
    return 1;
}

static int l_poti_window_border_size(lua_State *L) {

    int borders[4];
    SDL_GetWindowBordersSize(_window, &borders[0], &borders[1], &borders[2], &borders[3]);
    for (int i = 0; i < 4; i++) lua_pushinteger(L, borders[i]);

    return 4;
}

static int l_poti_window_maximize(lua_State *L) {
    SDL_MaximizeWindow(_window);
    return 0;
}

static int l_poti_window_minimize(lua_State *L) {
    SDL_MinimizeWindow(_window);
    return 0;
}

static int l_poti_window_restore(lua_State *L) {
    SDL_RestoreWindow(_window);
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
    if (flags == 0) return luaL_argerror(L, 1, "Invalid message box type");
    SDL_ShowSimpleMessageBox(flags, title, message, _window);

    return 0;
}

int luaopen_window(lua_State* L) {
    luaL_Reg reg[] = {
        {"init", l_poti_window_init},
        {"deinit", l_poti_window_deinit},
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
