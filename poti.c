#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define POTI_VER "0.1.0"

#ifdef _WIN32
    #define SDL_MAIN_HANDLED
#ifdef __MINGW32__
    #include <SDL2/SDL.h>
#else
    #include <SDL.h>
#endif
#else
    #include <SDL2/SDL.h>
#endif

#include "mocha.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define SBTAR_IMPLEMENTATION
#include "sbtar.h"

#include "poti.h"

#define POINT_CLASS "Point"
#define RECT_CLASS "Rect"

#define TEXTURE_CLASS "Texture"
#define FONT_CLASS "Font"

#define AUDIO_CLASS "Audio"

#define JOYSTICK_CLASS "Joystick"
#define GAMEPAD_CLASS "GamePad"

#define poti() (&_ctx)

typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long i64;
typedef unsigned long u64;
typedef float f32;
typedef double f64;

typedef u8 color_t[4];

typedef SDL_Texture Texture;
typedef struct Font Font;
typedef mo_audio_t Audio;

typedef SDL_Window Window;
typedef SDL_Renderer Render;
typedef SDL_Event Event;

typedef SDL_Joystick Joystick;
typedef SDL_GameController GameController;

struct Font {
    struct {
        // advance x and y
        i32 ax, ay;
        //bitmap width and rows
        i32 bw, bh;
        // bitmap left and top
        i32 bl, bt;
        // x offset of glyph
        i32 tx;
    } c[256];

    Texture *tex;
    u8 size;

    stbtt_fontinfo info;
    f32 ptsize;
    f32 scale;
    i32 baseline;
    i32 height;
    void *data;
};

struct Context {
    lua_State *L;
    Window *window;
    Render *render;
    Event event;

    f64 last_time;
    f64 delta;
    i32 fps, frames;

    const Uint8 *keys;
    color_t color;
    i8 draw_mode;
    sbtar_t pkg;

    i8 is_packed;
    i8*(*read_file)(const i8*, size_t*);

    Font default_font;
    Font *font;
    i32(*init_font)(Font*, const void*, size_t, i32);
    Font*(*load_font)(const i8*, i32);
    u8*(*utf8_codepoint)(u8*, i32*);

    int(*flip_from_table)(lua_State*, i32, i32*);
    int(*point_from_table)(lua_State*, i32, SDL_Point*);
    int(*rect_from_table)(lua_State*, i32, SDL_Rect*);
};

struct Context _ctx;
static const i8 *boot_lua = "local traceback = debug.traceback\n"
"package.path = package.path .. ';core/?.lua;core/?/init.lua'\n"
"local function _err(msg)\n"
"   local trace = traceback('', 1)\n"
"   poti.error(msg, trace)\n"
"   print(msg, trace)\n"
"end\n"
"function poti.error(msg, trace)\n"
"   poti.update = function() end\n"
"   poti.draw = function()\n" 
"       poti.print('error', 32)\n"
"       poti.print(msg, 32, 16)\n"
"       poti.print(trace, 32, 32)\n"
"   end\n"
"end\n"
"function poti.run()\n"
"   if poti.load then poti.load() end\n"
"   local delta = 0\n"
"   return function()\n"
"       dt = poti.delta()\n"
"       if poti.update then poti.update(dt) end\n"
"       if poti.draw then poti.draw() end\n"
"   end\n"
"end\n"
"xpcall(function() require 'main' end, _err)";

#if 0
static const char *boot_lua = "local traceback = debug.traceback\n"
"package.path = package.path .. ';core/?.lua;core/?/init.lua'\n"
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
#endif

static int poti_init(void);
static int poti_loop(void);
static int poti_quit(void);

static int luaopen_poti(lua_State *L);
static int luaopen_texture(lua_State *L);
static int luaopen_font(lua_State *L);
static int luaopen_audio(lua_State *L);
static int luaopen_joystick(lua_State *L);
static int luaopen_gamepad(lua_State *L);

/* Core */
static int poti_ver(lua_State *L);
static int poti_delta(lua_State *L);

/*=================================*
 *              Audio              *
 *=================================*/
static int poti_volume(lua_State *L);
static int poti_new_audio(lua_State *L);
static int poti_audio_play(lua_State *L);
static int poti_audio_stop(lua_State *L);
static int poti_audio_pause(lua_State *L);
static int poti_audio_playing(lua_State *L);
static int poti_audio_volume(lua_State *L);
static int poti_audio_pitch(lua_State *L);
static int poti_audio__gc(lua_State *L);

/*=================================*
 *           Filesystem            *
 *=================================*/
static int poti_file(lua_State *L);
static int poti_file_read(lua_State *L);
static int poti_file_write(lua_State *L);
static int poti_file_seek(lua_State *L);
static int poti_file_tell(lua_State *L);
static int poti_list_folder(lua_State *L);

/*=================================*
 *            Graphics             *
 *=================================*/

/* Texture */
static int poti_new_texture(lua_State *L);
static int poti_texture_draw(lua_State *L);
static int poti_texture_width(lua_State *L);
static int poti_texture_height(lua_State *L);
static int poti_texture_size(lua_State *L);
static int poti_texture__gc(lua_State *L);

/* Font */
static int poti_new_font(lua_State *L);
static int poti_font_print(lua_State *L);
static int poti_font__gc(lua_State *L);


/* Draw */
static int poti_clear(lua_State *L);
static int poti_color(lua_State *L);
static int poti_mode(lua_State *L);
static int poti_set_target(lua_State *L);
static int poti_draw_point(lua_State *L);
static int poti_draw_line(lua_State *L);
static int poti_draw_rectangle(lua_State *L);
static int poti_draw_circle(lua_State *L);
static int poti_draw_triangle(lua_State *L);
static int poti_print(lua_State *L);

