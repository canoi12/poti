#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "poti.h"

#include "tea.h"
#include "mocha.h"

#define POINT_CLASS "Point"
#define RECT_CLASS "Rect"

#define TEXTURE_CLASS "Texture"
#define FONT_CLASS "Font"

#define AUDIO_CLASS "Audio"

struct Context {
    lua_State *L;
};

#define poti() (&_ctx)
struct Context _ctx;

static const char *boot_lua = "local traceback = debug.traceback\n"
"package.path = package.path .. ';' .. 'core/?.lua;core/?/init.lua'\n"
"local function _error(msg)\n"
"    trace = traceback('', 1)\n"
"    print(msg, trace)\n"
"end\n"
"local function _step()\n"
"    local dt = poti.delta()\n"
"    if poti.update then poti.update(dt) end\n"
"    if poti.draw then poti.draw() end\n"
"end\n"
"function poti._load()\n"
"    if poti.load then xpcall(poti.load, _error) end\n"
"end\n"
"function poti._step()\n"
"    xpcall(_step, _error)\n"
"end\n"
"function poti.run()\n"
"    local dt = poti.delta()\n"
"    if poti.load then poti.load() end\n"
"    while poti.running() do\n"
"        if poti.update then poti.update(dt) end\n"
"        if poti.draw then poti.draw() end\n"
"    end\n"
"    return 0\n"
"end\n"
"function poti.error()\n"
"end\n"
"xpcall(function() require 'main' end, _error)\n";

static int luaopen_poti(lua_State *L);
static int luaopen_point(lua_State *L);
static int luaopen_rect(lua_State *L);
static int luaopen_texture(lua_State *L);
static int luaopen_font(lua_State *L);
static int luaopen_shader(lua_State *L);
static int luaopen_audio(lua_State *L);

/* Core */
static int poti_ver(lua_State *L);
static int poti_delta(lua_State *L);

/* Point */
static int poti_point(lua_State *L);
static int poti_point_set(lua_State *L);
static int poti_point_x(lua_State *L);
static int poti_point_y(lua_State *L);

/* Rect */
static int poti_rect(lua_State *L);
static int poti_rect_set(lua_State *L);
static int poti_rect_x(lua_State *L);
static int poti_rect_y(lua_State *L);
static int poti_rect_width(lua_State *L);
static int poti_rect_height(lua_State *L);
static int poti_rect_pos(lua_State *L);
static int poti_rect_size(lua_State *L);

/* Texture */
static int poti_texture(lua_State *L);
static int poti_texture_draw(lua_State *L);
static int poti_texture_width(lua_State *L);
static int poti_texture_height(lua_State *L);
static int poti_texture_size(lua_State *L);
static int poti_texture__gc(lua_State *L);

/* Font */
static int poti_font(lua_State *L);
static int poti_font_print(lua_State *L);
static int poti_font__gc(lua_State *L);

/* Shader */
static int poti_shader(lua_State *L);
static int poti_shader_set(lua_State *L);
static int poti_shader_unset(lua_State *L);
static int poti_shader_send(lua_State *L);
static int poti_shader__gc(lua_State *L);

/* Audio */
static int poti_volume(lua_State *L);
static int poti_audio(lua_State *L);
static int poti_audio_play(lua_State *L);
static int poti_audio_stop(lua_State *L);
static int poti_audio_pause(lua_State *L);
static int poti_audio_playing(lua_State *L);
static int poti_audio_volume(lua_State *L);
static int poti_audio_pitch(lua_State *L);
static int poti_audio__gc(lua_State *L);

/* Draw */
static int poti_clear(lua_State *L);
static int poti_color(lua_State *L);
static int poti_mode(lua_State *L);
static int poti_set_target(lua_State *L);
static int poti_draw_point(lua_State *L);
static int poti_draw_line(lua_State *L);
static int poti_draw_rect(lua_State *L);
static int poti_draw_circ(lua_State *L);
static int poti_draw_tria(lua_State *L);
static int poti_print(lua_State *L);

/* Input */
static int poti_key_down(lua_State *L);
static int poti_key_up(lua_State *L);
static int poti_key_pressed(lua_State *L);
static int poti_key_released(lua_State *L);

