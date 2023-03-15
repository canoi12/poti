#include "poti.h"
#if !defined(POTI_NO_EVENT)

static Event _event;

static int l_event_reg;
static int l_polltable_reg;
static int table_index = 1;

int l_poti_callback_quit(lua_State* L) {
    lua_pushstring(L, "quit");
    return 1;
}

int l_poti_callback_keypressed(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushboolean(L, event->key.repeat);
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "lower");
    lua_remove(L, -2);
    lua_pushstring(L, SDL_GetKeyName(event->key.keysym.sym));
    lua_call(L, 1, 1);
    lua_pushstring(L, "key_pressed");
    return 3;
}

int l_poti_callback_keyreleased(lua_State* L) {
    SDL_Event* event = &_event;
    lua_pushboolean(L, event->key.repeat);
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "lower");
    lua_remove(L, -2);
    lua_pushstring(L, SDL_GetKeyName(event->key.keysym.sym));
    lua_call(L, 1, 1);
    lua_pushstring(L, "key_released");
    return 3;
}

int l_poti_callback_mousemotion(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->motion.state);
    lua_pushnumber(L, event->motion.yrel);
    lua_pushnumber(L, event->motion.xrel);
    lua_pushnumber(L, event->motion.y);
    lua_pushnumber(L, event->motion.x);
    lua_pushstring(L, "mouse_motion");
    return 6;
}

int l_poti_callback_mousebutton(lua_State* L) {
    SDL_Event *e = &_event;
    lua_pushnumber(L, e->button.clicks);
    lua_pushboolean(L, e->button.state);
    lua_pushnumber(L, e->button.button);
    lua_pushstring(L, "mouse_button");
    return 4;
}

int l_poti_callback_mousewheel(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->wheel.y);
    lua_pushnumber(L, event->wheel.x);
    lua_pushstring(L, "mouse_wheel");
    return 3;
}

int l_poti_callback_joyaxismotion(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->jaxis.value);
    lua_pushnumber(L, event->jaxis.axis);
    lua_pushinteger(L, event->jaxis.which);
    lua_pushstring(L, "joy_axismotion");
    return 4;
}

int l_poti_callback_joyballmotion(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->jball.yrel);
    lua_pushnumber(L, event->jball.xrel);
    lua_pushnumber(L, event->jball.ball);
    lua_pushinteger(L, event->jball.which);
    lua_pushstring(L, "joy_ballmotion");
    return 5;
}

int l_poti_callback_joyhatmotion(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->jhat.value);
    lua_pushnumber(L, event->jhat.hat);
    lua_pushinteger(L, event->jhat.which);
    lua_pushstring(L, "joy_hatmotion");
    return 4;
}

int l_poti_callback_joybutton(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushboolean(L, event->jbutton.state);
    lua_pushnumber(L, event->jbutton.button);
    lua_pushnumber(L, event->jbutton.which);
    lua_pushstring(L, "joy_button");
    return 4;
}

int l_poti_callback_joydevice(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushboolean(L, SDL_JOYDEVICEREMOVED - event->jdevice.type);
    lua_pushinteger(L, event->jdevice.which);
    lua_pushstring(L, "joy_device");
    return 3;
}

int l_poti_callback_gamepadaxismotion(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->caxis.value);
    lua_pushstring(L, SDL_GameControllerGetStringForAxis(event->caxis.axis));
    lua_pushinteger(L, event->caxis.which);
    lua_pushstring(L, "gpad_axismotion");
    return 4;
}

int l_poti_callback_gamepadbutton(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushboolean(L, event->cbutton.state);
    lua_pushstring(L, SDL_GameControllerGetStringForButton(event->cbutton.button));
    lua_pushinteger(L, event->cbutton.which);
    lua_pushstring(L, "gpad_button");
    return 4;
}

int l_poti_callback_gamepaddevice(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushboolean(L, SDL_CONTROLLERDEVICEREMOVED - event->cdevice.type);
    lua_pushinteger(L, event->cdevice.which);
    lua_pushstring(L, "gpad_device");
    return 3;
}

int l_poti_callback_gamepadremap(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushstring(L, SDL_GameControllerNameForIndex(event->cdevice.which));
    lua_pushinteger(L, event->cdevice.which);
    lua_pushstring(L, "gpad_remap");
    return 3;
}


#if SDL_VERSION_ATLEAST(2, 0, 14)
int l_poti_callback_gamepadtouchpad(lua_State* L) {
// SDL_Event *event = &_event;
    return 0;
}