/*=================================*
 *              Input              *
 *=================================*/

/* Keyboard */
static int poti_key_down(lua_State *L);
static int poti_key_up(lua_State *L);
static int poti_key_pressed(lua_State *L);
static int poti_key_released(lua_State *L);

/* Mouse */
static int poti_mouse_pos(lua_State *L);
static int poti_mouse_down(lua_State *L);
static int poti_mouse_up(lua_State *L);
static int poti_mouse_pressed(lua_State *L);
static int poti_mouse_released(lua_State *L);

/* Joystick */
static int poti_joystick(lua_State* L);
static int poti_num_joysticks(lua_State *L);
// static int poti_joystick_opened(lua_State *L);

static int poti_joystick_name(lua_State *L);
static int poti_joystick_vendor(lua_State *L);
static int poti_joystick_product(lua_State *L);
static int poti_joystick_product_version(lua_State *L);
static int poti_joystick_type(lua_State *L);
static int poti_joystick_num_axes(lua_State *L);
static int poti_joystick_num_balls(lua_State *L);
static int poti_joystick_num_hats(lua_State *L);
static int poti_joystick_num_buttons(lua_State *L);
static int poti_joystick_button(lua_State *L);
static int poti_joystick_axis(lua_State *L);
static int poti_joystick_hat(lua_State *L);
static int poti_joystick_ball(lua_State *L);
static int poti_joystick_rumble(lua_State *L);
static int poti_joystick_powerlevel(lua_State *L);
static int poti_joystick_close(lua_State *L);
static int poti_joystick__gc(lua_State *L);

/* Game Controller */
static int poti_gamepad(lua_State *L);
static int poti_is_gamepad(lua_State *L);
static int poti_gamepad_name(lua_State *L);
static int poti_gamepad_vendor(lua_State *L);
static int poti_gamepad_product(lua_State *L);
static int poti_gamepad_product_version(lua_State *L);

static int poti_gamepad_axis(lua_State *L);
static int poti_gamepad_button(lua_State *L);
static int poti_gamepad_rumble(lua_State *L);

static int poti_gamepad_close(lua_State *L);
static int poti_gamepad__gc(lua_State *L);

static int s_setup_function_ptrs(struct Context *ctx);

int poti_init(void) {
    poti()->L = luaL_newstate();

#if 0
    FILE *fp = fopen("boot.lua", "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open boot.lua\n");
        exit(0);
    }

    fseek(fp, 0, SEEK_END);
    unsigned int sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char buf[sz+1];
    fread(buf, sz, 1, fp);
    buf[sz] = '\0';

    fclose(fp);
#endif

    lua_State *L = poti()->L;

    luaL_openlibs(L);
    luaL_requiref(L, "poti", luaopen_poti, 1);

    char title[100];
#ifdef _WIN32
    sprintf_s(title, 100, "poti %s", POTI_VER);
#else
    sprintf(title, "poti %s", POTI_VER);
#endif
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        fprintf(stderr, "Failed to init SDL2: %s\n", SDL_GetError());
        exit(0);
    }

    SDL_Window *window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 380, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Failed to create SDL2 window: %s\n", SDL_GetError());
        exit(0);
    }
    SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    poti()->keys = SDL_GetKeyboardState(NULL);
    poti()->window = window;
    poti()->render = render;

    if (sbtar_open(&poti()->pkg, "game.pkg"))
        poti()->is_packed = 1;
    s_setup_function_ptrs(poti());

    poti()->init_font(&poti()->default_font, _5x5_ttf, _5x5_ttf_size, 10);

    mo_init(0);

#if 1
    if (luaL_dostring(L, boot_lua) != LUA_OK) {
	const char *error_buf = lua_tostring(L, -1);
	fprintf(stderr, "Failed to load Lua boot: %s\n", error_buf);
	exit(0);
    }
#else
    luaL_dostring(L, buf);
#endif

#if 0
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
#endif

    lua_getglobal(L, "poti");
    if (!lua_isnil(L, -1)) {
        lua_getfield(L, -1, "run");
        if (!lua_isnil(L, -1)) {
            int err = lua_pcall(L, 0, 1, 0);
            if (err) {
                const char *str = lua_tostring(L, -1);
                fprintf(stderr, "Lua error: %s", str);
                exit(0);
            }
            lua_rawsetp(L, LUA_REGISTRYINDEX, boot_lua);
        }
    }
    fprintf(stderr, "Is Packed: %d\n", poti()->is_packed);
    return 1;
}

int poti_quit(void) {
    SDL_DestroyWindow(poti()->window);
    SDL_DestroyRenderer(poti()->render);
    SDL_Quit();
    mo_deinit();
    lua_close(poti()->L);
    return 1;
}

static int _poti_step(lua_State *L) {
#if 0
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
#endif
    lua_rawgetp(L, LUA_REGISTRYINDEX, boot_lua);
    if (lua_pcall(L, 0, 0, 0) != 0) {
        const char *msg = lua_tostring(L, -1);
        fprintf(stderr, "poti error: %s\n", msg);
        exit(0);
    }
    return 1;
}