static int poti_mouse_down(lua_State *L);
static int poti_mouse_up(lua_State *L);
static int poti_mouse_pressed(lua_State *L);
static int poti_mouse_released(lua_State *L);

int poti_init(int flags) {
    poti()->L = luaL_newstate();

    /*FILE *fp = fopen("core/boot.lua", "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open core/boot.lua\n");
        exit(0);
    }

    fseek(fp, 0, SEEK_END);
    unsigned int sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char buf[sz+1];
    fread(buf, sz, 1, fp);
    buf[sz] = '\0';

    fclose(fp);*/

    lua_State *L = poti()->L;

    luaL_openlibs(L);
    luaL_requiref(L, "poti", luaopen_poti, 1);

    char title[100];
    sprintf(title, "poti %s", POTI_VER);
    te_config_t conf = (te_config_t){{0}};
    conf = tea_config_init(title, 640, 380);

    tea_init(&conf);
    mo_init(0);
    // luaL_dostring(L, buf);
    if (luaL_dostring(L, boot_lua) != LUA_OK) {
	const char *error_buf = lua_tostring(L, -1);
	fprintf(stderr, "Failed to load Lua boot: %s\n", error_buf);
	exit(0);
    }

    lua_getglobal(L, "poti");
    if (!lua_isnil(L, -1)) {
        lua_getfield(L, -1, "_load");
        if (!lua_isnil(L, -1)) {
            int err = lua_pcall(L, 0, 0, 0);
            if (err) {
                const char *str = lua_tostring(L, -1);
                fprintf(stderr, "Lua error: %s", str);
                exit(0);
            }
        }
        lua_pop(L, 1);
        lua_settop(L, 1);
    }
    return 1;
}

int poti_deinit() {
    tea_deinit();
    mo_deinit();
    lua_close(poti()->L);
    return 1;
}

static int _poti_step(lua_State *L) {
    lua_getglobal(L, "poti");
    if (!lua_isnil(L, -1)) {
        lua_getfield(L, -1, "_step");
        if (!lua_isnil(L, -1)) {
            int err = lua_pcall(L, 0, 0, 0);
            if (err) {
                const char *str = lua_tostring(L, -1);
                fprintf(stderr, "Lua error: %s", str);
                exit(0);
            }
        }
        lua_pop(L, 1);
        lua_settop(L, 1);
    }
    return 1;
}

int poti_loop() {
    while (!tea_should_close()) {
        tea_update_input();
        tea_begin();
        _poti_step(poti()->L);
        tea_end();
    }

    return 1;
}