int l_poti_callback_gamepadtouchpadmotion(lua_State* L) {

    return 0;
}
#endif

static int l_poti_callback_finger(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->tfinger.pressure);
    lua_pushnumber(L, event->tfinger.dy);
    lua_pushnumber(L, event->tfinger.dx);
    lua_pushnumber(L, event->tfinger.y);
    lua_pushnumber(L, event->tfinger.x);
    lua_pushinteger(L, event->tfinger.fingerId);
    lua_pushboolean(L, SDL_FINGERUP - event->type);
    lua_pushinteger(L, event->tfinger.touchId);
    lua_pushstring(L, "touchpad");
    return 9;
}

static int l_poti_callback_fingermotion(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->tfinger.pressure);
    lua_pushnumber(L, event->tfinger.dy);
    lua_pushnumber(L, event->tfinger.dx);
    lua_pushnumber(L, event->tfinger.y);
    lua_pushnumber(L, event->tfinger.x);
    lua_pushinteger(L, event->tfinger.fingerId);
    lua_pushinteger(L, event->tfinger.touchId);
    lua_pushstring(L, "touch_motion");
    return 8;
}

static int l_poti_callback_dollargesture(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->dgesture.y);
    lua_pushnumber(L, event->dgesture.x);
    lua_pushnumber(L, event->dgesture.error);
    lua_pushinteger(L, event->dgesture.numFingers);
    lua_pushinteger(L, event->dgesture.gestureId);
    lua_pushinteger(L, event->dgesture.touchId);
    lua_pushstring(L, "dollar_gesture");
    return 7;
}

static int l_poti_callback_dollarrecord(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->dgesture.y);
    lua_pushnumber(L, event->dgesture.x);
    lua_pushnumber(L, event->dgesture.error);
    lua_pushinteger(L, event->dgesture.numFingers);
    lua_pushinteger(L, event->dgesture.gestureId);
    lua_pushinteger(L, event->dgesture.touchId);
    lua_pushstring(L, "dollar_record");
    return 7;
}

int l_poti_callback_multigesture(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->mgesture.dDist);
    lua_pushnumber(L, event->mgesture.dTheta);
    lua_pushnumber(L, event->mgesture.y);
    lua_pushnumber(L, event->mgesture.x);
    lua_pushinteger(L, event->mgesture.numFingers);
    lua_pushinteger(L, event->mgesture.touchId);
    lua_pushstring(L, "multigesture");
    return 7;
}

int l_poti_callback_clipboardupdate(lua_State* L) {
    // SDL_Event *event = &_event;
    char *text = SDL_GetClipboardText();
    lua_pushstring(L, text);
    SDL_free(text);
    lua_pushstring(L, "clipboard_update");
    return 2;
}

int l_poti_callback_dropfile(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushstring(L, event->drop.file);
    lua_pushinteger(L, event->drop.windowID);
    lua_pushstring(L, "drop_file");
    return 2;
}

int l_poti_callback_droptext(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushstring(L, event->drop.file);
    lua_pushinteger(L, event->drop.windowID);
    lua_pushstring(L, "drop_text");
    return 2;
}

int l_poti_callback_dropbegin(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushinteger(L, event->drop.windowID);
    lua_pushstring(L, "drop_begin");
    return 2;
}

int l_poti_callback_dropcomplete(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushinteger(L, event->drop.windowID);
    lua_pushstring(L, "drop_complete");
    return 2;
}

int l_poti_callback_audiodevice(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushboolean(L, event->adevice.iscapture);
    lua_pushboolean(L, SDL_AUDIODEVICEREMOVED - event->type);
    lua_pushnumber(L, event->adevice.which);
    lua_pushstring(L, "audio_device");
    return 4;
}

/* Window Events */
static int poti_windowevent_null(lua_State* L) {
    lua_pushstring(L, "none");
    return 1;
}

int l_poti_callback_window_moved(lua_State* L) {
    SDL_Event* event = &_event;
    lua_pushnumber(L, event->window.data2);
    lua_pushnumber(L, event->window.data1);
    lua_pushnumber(L, event->window.windowID);
    lua_pushstring(L, "window_moved");
    return 4;
}

int l_poti_callback_window_resized(lua_State* L) {
    SDL_Event* event = &_event;
    lua_pushnumber(L, event->window.data2);
    lua_pushnumber(L, event->window.data1);
    lua_pushnumber(L, event->window.windowID);
    lua_pushstring(L, "window_resized");
    return 4;
}