int poti_loop() {
    char should_close = 0;
    SDL_Event *event = &poti()->event;
    double current_time;
    poti()->last_time = SDL_GetTicks();
    while (!should_close) {
        while (SDL_PollEvent(event)) {
            switch (event->type) {
                case SDL_QUIT:
                    should_close = 1;
                    break;
            }
        }
        current_time = SDL_GetTicks();
        poti()->delta = (current_time - poti()->last_time) / 1000.f;
        poti()->last_time = current_time;
        _poti_step(poti()->L);

        SDL_RenderPresent(poti()->render);
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
        {"circle", poti_draw_circle},
        {"rectangle", poti_draw_rectangle},
        {"triangle", poti_draw_triangle},
        {"print", poti_print},
        /* Audio */
        {"volume", poti_volume},
        /* Types */
        {"Texture", poti_new_texture},
        {"Font", poti_new_font},
        {"Audio", poti_new_audio},
        {"Joystick", poti_joystick},
        {"Gamepad", poti_gamepad},
        // {"Gamepad", poti_gamepad},
        /* Keyboard */
        {"key_down", poti_key_down},
        {"key_up", poti_key_up},
        {"key_pressed", poti_key_pressed},
        {"key_released", poti_key_released},
        /* Mouse */
        {"mouse_pos", poti_mouse_pos},
        {"mouse_down", poti_mouse_down},
        {"mouse_up", poti_mouse_up},
        {"mouse_pressed", poti_mouse_pressed},
        {"mouse_released", poti_mouse_released},
        /* Joystick */
        {"num_joysticks", poti_num_joysticks},
        /* Gamepad */
        {"is_gamepad", poti_is_gamepad},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    struct { char *name; int(*fn)(lua_State*); } libs[] = {
        {"_Texture", luaopen_texture},
        {"_Font", luaopen_font},
        {"_Audio", luaopen_audio},
        {"_Joystick", luaopen_joystick},
        {"_Gamepad", luaopen_gamepad},
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
    lua_pushnumber(L, poti()->delta);
    return 1;
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
    int usage = SDL_TEXTUREACCESS_STATIC;
    if (!strcmp(name, "static")) usage = SDL_TEXTUREACCESS_STATIC;
    else if (!strcmp(name, "stream")) usage = SDL_TEXTUREACCESS_STREAMING;
    else if (!strcmp(name, "target")) usage = SDL_TEXTUREACCESS_TARGET;
    else {
        lua_pushstring(L, "Invalid texture usage");
        return 0;
    }

    return usage;
}

static int _texture_from_path(lua_State *L, Texture **tex) {
    size_t size;
    int top = lua_gettop(L)-1;
    const char *path = luaL_checklstring(L, 1, &size);

    int w, h, format, req_format;
    req_format = STBI_rgb_alpha;
    // FILE *fp;
    // fp = fopen(path, "rb");
    // fseek(fp, 0, SEEK_END);
    // int sz = ftell(fp);
    // fseek(fp, 0, SEEK_SET);

    // unsigned char img[sz];
    // fread(img, 1, sz, fp);
    // fclose(fp);

    // sbtar_t *tar = &poti()->pkg;

    // sbtar_find(tar, path);
    // sbtar_header_t header;
    // sbtar_header(tar, &header);
    // char img[header.size];
    // sbtar_data(tar, img, header.size);

    size_t fsize;
    char *img = poti()->read_file(path, &fsize);
    if (!img) {
        fprintf(stderr, "Failed to open image %s\n", path);
        exit(1);
    }

    unsigned char *pixels = stbi_load_from_memory(img, fsize, &w, &h, &format, req_format);
    int pixel_format = SDL_PIXELFORMAT_RGBA32;
    switch (req_format) {
        case STBI_rgb: pixel_format = SDL_PIXELFORMAT_RGB888; break;
        case STBI_rgb_alpha: pixel_format = SDL_PIXELFORMAT_RGBA32; break;
    }

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, 1, w*req_format, pixel_format);

    *tex = SDL_CreateTextureFromSurface(poti()->render, surf);
    SDL_FreeSurface(surf);
    stbi_image_free(pixels);
    free(img);
    // *tex = tea_texture_load(path, usage);
    if (!*tex) {
        fprintf(stderr, "Failed to load texture: %s\n", SDL_GetError());
        exit(0);
    }

    return 1;
}

static int _texture_from_size(lua_State *L, Texture **tex) {
    int top = lua_gettop(L)-1;
    float w, h;
    w = luaL_checknumber(L, 1);
    h = luaL_checknumber(L, 2);
    
    const char* s_usage = NULL;
    if (top > 1 && lua_type(L, top) == LUA_TSTRING)
        s_usage = luaL_checkstring(L, top);

    int usage = _textype_from_string(L, s_usage);
    // *tex = tea_texture(NULL, w, h, TEA_RGBA, usage);
    *tex = SDL_CreateTexture(poti()->render, SDL_PIXELFORMAT_RGBA32, usage, w, h);
    if (!*tex) {
        fprintf(stderr, "Failed to load texture: %s\n", SDL_GetError());
        exit(0);
    }

    return 1;
}

int poti_new_texture(lua_State *L) {
    Texture **tex = lua_newuserdata(L, sizeof(*tex));
    luaL_setmetatable(L, TEXTURE_CLASS);

    switch(lua_type(L, 1)) {
        case LUA_TSTRING: _texture_from_path(L, tex); break;
        case LUA_TNUMBER: _texture_from_size(L, tex); break;
    }
    
    return 1;
}

static int get_rect_from_table(lua_State *L, int index, SDL_Rect *r) {
    if (!lua_istable(L, index)) return 0;
    int parts[4] = {r->x, r->y, r->w, r->h};

    size_t len = lua_rawlen(L, index);
    int i;
    for (i = 0; i < len; i++) {
        lua_pushinteger(L, i+1);
        lua_gettable(L, index);

        parts[i] = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    r->x = parts[0];
    r->y = parts[1];
    r->w = parts[2];
    r->h = parts[3];

    return 1;
}

static int get_point_from_table(lua_State *L, int index, SDL_Point *p) {
    if (!lua_istable(L, index)) return 0;
    int parts[2] = { p->x, p->y };
    size_t len = lua_rawlen(L, index);

    int i;
    for (i = 0; i < len; i++) {
        lua_pushinteger(L, i+1);
        lua_gettable(L, index);
        parts[i] = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    p->x = parts[0];
    p->y = parts[1];

    return 1;
}

static int get_flip_from_table(lua_State *L, int index, int *flip) {
    if (!lua_istable(L, index)) return 0;
    size_t len = lua_rawlen(L, index);

    int i;
    for (i = 0; i < len; i++) {
        lua_pushinteger(L, i+1);
        lua_gettable(L, index);
        *flip |= (i+1) * lua_toboolean(L, -1);
        lua_pop(L, 1);
    }

    return 1;
}


int poti_texture_draw(lua_State *L) {
    Texture **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);
    int index = 2;
    int offset = 0;
    int top = lua_gettop(L);
    // if (top < 2) tea_texture_draw(*tex, NULL, NULL);
    int w, h;
    SDL_QueryTexture(*tex, NULL, NULL, &w, &h);

    SDL_Rect s = {0, 0, w, h};
    get_rect_from_table(L, index++, &s);
    SDL_Rect d = { 0, 0, s.w, s.h };
    get_rect_from_table(L, index++, &d);

    if (index > 3) {
        float angle = luaL_optnumber(L, index++, 0);

        SDL_Point origin = {0, 0};
        get_point_from_table(L, index++, &origin);

        int flip = SDL_FLIP_NONE;
        get_flip_from_table(L, index++, &flip);

        SDL_RenderCopyEx(poti()->render, *tex, &s, &d, angle, &origin, flip);
    } else {
        SDL_RenderCopy(poti()->render, *tex, &s, &d);
    }

    return 0;
}

int poti_texture_width(lua_State *L) {
    Texture **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);

    int width;
    SDL_QueryTexture(*tex, NULL, NULL, &width, NULL);

    lua_pushnumber(L, width);
    return 1;
}

int poti_texture_height(lua_State *L) {
    Texture **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);

    int height;
    SDL_QueryTexture(*tex, NULL, NULL, &height, NULL);

    lua_pushnumber(L, height);
    return 1;
}