int luaopen_poti(lua_State *L) {
    luaL_Reg reg[] = {
        {"ver", poti_ver},
        {"delta", poti_delta},
        /* Draw */
        {"clear", poti_clear},
        {"mode", poti_mode},
        {"color", poti_color},
        {"target", poti_set_target},
        {"point", poti_draw_point},
        {"line", poti_draw_line},
        {"circle", poti_draw_circ},
        {"rectangle", poti_draw_rect},
        {"triangle", poti_draw_tria},
        /* Audio */
        {"volume", poti_volume},
        /* Types */
        {"Point", poti_point},
        {"Rect", poti_rect},
        {"Texture", poti_texture},
        {"Font", poti_font},
        {"Audio", poti_audio},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    struct { char *name; int(*fn)(lua_State*); } libs[] = {
        {"_Point", luaopen_point},
        {"_Rect", luaopen_rect},
        {"_Texture", luaopen_texture},
        {"_Font", luaopen_font},
        {"_Audio", luaopen_audio},
        {NULL, NULL}
    };

    int i;
    for (i = 0; libs[i].name; i++) {
        if (libs[i].fn) {
            libs[i].fn(L);
            lua_setfield(L, -2, libs[i].name);
        }
    }

    return 1;
}

int poti_ver(lua_State *L) {
    lua_pushstring(L, POTI_VER);
    return 1;
}

int poti_delta(lua_State *L) {
    lua_pushnumber(L, tea_delta());
    return 1;
}

/*********************************
 * Point
 *********************************/

int luaopen_point(lua_State *L) {
    luaL_Reg reg[] = {
        {"__call", poti_point_set},
        {"x", poti_point_x},
        {"y", poti_point_y},
        {NULL, NULL}
    };

    luaL_newmetatable(L, POINT_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

int poti_point(lua_State *L) {
    te_point_t *p = lua_newuserdata(L, sizeof(*p));
    luaL_setmetatable(L, POINT_CLASS);

    int top = lua_gettop(L) - 1;
    int i;
    TEA_TNUM *n = (TEA_TNUM*)p;
    for (i = 0; i < top; i++) n[i] = luaL_optnumber(L, i+1, 0);

    return 1;
}

int poti_point_set(lua_State *L) {
    te_point_t *p = luaL_checkudata(L, 1, POINT_CLASS);

    TEA_TNUM *n = (TEA_TNUM*)p;
    int top = lua_gettop(L);
    int i;
    for (i = 1; i < top; i++) n[i-1] = luaL_optnumber(L, i+1, n[i-1]);

    lua_pushnumber(L, p->x);
    lua_pushnumber(L, p->y);
    return 2;
}

int poti_point_x(lua_State *L) {
    te_point_t *p = luaL_checkudata(L, 1, POINT_CLASS);

    p->x = luaL_optnumber(L, 2, p->x);
    lua_pushnumber(L, p->x);

    return 1;
}

int poti_point_y(lua_State *L) {
    te_point_t *p = luaL_checkudata(L, 1, POINT_CLASS);

    p->y = luaL_optnumber(L, 2, p->y);
    lua_pushnumber(L, p->y);

    return 1;
}

/*********************************
 * Rect
 *********************************/

int luaopen_rect(lua_State *L) {
    luaL_Reg reg[] = {
        {"__call", poti_rect_set},
        {"x", poti_rect_x},
        {"y", poti_rect_y},
        {"width", poti_rect_width},
        {"height", poti_rect_height},
        {"pos", poti_rect_pos},
        {"size", poti_rect_size},
        {NULL, NULL}
    };

    luaL_newmetatable(L, RECT_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

int poti_rect(lua_State *L) {

    te_rect_t r;
    int args = lua_gettop(L);
    TEA_TNUM *n = (TEA_TNUM*)&r;
    for (int i = 0; i < args; i++) {
        n[i] = luaL_checknumber(L, i+1);
    }

    te_rect_t *rect = lua_newuserdata(L, sizeof(*rect));
    luaL_setmetatable(L, RECT_CLASS);
    memcpy(rect, &r, sizeof(*rect));

    return 1;
}

int poti_rect_set(lua_State *L) {
    te_rect_t *r = luaL_checkudata(L, 1, RECT_CLASS);

    int args = lua_gettop(L);
    TEA_TNUM *n = (TEA_TNUM*)r;
    for (int i = 1; i < args; i++) {
        n[i-1] = luaL_optnumber(L, i+1, n[i-1]);
    }

    return 0;
}

int poti_rect_x(lua_State *L) {
    te_rect_t *r = luaL_checkudata(L, 1, RECT_CLASS);

    TEA_TNUM x = luaL_optnumber(L, 2, r->x);

    r->x = x;
    lua_pushnumber(L, r->x);

    return 1;
}

int poti_rect_y(lua_State *L) {
    te_rect_t *r = luaL_checkudata(L, 1, RECT_CLASS);

    TEA_TNUM y = luaL_optnumber(L, 2, r->y);

    r->y = y;
    lua_pushnumber(L, r->y);

    return 1;
}

int poti_rect_width(lua_State *L) {
    te_rect_t *r = luaL_checkudata(L, 1, RECT_CLASS);

    TEA_TNUM width = luaL_optnumber(L, 2, r->w);

    r->w = width;
    lua_pushnumber(L, r->w);

    return 1;
}

int poti_rect_height(lua_State *L) {
    te_rect_t *r = luaL_checkudata(L, 1, RECT_CLASS);

    TEA_TNUM height = luaL_optnumber(L, 2, r->h);

    r->h = height;
    lua_pushnumber(L, r->h);

    return 1;
}

int poti_rect_pos(lua_State *L) {
    te_rect_t *r = luaL_checkudata(L, 1, RECT_CLASS);

    TEA_TNUM x, y;
    x = luaL_optnumber(L, 2, r->x);
    y = luaL_optnumber(L, 3, r->y);

    r->x = x;
    r->y = y;

    lua_pushnumber(L, r->x);
    lua_pushnumber(L, r->y);

    return 2;
}

int poti_rect_size(lua_State *L) {
    te_rect_t *r = luaL_checkudata(L, 1, RECT_CLASS);

    TEA_TNUM w, h;
    w = luaL_optnumber(L, 2, r->w);
    h = luaL_optnumber(L, 3, r->h);

    r->w = w;
    r->h = h;

    lua_pushnumber(L, r->w);
    lua_pushnumber(L, r->h);

    return 2;
}

/*********************************
 * Texture
 *********************************/

int luaopen_texture(lua_State *L) {
    luaL_Reg reg[] = {
        {"draw", poti_texture_draw},
        {"filter", NULL},
        {"wrap", NULL},
        {"width", poti_texture_width},
        {"height", poti_texture_height},
        {"size", poti_texture_size},
        {"__gc", poti_texture__gc},
        {NULL, NULL}
    };

    luaL_newmetatable(L, TEXTURE_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

static int _textype_from_string(lua_State *L, const char *name) {
    int usage = TEA_TEXTURE_STATIC;
    if (!strcmp(name, "stream")) usage = TEA_TEXTURE_STREAM;
    else if (!strcmp(name, "target")) usage = TEA_TEXTURE_TARGET;

    return usage;
}

static int _texture_from_path(lua_State *L, te_texture_t **tex) {
    size_t size;
    int top = lua_gettop(L)-1;
    const char *path = luaL_checklstring(L, 1, &size);
    const char *s_usage = "static";
    if (top > 1 && lua_type(L, top) == LUA_TSTRING)
        s_usage = luaL_checkstring(L, top);

    int usage = _textype_from_string(L, s_usage);
    *tex = tea_texture_load(path, usage);
    if (!*tex) {
        fprintf(stderr, "Failed to load texture: %s\n", tea_geterror());
        exit(0);
    }

    return 1;
}

static int _texture_from_size(lua_State *L, te_texture_t **tex) {
    int top = lua_gettop(L)-1;
    TEA_TNUM w, h;
    w = luaL_checknumber(L, 1);
    h = luaL_checknumber(L, 2);
    
    const char *s_usage = "static";
    if (top > 1 && lua_type(L, top) == LUA_TSTRING)
        s_usage = luaL_checkstring(L, top);

    int usage = _textype_from_string(L, s_usage);
    *tex = tea_texture(NULL, w, h, TEA_RGBA, usage);
    if (!*tex) {
        fprintf(stderr, "Failed to load texture: %s\n", tea_geterror());
        exit(0);
    }

    return 1;
}

int poti_texture(lua_State *L) {
    te_texture_t **tex = lua_newuserdata(L, sizeof(*tex));
    luaL_setmetatable(L, TEXTURE_CLASS);

    switch(lua_type(L, 1)) {
        case LUA_TSTRING: _texture_from_path(L, tex); break;
        case LUA_TNUMBER: _texture_from_size(L, tex); break;
    }
    
    return 1;
}


int poti_texture_draw(lua_State *L) {
    te_texture_t **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);
    int top = lua_gettop(L);
    // if (top < 2) tea_texture_draw(*tex, NULL, NULL);

    te_texinfo_t info;
    tea_texture_info(*tex, &info);
    
    te_rect_t *d = luaL_testudata(L, 2, RECT_CLASS);
    te_rect_t *s = luaL_testudata(L, 3, RECT_CLASS);

    if (top > 3) {
        TEA_TNUM angle = luaL_optnumber(L, 4, 0); 
        te_point_t ori = TEA_POINT(0, 0);

        te_point_t *origin = luaL_testudata(L, 5, POINT_CLASS);
        if (origin) memcpy(&ori, origin, sizeof(*origin));
        int flip = luaL_optnumber(L, 6, 0);

        tea_texture_draw_ex(*tex, d, s, angle, &ori, flip);
    } else {
        tea_texture_draw(*tex, d, s);
    }

    return 0;
}

int poti_texture_width(lua_State *L) {
    te_texture_t **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);

    te_texinfo_t info;
    tea_texture_info(*tex, &info);

    lua_pushnumber(L, info.size.w);
    return 1;
}

int poti_texture_height(lua_State *L) {
    te_texture_t **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);

    te_texinfo_t info;
    tea_texture_info(*tex, &info);

    lua_pushnumber(L, info.size.h);
    return 1;
}

int poti_texture_size(lua_State *L) {
    te_texture_t **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);

    te_texinfo_t info;
    tea_texture_info(*tex, &info);

    lua_pushnumber(L, info.size.w);
    lua_pushnumber(L, info.size.h);
    return 2;
}

int poti_texture__gc(lua_State *L) {
    return 0;
}

/*********************************
 * Font
 *********************************/

int luaopen_font(lua_State *L) {
    luaL_Reg reg[] = {
        {"print", poti_font_print},
        {"__gc", poti_font__gc},
        {NULL, NULL}   
    };

    luaL_newmetatable(L, FONT_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

int poti_font(lua_State *L) {
   const char *path = luaL_checkstring(L, 1); 
   int size = luaL_optnumber(L, 2, 16);

   te_font_t **font = lua_newuserdata(L, sizeof(*font));
   luaL_setmetatable(L, FONT_CLASS);
   *font = tea_font_load(path, size);

   return 1;
}

int poti_font_print(lua_State *L) {
    te_font_t **font = luaL_checkudata(L, 1, FONT_CLASS);
    const char *text = luaL_optstring(L, 2, "");
    TEA_TNUM x, y;
    x = luaL_optnumber(L, 3, 0);
    y = luaL_optnumber(L, 4, 0);

    tea_font_print(*font, text, x, y);

    return 0;
}

int poti_font__gc(lua_State *L) {
    te_font_t **font = luaL_checkudata(L, 1, FONT_CLASS);
    // tea_destroy_font(*font);
    return 0;
}

/*********************************
 * Audio
 *********************************/

int luaopen_audio(lua_State *L) {
    luaL_Reg reg[] = {
        {"play", poti_audio_play},
        {"stop", poti_audio_stop},
        {"pause", poti_audio_pause},
        {"playing", poti_audio_playing},
        {"volume", poti_audio_volume},
        {"pitch", poti_audio_pitch},
        {"__gc", poti_audio__gc},
        {NULL, NULL}
    };

    luaL_newmetatable(L, AUDIO_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

int poti_audio(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const char *s_usage = luaL_optstring(L, 1, "stream");

    mo_audio_t **buf = lua_newuserdata(L, sizeof(*buf));
    luaL_setmetatable(L, AUDIO_CLASS);
    int usage = MO_AUDIO_STREAM;

    if (!strcmp(s_usage, "static")) usage = MO_AUDIO_STATIC; 


    *buf = mo_audio_load(path, usage);
    return 1;
}

int poti_volume(lua_State *L) {
    float volume = luaL_optnumber(L, 1, 0);
    mo_volume(NULL, volume);

    return 0;
}

int poti_audio_play(lua_State *L) {
    mo_audio_t **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    mo_play(*buf);
    return 0;
}

int poti_audio_stop(lua_State *L) {
    mo_audio_t **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    mo_stop(*buf);
    return 0;
}

int poti_audio_pause(lua_State *L) {
    mo_audio_t **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    mo_pause(*buf);
    return 0;
}

int poti_audio_playing(lua_State *L) {
    mo_audio_t **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    lua_pushboolean(L, mo_is_playing(*buf));
    return 1;
}

int poti_audio_volume(lua_State *L) {
    mo_audio_t **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    float volume = luaL_optnumber(L, 2, 0);
    mo_volume(*buf, volume);
    return 0;
}

int poti_audio_pitch(lua_State *L) { return 0; }

int poti_audio__gc(lua_State *L) {
    mo_audio_t **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    mo_audio_destroy(*buf); 
    return 0;
}



/*********************************
 * Draw
 *********************************/

int poti_clear(lua_State *L) {
    te_color_t color = TEA_COLOR(0, 0, 0, 255);
    unsigned char *c = (unsigned char*)&color;
    int args = lua_gettop(L);

    for (int i = 0; i < args; i++) {
        c[i] = lua_tonumber(L, i+1);
    }
    tea_clear(color);
    return 0;
}

int poti_mode(lua_State *L) {
    int mode = luaL_checknumber(L, 1);
    tea_mode(mode);
    return 0;
}

int poti_color(lua_State *L) {
    int args = lua_gettop(L);

    unsigned char col[4] = {0, 0, 0, 255};
    for (int i = 0; i < args; i++) { 
        col[i] = luaL_checknumber(L, i+1);
    }
    tea_color(TEA_COLOR(col[0], col[1], col[2], col[3]));

    return 0;
}

int poti_set_target(lua_State *L) {
    te_texture_t **tex = luaL_testudata(L, 1, TEXTURE_CLASS);
    te_texture_t *t = NULL;
    if (tex) t = *tex;

    tea_set_target(t);
    return 0;
}

int poti_draw_point(lua_State *L) {
    TEA_TNUM x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);

    tea_point(x, y);
    return 0;
}

int poti_draw_line(lua_State *L) {
    te_point_t p0, p1;
    p0.x = luaL_checknumber(L, 1);
    p0.y = luaL_checknumber(L, 2);

    p1.x = luaL_checknumber(L, 3);
    p1.y = luaL_checknumber(L, 4);

    tea_line(p0.x, p0.y, p1.x, p1.y);
    return 0;
}

int poti_draw_circ(lua_State *L) {

    TEA_TNUM x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);

    TEA_TNUM radius = luaL_checknumber(L, 3);

    tea_circle(x, y, radius);

    return 0;
}

int poti_draw_rect(lua_State *L) {

    te_rect_t r = {0, 0, 0, 0};

    r.x = luaL_checknumber(L, 1);
    r.y = luaL_checknumber(L, 2);
    r.w = luaL_checknumber(L, 3);
    r.h = luaL_checknumber(L, 4);

    tea_rect(r.x, r.y, r.w, r.h);
    
    return 0;
}

int poti_draw_tria(lua_State *L) {

    TEA_TNUM points[6];

    for (int i = 0; i < 6; i++) {
        points[i] = luaL_checknumber(L, i+1);
    }

        tea_triangle(points[0], points[1], points[2], points[3], points[4], points[5]);

    return 0;
}

/*********************************
 * Input
 *********************************/

int luaopen_keyboard(lua_State *L) {
    luaL_Reg reg[] = {
        {"down", poti_key_down},
        {"up", poti_key_up},
        {"pressed", poti_key_pressed},
        {"released", poti_key_released},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    return 1;
}

int luaopen_mouse(lua_State *L) {
    luaL_Reg reg[] = {
        {"down", poti_mouse_down},
        {"up", poti_mouse_up},
        {"pressed", poti_mouse_pressed},
        {"released", poti_mouse_released},
        {NULL, NULL}
    };
    
    luaL_newlib(L, reg);

    return 1;
}

int poti_key_down(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = tea_key_from_name(key);
    lua_pushboolean(L, tea_key_down(code));
    
    return 1;
}

int poti_key_up(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = tea_key_from_name(key);
    lua_pushboolean(L, tea_key_up(code));
    
    return 1;
}

int poti_key_pressed(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = tea_key_from_name(key);
    lua_pushboolean(L, tea_key_pressed(code));
    
    return 1;
}

int poti_key_released(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = tea_key_from_name(key);
    lua_pushboolean(L, tea_key_released(code));
    
    return 1;
}

int poti_mouse_down(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, tea_mouse_down(button));
    
    return 1;
}

int poti_mouse_up(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, tea_mouse_up(button));
    
    return 1;
}

int poti_mouse_pressed(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, tea_mouse_pressed(button));
    
    return 1;
}

int poti_mouse_released(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, tea_mouse_released(button));
    
    return 1;
}