int l_poti_callback_window_minimized(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->window.windowID);
    lua_pushstring(L, "window_minimized");
    return 2;
}

int l_poti_callback_window_maximized(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->window.windowID);
    lua_pushstring(L, "window_maximized");
    return 2;
}

int l_poti_callback_window_restored(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->window.windowID);
    lua_pushstring(L, "window_restored");
    return 2;
}

int l_poti_callback_window_shown(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->window.windowID);
    lua_pushstring(L, "window_shown");
    return 2;
}

int l_poti_callback_window_hidden(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->window.windowID);
    lua_pushstring(L, "window_hidden");
    return 2;
}

int l_poti_callback_window_mouse(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushboolean(L, SDL_WINDOWEVENT_LEAVE - event->window.event);
    lua_pushnumber(L, event->window.windowID);
    lua_pushstring(L, "window_mouse");
    return 3;
}

int l_poti_callback_window_focus(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushboolean(L, SDL_WINDOWEVENT_FOCUS_LOST - event->window.event);
    lua_pushnumber(L, event->window.windowID);
    lua_pushstring(L, "window_focus");
    return 3;
}

int l_poti_callback_textinput(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushstring(L, event->text.text);
    lua_pushnumber(L, event->text.windowID);
    lua_pushstring(L, "text_input");
    return 3;
}

int l_poti_callback_textediting(lua_State* L) {
    SDL_Event *event = &_event;
    lua_pushnumber(L, event->edit.length);
    lua_pushnumber(L, event->edit.start);
    lua_pushstring(L, event->edit.text);
    lua_pushnumber(L, event->edit.windowID);
    lua_pushstring(L, "text_edit");
    return 5;
}

static const lua_CFunction window_events[] = {
    [SDL_WINDOWEVENT_NONE] = poti_windowevent_null,
    [SDL_WINDOWEVENT_SHOWN] = l_poti_callback_window_shown,
    [SDL_WINDOWEVENT_HIDDEN] = l_poti_callback_window_hidden,
    [SDL_WINDOWEVENT_EXPOSED] = poti_windowevent_null,
    [SDL_WINDOWEVENT_MOVED] = l_poti_callback_window_moved,
    [SDL_WINDOWEVENT_RESIZED] = l_poti_callback_window_resized,
    [SDL_WINDOWEVENT_SIZE_CHANGED] = l_poti_callback_window_resized,
    [SDL_WINDOWEVENT_MINIMIZED] = l_poti_callback_window_minimized,
    [SDL_WINDOWEVENT_MAXIMIZED] = l_poti_callback_window_maximized,
    [SDL_WINDOWEVENT_RESTORED] = l_poti_callback_window_restored,
    [SDL_WINDOWEVENT_ENTER] = l_poti_callback_window_mouse,
    [SDL_WINDOWEVENT_LEAVE] = l_poti_callback_window_mouse,
    [SDL_WINDOWEVENT_FOCUS_GAINED] = l_poti_callback_window_focus,
    [SDL_WINDOWEVENT_FOCUS_LOST] = l_poti_callback_window_focus,
    [SDL_WINDOWEVENT_CLOSE] = poti_windowevent_null,
    [SDL_WINDOWEVENT_TAKE_FOCUS] = poti_windowevent_null,
    [SDL_WINDOWEVENT_HIT_TEST] = poti_windowevent_null,
#if SDL_VERSION_ATLEAST(2, 0, 22)
    [SDL_WINDOWEVENT_ICCPROF_CHANGED] = poti_windowevent_null,
    [SDL_WINDOWEVENT_DISPLAY_CHANGED] = poti_windowevent_null,
#endif
};

int l_poti_callback_windowevent(lua_State *L) {
    SDL_Event* event = &_event;
    return window_events[event->window.event](L);
}

static int s_poll(lua_State* L) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_polltable_reg);
    lua_rawgeti(L, -1, table_index);
    lua_remove(L, -2);
    int top = lua_gettop(L);
    int ret = lua_rawlen(L, -1);
    int i;
    for (i = 0; i < ret; i++) {
	lua_rawgeti(L, top, i+1);
    }
    lua_remove(L, top);
    table_index++;
    return ret;
}