int poti_texture_size(lua_State *L) {
    Texture **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);

    int width, height;
    SDL_QueryTexture(*tex, NULL, NULL, &width, &height);

    lua_pushnumber(L, width);
    lua_pushnumber(L, height);
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

int poti_new_font(lua_State *L) {
   const char *path = luaL_checkstring(L, 1); 
   int size = luaL_optnumber(L, 2, 16);

   Font *font = lua_newuserdata(L, sizeof(*font));
   luaL_setmetatable(L, FONT_CLASS);
   
   size_t buf_size;
   i8 *font_data = poti()->read_file(path, &buf_size);
   poti()->init_font(font, font_data, buf_size, size);
   free(font_data);

   return 1;
}

int poti_font_print(lua_State *L) {
    // te_font_t **font = luaL_checkudata(L, 1, FONT_CLASS);
    // const char *text = luaL_optstring(L, 2, "");
    float x, y;
    x = luaL_optnumber(L, 3, 0);
    y = luaL_optnumber(L, 4, 0);

    // tea_font_print(*font, text, x, y);

    return 0;
}

int poti_font__gc(lua_State *L) {
    // te_font_t **font = luaL_checkudata(L, 1, FONT_CLASS);
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

int poti_new_audio(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const char *s_usage = luaL_optstring(L, 1, "stream");

    Audio **buf = lua_newuserdata(L, sizeof(*buf));
    luaL_setmetatable(L, AUDIO_CLASS);
    int usage = MO_AUDIO_STREAM;

    if (!strcmp(s_usage, "static")) usage = MO_AUDIO_STATIC; 

    size_t size;
    void *data = poti()->read_file(path, &size);
    *buf = mo_audio(data, size, usage);
    return 1;
}

int poti_volume(lua_State *L) {
    float volume = luaL_optnumber(L, 1, 0);
    mo_volume(NULL, volume);

    return 0;
}

int poti_audio_play(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    mo_play(*buf);
    return 0;
}

int poti_audio_stop(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    mo_stop(*buf);
    return 0;
}

int poti_audio_pause(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    mo_pause(*buf);
    return 0;
}

int poti_audio_playing(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    lua_pushboolean(L, mo_is_playing(*buf));
    return 1;
}

int poti_audio_volume(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    float volume = luaL_optnumber(L, 2, 0);
    mo_volume(*buf, volume);
    return 0;
}

int poti_audio_pitch(lua_State *L) { return 0; }

int poti_audio__gc(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    mo_audio_destroy(*buf); 
    return 0;
}

/*********************************
 * Draw
 *********************************/

int poti_clear(lua_State *L) {
    color_t color = {0, 0, 0, 255};
    int args = lua_gettop(L);

    for (int i = 0; i < args; i++) {
        color[i] = lua_tonumber(L, i+1);
    }
    SDL_SetRenderDrawColor(poti()->render, color[0], color[1], color[2], color[3]);
    SDL_RenderClear(poti()->render);
    SDL_SetRenderDrawColor(poti()->render, poti()->color[0], poti()->color[1], poti()->color[2], poti()->color[3]);
    return 0;
}

int poti_mode(lua_State *L) {
    const char *mode_str = luaL_checkstring(L, 1);
    int mode = 0;
    if (!strcmp(mode_str, "line")) mode = 0;
    else if (!strcmp(mode_str, "fill")) mode = 1;
    poti()->draw_mode = mode;
    return 0;
}

int poti_color(lua_State *L) {
    int args = lua_gettop(L);

    color_t col = {0, 0, 0, 255};
    for (int i = 0; i < args; i++) { 
        col[i] = luaL_checknumber(L, i+1);
    }
    SDL_SetRenderDrawColor(poti()->render, col[0], col[1], col[2], col[3]);
    memcpy(&poti()->color, &col, sizeof(color_t));

    return 0;
}

int poti_set_target(lua_State *L) {
    Texture **tex = luaL_testudata(L, 1, TEXTURE_CLASS);
    Texture *t = NULL;
    if (tex) t = *tex;

    SDL_SetRenderTarget(poti()->render, t);
    return 0;
}

int poti_draw_point(lua_State *L) {
    float x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);

    SDL_RenderDrawPointF(poti()->render, x, y);
    return 0;
}