static int l_poti_event_poll(lua_State* L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &l_polltable_reg);
    int poll_top = lua_gettop(L);
    int index = 1;
    table_index = 1;
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_event_reg);
    while (SDL_PollEvent(&_event)) {
        lua_rawgeti(L, -1, _event.type);
        if (!lua_isnil(L, -1)) {
            lua_CFunction func = lua_tocfunction(L, -1);
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_rawseti(L, poll_top, index);
            index++;
            int top = lua_gettop(L);
            int args = func(L);
            // fprintf(stderr, "Teste(%d): %s %d\n", _event.type, lua_tostring(L, top+1), args);
            int i;
            for (i = 0; i < args; i++) lua_rawseti(L, top, i+1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_pushcfunction(L, s_poll);
    return 1;
}

static int l_poti_event_pump(lua_State* L) {
    return 0;
}

int luaopen_event(lua_State* L) {
    lua_newtable(L);

    struct {
        int type;
        lua_CFunction fn;
    } fn[] = {
        {SDL_QUIT, l_poti_callback_quit},
        {SDL_KEYDOWN, l_poti_callback_keypressed},
        {SDL_KEYUP, l_poti_callback_keyreleased},
        {SDL_MOUSEMOTION, l_poti_callback_mousemotion},
        {SDL_MOUSEBUTTONDOWN, l_poti_callback_mousebutton},
        {SDL_MOUSEBUTTONUP, l_poti_callback_mousebutton},
        {SDL_MOUSEWHEEL, l_poti_callback_mousewheel},
        {SDL_JOYAXISMOTION, l_poti_callback_joyaxismotion},
        {SDL_JOYBALLMOTION, l_poti_callback_joyballmotion},
        {SDL_JOYHATMOTION, l_poti_callback_joyhatmotion},
        {SDL_JOYBUTTONDOWN, l_poti_callback_joybutton},
        {SDL_JOYBUTTONUP, l_poti_callback_joybutton},
        {SDL_JOYDEVICEADDED, l_poti_callback_joydevice},
        {SDL_JOYDEVICEREMOVED, l_poti_callback_joydevice},
        {SDL_CONTROLLERAXISMOTION, l_poti_callback_gamepadaxismotion},
        {SDL_CONTROLLERBUTTONDOWN, l_poti_callback_gamepadbutton},
        {SDL_CONTROLLERBUTTONUP, l_poti_callback_gamepadbutton},
        {SDL_CONTROLLERDEVICEADDED, l_poti_callback_gamepaddevice},
        {SDL_CONTROLLERDEVICEREMOVED, l_poti_callback_gamepaddevice},
        {SDL_CONTROLLERDEVICEREMAPPED, l_poti_callback_gamepadremap},
#if SDL_VERSION_ATLEAST(2, 0, 14)
        {SDL_CONTROLLERTOUCHPADDOWN, l_poti_callback_gamepadtouchpad},
        {SDL_CONTROLLERTOUCHPADUP, l_poti_callback_gamepadtouchpad},
        {SDL_CONTROLLERTOUCHPADMOTION, l_poti_callback_gamepadtouchpadmotion},
#endif
        {SDL_FINGERDOWN, l_poti_callback_finger},
        {SDL_FINGERUP, l_poti_callback_finger},
        {SDL_FINGERMOTION, l_poti_callback_fingermotion},
        {SDL_DOLLARGESTURE, l_poti_callback_dollargesture},
        {SDL_DOLLARRECORD, l_poti_callback_dollarrecord},
        {SDL_MULTIGESTURE, l_poti_callback_multigesture},
        {SDL_CLIPBOARDUPDATE, l_poti_callback_clipboardupdate},
        {SDL_DROPFILE, l_poti_callback_dropfile},
        {SDL_DROPTEXT, l_poti_callback_droptext},
        {SDL_DROPBEGIN, l_poti_callback_dropbegin},
        {SDL_DROPCOMPLETE, l_poti_callback_dropcomplete},
        {SDL_AUDIODEVICEADDED, l_poti_callback_audiodevice},
        {SDL_AUDIODEVICEREMOVED, l_poti_callback_audiodevice},
        {SDL_WINDOWEVENT, l_poti_callback_windowevent},
        {SDL_TEXTINPUT, l_poti_callback_textinput},
        {SDL_TEXTEDITING, l_poti_callback_textediting},
        {-1, NULL}
    };

    int i;
    for (i = 0; fn[i].type != -1; i++) {
        lua_pushinteger(L, fn[i].type);
        lua_pushcfunction(L, fn[i].fn);
        lua_settable(L, -3);
    }
    lua_rawsetp(L, LUA_REGISTRYINDEX, &l_event_reg);

    luaL_Reg reg[] = {
        {"poll", l_poti_event_poll},
        {"pump", l_poti_event_pump},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}
#endif