int poti_draw_line(lua_State *L) {
    float p[4];

    for (int i = 0; i < 4; i++) {
        p[i] = luaL_checknumber(L, i+1);
    }

    SDL_RenderDrawLineF(poti()->render, p[0], p[1], p[2], p[3]);
    return 0;
}

typedef void(*DrawCircle)(float, float, float, int);

static void circle_fill_segment(int xc, int yc, int x, int y) {
    SDL_RenderDrawLine(poti()->render, xc + x, yc + y, xc - x, yc + y);
    SDL_RenderDrawLine(poti()->render, xc + x, yc - y, xc - x, yc - y);
    SDL_RenderDrawLine(poti()->render, xc + y, yc + x, xc - y, yc + x);
    SDL_RenderDrawLine(poti()->render, xc + y, yc - x, xc - y, yc - x);
}

static void circle_line_segment(int xc, int yc, int x, int y) {
    SDL_RenderDrawPoint(poti()->render, xc + x, yc + y);
    SDL_RenderDrawPoint(poti()->render, xc - x, yc + y);

    SDL_RenderDrawPoint(poti()->render, xc + x, yc - y);
    SDL_RenderDrawPoint(poti()->render, xc - x, yc - y);

    SDL_RenderDrawPoint(poti()->render, xc + y, yc + x);
    SDL_RenderDrawPoint(poti()->render, xc - y, yc + x);

    SDL_RenderDrawPoint(poti()->render, xc + y, yc - x);
    SDL_RenderDrawPoint(poti()->render, xc - y, yc - x);
}

static void draw_filled_circle(float xc, float yc, float radius, int segments) {
    int x = 0, y = radius;
    int d = 3 - 2 * radius;
    circle_fill_segment(xc, yc, x, y);

    while (y >= x) {
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else
            d = d + 4 * x + 6;
        circle_fill_segment(xc, yc, x, y);
    }
}

static void draw_lined_circle(float xc, float yc, float radius, int segments) {
    int x = 0, y = radius;
    int d = 3 - (2 * radius);
    circle_line_segment(xc, yc, x, y);

    while (y >= x) {
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else
            d = d + 4 * x + 6;
        circle_line_segment(xc, yc, x, y);
    }
}

int poti_draw_circle(lua_State *L) {

    float x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);

    float radius = luaL_checknumber(L, 3);

    float segments = luaL_optnumber(L, 4, 32);

    DrawCircle fn[2] = {
        draw_lined_circle,
        draw_filled_circle
    };

    fn[poti()->draw_mode](x, y, radius, segments);

    return 0;
}

typedef void(*DrawRect)(SDL_Rect*);

static void draw_filled_rect(SDL_Rect *r) {
    SDL_RenderFillRect(poti()->render, r);
}

static void draw_lined_rect(SDL_Rect *r) {
    SDL_RenderDrawRect(poti()->render, r);
}

int poti_draw_rectangle(lua_State *L) {

    SDL_Rect r = {0, 0, 0, 0};

    r.x = luaL_checknumber(L, 1);
    r.y = luaL_checknumber(L, 2);
    r.w = luaL_checknumber(L, 3);
    r.h = luaL_checknumber(L, 4);

    DrawRect fn[2] = {
        draw_lined_rect,
        draw_filled_rect
    };

    fn[poti()->draw_mode](&r);
    
    return 0;
}

typedef void(*DrawTriangle)(float, float, float, float, float, float);

static void bottom_flat_triangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    float invslope1 = (x1 - x0) / (y1 - y0);
    float invslope2 = (x2 - x0) / (y2 - y0);

    float curX1 = x0;
    float curX2 = x0;

    for (int scanlineY = y0; scanlineY <= y1; scanlineY++) {
        SDL_RenderDrawLineF(poti()->render, curX1, scanlineY, curX2, scanlineY);
        curX1 += invslope1;
        curX2 += invslope2;
    } 
}

static void top_flat_triangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    float invslope1 = (x2 - x0) / (y2 - y0);
    float invslope2 = (x2 - x1) / (y2 - y1);

    float curX1 = x2;
    float curX2 = x2;

    for (int scanlineY = y2; scanlineY > y0; scanlineY--) {
        SDL_RenderDrawLineF(poti()->render, curX1, scanlineY, curX2, scanlineY);
        curX1 -= invslope1;
        curX2 -= invslope2;
    }
}

static void swap_pos(float *x0, float *y0, float *x1, float *y1) {
    float aux_x, aux_y;
    aux_x = *x0;
    aux_y = *y0;
    *x0 = *x1;
    *y0 = *y1;
    *x1 = aux_x;
    *y1 = aux_y;
}

static void draw_filled_triangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    if (y1 < y0)
        swap_pos(&x1, &y1, &x0, &y0);
    if (y2 < y0)
        swap_pos(&x2, &y2, &x0, &y0);
    if (y2 < y1)
        swap_pos(&x2, &y2, &x1, &y1);

    if (y1 == y2)
        bottom_flat_triangle(x0, y0, x1, y1, x2, y2);
    else if (y0 == y1)
        top_flat_triangle(x0, y0, x1, y1, x2, y2);
    else {
        float x3, y3;
        x3 = x0 + ((y1 - y0) / (y2 - y0)) * (x2 - x0);
        y3 = y1;
        bottom_flat_triangle(x0, y0, x1, y1, x3, y3);
        top_flat_triangle(x1, y1, x3, y3, x2, y2);
    }
}

static void draw_lined_triangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    SDL_RenderDrawLine(poti()->render, x0, y0, x1, y1);
    SDL_RenderDrawLine(poti()->render, x1, y1, x2, y2);
    SDL_RenderDrawLine(poti()->render, x2, y2, x0, y0);
}

int poti_draw_triangle(lua_State *L) {

    float points[6];

    for (int i = 0; i < 6; i++) {
        points[i] = luaL_checknumber(L, i+1);
    }

    const DrawTriangle fn[] = {
        draw_lined_triangle,
        draw_filled_triangle
    };

    fn[poti()->draw_mode](points[0], points[1], points[2], points[3], points[4], points[5]);

    return 0;
}

static void s_char_rect(Font *font, const i32 c, f32 *x, f32 *y, SDL_Point *out_pos, SDL_Rect *rect, int width) {
    if (c == '\n') {
        *x = 0;
        *y += font->height;
        return;
    }

    if (c == '\t') {
        *x += font->c[c].bw * 2;
        return;
    }
    if (width != 0 && *x + (font->c[c].bl) > width) {
        *x = 0;
        *y += font->height;
    }

    float x2 = *x + font->c[c].bl;
    float y2 = *y + font->c[c].bt;
    float w = font->c[c].bw;
    float h = font->c[c].bh;

    float s0 = font->c[c].tx;
    float t0 = 0;
    int s1 = font->c[c].bw;
    int t1 = font->c[c].bh;

    *x += font->c[c].ax;
    *y += font->c[c].ay;

    if (out_pos) {
        out_pos->x = x2;
        out_pos->y = y2;
    }
    if (rect) *rect = (SDL_Rect){s0, t0, s1, t1};
}

int poti_print(lua_State *L) {
    int index = 1;
    Font *font = luaL_testudata(L, index++, FONT_CLASS);
    if (!font) {
        font = &poti()->default_font;
        --index;
    }

    const i8 *text = luaL_checkstring(L, index++);
    float x, y;
    x = luaL_optnumber(L, index++, 0);
    y = luaL_optnumber(L, index++, 0);

    int len = strlen(text);
    u8 *p = (u8*)text;

    float x0 = 0, y0 = 0;
    while (*p) {
        int codepoint;
        p = poti()->utf8_codepoint(p, &codepoint);
        SDL_Rect src, dest;
        SDL_Point pos;
        s_char_rect(font, codepoint, &x0, &y0, &pos, &src, 0);
        dest.x = x + pos.x;
        dest.y = y + pos.y;
        dest.w = src.w;
        dest.h = src.h;
        SDL_RenderCopy(poti()->render, font->tex, &src, &dest);
    }

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

int luaopen_joystick(lua_State *L) {
    luaL_Reg reg[] = {
        {"num_axes", poti_joystick_num_axes},
        {"num_hats", poti_joystick_num_hats},
        {"num_balls", poti_joystick_num_balls},
        {"num_buttons", poti_joystick_num_buttons},
        {"axis", poti_joystick_axis},
        {"button", poti_joystick_button},
        {"hat", poti_joystick_hat},
        {"ball", poti_joystick_ball},
        {"name", poti_joystick_name},
        {"vendor", poti_joystick_vendor},
        {"product", poti_joystick_product},
        {"product_version", poti_joystick_product_version},
        {"type", poti_joystick_type},
        {"rumble", poti_joystick_rumble},
        {"powerlevel", poti_joystick_powerlevel},
        {"__gc", poti_joystick__gc},
        {NULL, NULL}
    };
    luaL_newmetatable(L, JOYSTICK_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

int luaopen_gamepad(lua_State *L) {
    luaL_Reg reg[] = {
        {"axis", poti_gamepad_axis},
        {"button", poti_gamepad_button},
        {"name", poti_gamepad_name},
        {"vendor", poti_gamepad_vendor},
        {"product", poti_gamepad_product},
        {"product_version", poti_gamepad_product_version},
        {"rumble", poti_gamepad_rumble},
        // {"powerlevel", poti_gamepad_powerlevel},
        {"__gc", poti_gamepad__gc},
        {NULL, NULL}
    };
    luaL_newmetatable(L, GAMEPAD_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

int poti_key_down(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = SDL_GetScancodeFromName(key);
    lua_pushboolean(L, poti()->keys[code]);
    
    return 1;
}

int poti_key_up(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = SDL_GetScancodeFromName(key);
    lua_pushboolean(L, !poti()->keys[code]);
    
    return 1;
}

int poti_key_pressed(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = SDL_GetScancodeFromName(key);
    lua_pushboolean(L, poti()->keys[code]);
    
    return 1;
}

int poti_key_released(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = SDL_GetScancodeFromName(key);
    lua_pushboolean(L, !poti()->keys[code]);
    
    return 1;
}

int poti_mouse_pos(lua_State *L) {
    int x, y;
    SDL_GetMouseState(&x, &y);

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

static int mouse_down(int btn) {
    return SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(btn);
}

int poti_mouse_down(lua_State *L) {
    int button = luaL_checknumber(L, 1);

    lua_pushboolean(L, mouse_down(button));
    
    return 1;
}

int poti_mouse_up(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, !mouse_down(button));
    
    return 1;
}

int poti_mouse_pressed(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, mouse_down(button));
    
    return 1;
}

int poti_mouse_released(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, !mouse_down(button));
    
    return 1;
}

/* Joystick */
int poti_joystick(lua_State* L) {
    Joystick** j = lua_newuserdata(L, sizeof(*j));
    luaL_setmetatable(L, JOYSTICK_CLASS);
    int number = luaL_checknumber(L, 1);
    *j = SDL_JoystickOpen(number);

    return 1;
}

int poti_num_joysticks(lua_State *L) {
    lua_pushnumber(L, SDL_NumJoysticks());
    return 1;
}

int poti_joystick_name(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushstring(L, SDL_JoystickName(*j));
    return 1;
}

int poti_joystick_vendor(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickGetVendor(*j));
    return 1;
}

int poti_joystick_product(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickGetProduct(*j));
    return 1;
}

int poti_joystick_product_version(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
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

int poti_joystick_type(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushstring(L, joy_types[SDL_JoystickGetType(*j)]);
    return 1;
}


int poti_joystick_num_axes(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumAxes(*j));
    return 1;
}

int poti_joystick_num_balls(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumBalls(*j));
    return 1;
}

int poti_joystick_num_hats(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumHats(*j));
    return 1;
}

int poti_joystick_num_buttons(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumButtons(*j));
    return 1;
}
int poti_joystick_axis(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int axis = luaL_checknumber(L, 2);
    lua_pushnumber(L, SDL_JoystickGetAxis(*j, axis));
    return 1;
}

int poti_joystick_ball(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int ball = luaL_checknumber(L, 2);
    int dx, dy;
    SDL_JoystickGetBall(*j, ball, &dx, &dy);
    lua_pushnumber(L, dx);
    lua_pushnumber(L, dy);
    return 2;
}

int poti_joystick_hat(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int hat = luaL_checknumber(L, 2);
    lua_pushnumber(L, SDL_JoystickGetHat(*j, hat));
    return 1;
}

int poti_joystick_button(lua_State* L) {
    Joystick** j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int button = luaL_checknumber(L, 2);
    int res = SDL_JoystickGetButton(*j, button);

    lua_pushboolean(L, res);
    return 1;
}

int poti_joystick_rumble(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    u16 low = luaL_checknumber(L, 2);
    u16 high = luaL_checknumber(L, 3);
    u32 freq = luaL_optnumber(L, 4, 100);
    lua_pushboolean(L, SDL_JoystickRumble(*j, low, high, freq) == 0);
    return 1;
}

const char *joy_powerlevels[] = {
    "unknown", "empty", "low", "medium", "high", "full", "wired"
};

int poti_joystick_powerlevel(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushstring(L, joy_powerlevels[SDL_JoystickCurrentPowerLevel(*j) + 1]);
    return 1;
}

int poti_joystick_close(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    SDL_JoystickClose(*j);
    return 0;
}

int poti_joystick__gc(lua_State* L) {
    Joystick** j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    SDL_JoystickClose(*j);
    return 0;
}

/* Game Controller */
int poti_gamepad(lua_State *L) {
    GameController **g = lua_newuserdata(L, sizeof(*g));
    *g = SDL_GameControllerOpen(luaL_checknumber(L, 1));
    luaL_setmetatable(L, GAMEPAD_CLASS);
    return 1;
}

int poti_is_gamepad(lua_State *L) {
    lua_pushboolean(L, SDL_IsGameController(luaL_checknumber(L, 1)));
    return 1;
}

int poti_gamepad_name(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushstring(L, SDL_GameControllerName(*g));
    return 1;
}

int poti_gamepad_vendor(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushnumber(L, SDL_GameControllerGetVendor(*g));
    return 1;
}

int poti_gamepad_product(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushnumber(L, SDL_GameControllerGetProduct(*g));
    return 1;
}

int poti_gamepad_product_version(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushnumber(L, SDL_GameControllerGetProductVersion(*g));
    return 1;
}

const char *gpad_axes[] = {
    "leftx", "lefty", "rightx", "righty", "triggerleft", "triggerright"
};

int poti_gamepad_axis(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    const char *str = luaL_checkstring(L, 2);
    int axis = SDL_GameControllerGetAxisFromString(str);
    if (axis < 0) {
        luaL_argerror(L, 2, "invalid axis");
    }
    lua_pushnumber(L, SDL_GameControllerGetAxis(*g, axis));
    return 1;
}

int poti_gamepad_button(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    const char *str = luaL_checkstring(L, 2);
    int button = SDL_GameControllerGetButtonFromString(str);
    if (button < 0) {
        luaL_argerror(L, 2, "invalid button");
    }
    lua_pushboolean(L, SDL_GameControllerGetButton(*g, button));
    return 1;
}

int poti_gamepad_rumble(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    u16 low = luaL_checknumber(L, 2);
    u16 high = luaL_checknumber(L, 3);
    u32 freq = luaL_optnumber(L, 4, 100);
    lua_pushboolean(L, SDL_GameControllerRumble(*g, low, high, freq) == 0);
    return 1;
}

int poti_gamepad_close(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    SDL_GameControllerClose(*g);
    return 0;
}

int poti_gamepad__gc(lua_State* L) {
    GameController** g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    SDL_GameControllerClose(*g);
    return 0;
}

static i8* s_read_file(const char *filename, size_t *size) {
    FILE *fp;
#if defined(_WIN32)
    fopen_s(&fp, filename, "rb+");
#else
    fp = fopen(filename, "rb");
#endif
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp);
    if (size) *size = sz;
    fseek(fp, 0, SEEK_SET);
    char *str = malloc(sz);
    fread(str, 1, sz, fp);
    fclose(fp);

    return str;
}

static i8* s_read_file_packed(const char *filename, size_t *size) {
    sbtar_t *tar = &poti()->pkg;
    if (!sbtar_find(tar, filename)) return;
    sbtar_header_t h;
    sbtar_header(tar, &h);
    char *str = malloc(h.size);
    if (size) *size = h.size;
    sbtar_data(tar, str, h.size);

    return str;
}

#define MAX_UNICODE 0x10FFFF

static u8* s_utf8_codepoint(u8 *p, i32 *codepoint) {
    u8* n = NULL;
    u8 c = *p;
    if (c < 0x80) {
        *codepoint = c;
        n = p + 1;
        return n;
    }

    switch (c & 0xf0) {
        case 0xf0: {
            *codepoint = ((p[0] & 0x07) << 18) | ((p[1] & 0x3f) << 12) | ((p[2] & 0x3f) << 6) | ((p[3] & 0x3f));
            n = p + 4;
            break;
        }
        case 0xe0: {
            *codepoint = ((p[0] & 0x0f) << 12) | ((p[1] & 0x3f) << 6) | ((p[2] & 0x3f));
            n = p + 3;
            break;
        }
        case 0xc0:
        case 0xd0: {
            *codepoint = ((p[0] & 0x1f) << 6) | ((p[1] & 0x3f));
            n = p + 2;
            break;
        }
        default:
        {
            *codepoint = -1;
            n = n + 1;
        }
    }
    if (*codepoint > MAX_UNICODE) *codepoint = -1;
    return n;
}

static int s_init_font(Font *font, const void *data, size_t buf_size, int font_size) {
    if (buf_size <= 0) return 0;
    font->data = malloc(buf_size);
    if (!font->data) {
        fprintf(stderr, "Failed to alloc Font data\n");
        exit(EXIT_FAILURE);
    }
    memcpy(font->data, data, buf_size);

    if (!stbtt_InitFont(&font->info, font->data, 0)) {
        fprintf(stderr, "Failed to init stbtt_info\n");
        exit(EXIT_FAILURE);
    }

    int ascent, descent, line_gap;
    font->size = font_size;
    float fsize = font_size;
    font->scale = stbtt_ScaleForMappingEmToPixels(&font->info, fsize);
    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
    font->baseline = ascent * font->scale;

    int tw = 0, th = 0;

    for (int i = 0; i < 256; i++) {
        int ax, bl;
        int x0, y0, x1, y1;
        int w, h;

        stbtt_GetCodepointHMetrics(&font->info, i, &ax, &bl);
        stbtt_GetCodepointBitmapBox(&font->info, i, font->scale, font->scale, &x0, &y0, &x1, &y1);

        w = x1 - x0;
        h = y1 - y0;

        font->c[i].ax = ax * font->scale;
        font->c[i].ay = 0;
        font->c[i].bl = bl * font->scale;
        font->c[i].bw = w;
        font->c[i].bh = h;
        font->c[i].bt = font->baseline + y0;

        tw += w;
        th = th > h ? th : h;
    }

    font->height = th;

    const int final_size = tw * th;

    u32* bitmap = malloc(final_size * sizeof(u32));
    memset(bitmap, 0, final_size * sizeof(u32));
    int x = 0;
    SDL_PixelFormat *fmt = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
    for (int i = 0; i < 256; i++) {
        int ww = font->c[i].bw;
        int hh = font->c[i].bh;
        int ssize = ww * hh;
        int ox, oy;

        unsigned char *bmp = stbtt_GetCodepointBitmap(&font->info, 0, font->scale, i, NULL, NULL, &ox, &oy);
        // u32 pixels[ssize];
        int xx, yy;
        xx = yy = 0;
        for (int j = 0; j < ssize; j++) {
            // int ii = j / 4;
            xx = (j % ww) + x;
            if (j != 0 && j % ww == 0) {
                yy += tw;
            }
            bitmap[xx + yy] = SDL_MapRGBA(fmt, 255, 255, 255, bmp[j]);
        }
        stbtt_FreeBitmap(bmp, font->info.userdata);
        
        font->c[i].tx = x;

        x += font->c[i].bw;
    }
    SDL_FreeFormat(fmt);
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(bitmap, tw, th, 1, tw * 4, SDL_PIXELFORMAT_RGBA32);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(poti()->render, surf);
    SDL_FreeSurface(surf);
    free(bitmap);
    font->tex = tex;
}

static Font* s_load_font(const i8 *filename, i32 font_size) {
    Font *font = malloc(sizeof(*font));
    if (!font) {
        fprintf(stderr, "Failed to alloc memory for font\n");
        exit(EXIT_FAILURE);
    }
    size_t buf_size;
    i8 *buf = poti()->read_file(filename, &buf_size);
    poti()->init_font(font, buf, buf_size, font_size);
    free(buf);
    return font;
}

int s_setup_function_ptrs(struct Context *ctx) {
    ctx->read_file = s_read_file;
    if (poti()->is_packed)
        ctx->read_file = s_read_file_packed;
    ctx->init_font = s_init_font;
    ctx->utf8_codepoint = s_utf8_codepoint;

    return 1;
}

int main(int argc, char ** argv) {
    poti_init();
    poti_loop();
    poti_quit();
    return 1;
}
