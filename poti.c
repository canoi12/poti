#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

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

#include "miniaudio.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include "sbtar.h"

#include "poti.h"

#define MAX_AUDIO_BUFFER_CHANNELS 255

#define POTI_VER "0.1.0"

#define POINT_CLASS "Point"
#define RECT_CLASS "Rect"

#define TEXTURE_CLASS "Texture"
#define FONT_CLASS "Font"

#define AUDIO_CLASS "Audio"

#define JOYSTICK_CLASS "Joystick"
#define GAMEPAD_CLASS "GamePad"

#define poti() (&_ctx)

#define AUDIO_DEVICE_FORMAT ma_format_f32
#define AUDIO_DEVICE_CHANNELS 2
#define AUDIO_DEVICE_SAMPLE_RATE 44100

#define AUDIO_STREAM 0
#define AUDIO_STATIC 1

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

typedef struct Audio Audio;
typedef struct AudioData AudioData;
typedef struct AudioBuffer AudioBuffer;

typedef SDL_Texture Texture;
typedef struct Font Font;

typedef SDL_Window Window;
typedef SDL_Renderer Render;
typedef SDL_Event Event;

typedef SDL_Joystick Joystick;
typedef SDL_GameController GameController;
typedef void Keyboard;
typedef void Mouse;

#ifndef ma_countof
#define ma_countof(x)               (sizeof(x) / sizeof(x[0]))
#endif

struct AudioData {
    u8 usage;
    u32 size;
    u8* data;
};

struct AudioBuffer {
    u8 *data;
    u16 id;
    ma_decoder decoder;
    f32 volume, pitch;
    u8 playing, paused;
    u8 loop, loaded;
    i64 offset;
    u8 usage;
    u32 size;
};

struct Audio {
    AudioData data;
};

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

    struct {
        ma_context ctx;
        ma_device device;
        ma_mutex lock;

        u8 is_ready;

        AudioBuffer buffers[MAX_AUDIO_BUFFER_CHANNELS];
    } audio;
    
    u8 should_close;

    struct {
        f64 last;
        f64 delta;
        i32 fps, frames;
    } time;

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
static const i8 _callbacks;
static const i8 _poti_step;
static const i8 _audio_data;

static const i8 *_err_func =
"function poti.error(msg, trace)\n"
"   local trace = debug.traceback('', 1)\n"
"    poti.update = function() end\n"
"    poti.draw = function()\n"
"       poti.clear(0, 0, 0)\n"
"       poti.print('error', 32)\n"
"       poti.print(msg, 32, 16)\n"
"       poti.print(trace, 32, 32)\n"
"    end\n"
"end\n"
"local function _err(msg)\n"
"    local trace = debug.traceback('', 1)\n"
"    print('okok')\n"
"    print(msg, trace)\n"
"    poti.error(msg, trace)\n"
"end\n"
"return _err";

static const i8 *_initialize =
"local traceback = debug.traceback\n"
"local main_state = false\n"
"local function _err(msg)\n"
"   local trace = traceback('', 1)\n"
"   print(msg, trace)\n"
"   poti.error(msg, trace)\n"
"end\n"
"local function _mainLoop()\n"
"   if poti.update then poti.update(poti.delta()) end\n"
"   if poti.draw then poti.draw() end\n"
"end\n"
"local _step = _mainLoop\n"
"function poti.run()\n"
"   if poti.load then poti.load() end\n"
"   return _mainLoop\n"
"end\n"
"local function _initialize()\n"
"   if not main_state then return function() xpcall(_step, _err) end end\n"
"   local state, ret = xpcall(poti.run, _err)\n"
"   if ret then _step = ret end\n"
"   return function() xpcall(_step, _err) end\n"
"end\n"
"function poti.quit()\n"
"   return true\n"
"end\n"
"main_state = xpcall(function() require 'main' end, _err)\n"
"return _initialize";

static int s_get_rect_from_table(lua_State* L, int idx, SDL_Rect* out);
static int s_get_point_from_table(lua_State* L, int idx, SDL_Point* out);
static void s_char_rect(Font *font, const i32 c, f32 *x, f32 *y, SDL_Point *out_pos, SDL_Rect *rect, int width);
static u32 s_read_and_mix_pcm_frames(AudioBuffer *buffer, f32 *output, u32 frames);
static void s_audio_callback(ma_device *device, void *output, const void *input, ma_uint32 frameCount);

static int poti_init(void);
static int poti_loop(void);
static int poti_quit(void);

static int poti_call(const char *name, int args, int ret);

static int luaopen_poti(lua_State *L);
static int luaopen_event(lua_State *L);
static int luaopen_texture(lua_State *L);
static int luaopen_font(lua_State *L);
static int luaopen_audio(lua_State *L);
static int luaopen_keyboard(lua_State* L);
static int luaopen_mouse(lua_State* L);
static int luaopen_joystick(lua_State *L);
static int luaopen_gamepad(lua_State *L);

/*=================================*
 *              Core               *
 *=================================*/
static int l_poti_ver(lua_State *L);
static int l_poti_delta(lua_State *L);

/*=================================*
 *              Event              *
 *=================================*/

static int l_poti_start_textinput(lua_State* L);
static int l_poti_stop_textinput(lua_State* L);
static int l_poti_textinput_rect(lua_State* L);

/* Callbacks */
static int l_poti_callback_quit(lua_State* L);
static int l_poti_callback_keypressed(lua_State* L);
static int l_poti_callback_keyreleased(lua_State* L);

static int l_poti_callback_mousemotion(lua_State* L);
static int l_poti_callback_mousebutton(lua_State* L);
static int l_poti_callback_mousewheel(lua_State* L);

static int l_poti_callback_joyaxismotion(lua_State* L);
static int l_poti_callback_joyballmotion(lua_State* L);
static int l_poti_callback_joyhatmotion(lua_State* L);
static int l_poti_callback_joybutton(lua_State* L);
static int l_poti_callback_joydevice(lua_State* L);

static int l_poti_callback_gamepadaxismotion(lua_State* L);
static int l_poti_callback_gamepadbutton(lua_State* L);
static int l_poti_callback_gamepaddevice(lua_State* L);
static int l_poti_callback_gamepadremap(lua_State* L);
#if SDL_VERSION_ATLEAST(2, 1, 0)
static int l_poti_callback_gamepadtouchpad(lua_State* L);
static int l_poti_callback_gamepadtouchpadmotion(lua_State* L);
#endif

static int l_poti_callback_finger(lua_State* L);
static int l_poti_callback_fingermotion(lua_State* L);
static int l_poti_callback_dollargesture(lua_State* L);
static int l_poti_callback_dollarrecord(lua_State* L);
static int l_poti_callback_multigesture(lua_State* L);
static int l_poti_callback_clipboardupdate(lua_State* L);
static int l_poti_callback_dropfile(lua_State* L);
static int l_poti_callback_droptext(lua_State* L);
static int l_poti_callback_dropbegin(lua_State* L);
static int l_poti_callback_dropcomplete(lua_State* L);

static int l_poti_callback_audiodevice(lua_State* L);

static int l_poti_callback_windowevent(lua_State* L);

static int l_poti_callback_window_moved(lua_State* L);
static int l_poti_callback_window_resized(lua_State* L);
static int l_poti_callback_window_focus(lua_State* L);
static int l_poti_callback_window_minimized(lua_State* L);
static int l_poti_callback_window_maximized(lua_State* L);
static int l_poti_callback_window_restored(lua_State* L);
static int l_poti_callback_window_shown(lua_State* L);
static int l_poti_callback_window_hidden(lua_State* L);
static int l_poti_callback_window_mouse(lua_State* L);

static int l_poti_callback_textinput(lua_State* L);
static int l_poti_callback_textediting(lua_State* L);

/*=================================*
 *              Audio              *
 *=================================*/
static int poti_volume(lua_State *L);
static int l_poti_new_audio(lua_State *L);
static int l_poti_audio_play(lua_State *L);
static int l_poti_audio_stop(lua_State *L);
static int l_poti_audio_pause(lua_State *L);
static int l_poti_audio_playing(lua_State *L);
static int l_poti_audio_volume(lua_State *L);
static int l_poti_audio_pitch(lua_State *L);
static int l_poti_audio__gc(lua_State *L);

/*=================================*
 *           Filesystem            *
 *=================================*/
static int l_poti_file(lua_State *L);
static int l_poti_file_read(lua_State *L);
static int l_poti_file_write(lua_State *L);
static int l_poti_file_seek(lua_State *L);
static int l_poti_file_tell(lua_State *L);
static int poti_list_folder(lua_State *L);

/*=================================*
 *            Graphics             *
 *=================================*/

/* Texture */
static int l_poti_new_texture(lua_State *L);
static int l_poti_texture_draw(lua_State *L);
static int l_poti_texture_width(lua_State *L);
static int l_poti_texture_height(lua_State *L);
static int l_poti_texture_size(lua_State *L);
static int l_poti_texture__gc(lua_State *L);

/* Font */
static int l_poti_new_font(lua_State *L);
static int l_poti_font_print(lua_State *L);
static int l_poti_font__gc(lua_State *L);


/* Draw */
static int l_poti_clear(lua_State *L);
static int l_poti_color(lua_State *L);
static int l_poti_mode(lua_State *L);
static int l_poti_set_target(lua_State *L);
static int l_poti_draw_point(lua_State *L);
static int l_poti_draw_line(lua_State *L);
static int l_poti_draw_rectangle(lua_State *L);
static int l_poti_draw_circle(lua_State *L);
static int l_poti_draw_triangle(lua_State *L);
static int l_poti_print(lua_State *L);

/*=================================*
 *              Input              *
 *=================================*/

/* Keyboard */
static int l_poti_keyboard_down(lua_State *L);
static int l_poti_keyboard_up(lua_State *L);

/* Mouse */
static int l_poti_mouse_pos(lua_State *L);
static int l_poti_mouse_down(lua_State *L);
static int l_poti_mouse_up(lua_State *L);

/* Joystick */
static int poti_joystick(lua_State* L);
static int poti_num_joysticks(lua_State *L);
// static int l_poti_joystick_opened(lua_State *L);

static int l_poti_joystick_name(lua_State *L);
static int l_poti_joystick_vendor(lua_State *L);
static int l_poti_joystick_product(lua_State *L);
static int l_poti_joystick_product_version(lua_State *L);
static int l_poti_joystick_type(lua_State *L);
static int l_poti_joystick_num_axes(lua_State *L);
static int l_poti_joystick_num_balls(lua_State *L);
static int l_poti_joystick_num_hats(lua_State *L);
static int l_poti_joystick_num_buttons(lua_State *L);
static int l_poti_joystick_button(lua_State *L);
static int l_poti_joystick_axis(lua_State *L);
static int l_poti_joystick_hat(lua_State *L);
static int l_poti_joystick_ball(lua_State *L);
static int l_poti_joystick_rumble(lua_State *L);
static int l_poti_joystick_powerlevel(lua_State *L);
static int l_poti_joystick_close(lua_State *L);
static int l_poti_joystick__gc(lua_State *L);

/* Game Controller */
static int l_poti_gamepad(lua_State *L);
static int l_poti_is_gamepad(lua_State *L);
static int l_poti_gamepad_name(lua_State *L);
static int l_poti_gamepad_vendor(lua_State *L);
static int l_poti_gamepad_product(lua_State *L);
static int l_poti_gamepad_product_version(lua_State *L);

static int l_poti_gamepad_axis(lua_State *L);
static int l_poti_gamepad_button(lua_State *L);
static int l_poti_gamepad_rumble(lua_State *L);

static int l_poti_gamepad_close(lua_State *L);
static int l_poti_gamepad__gc(lua_State *L);

static int s_setup_function_ptrs(struct Context *ctx);

int poti_init(void) {
    poti()->L = luaL_newstate();
    lua_State *L = poti()->L;

    luaL_openlibs(L);
    luaL_requiref(L, "poti", luaopen_poti, 1);

    char title[100];
#ifdef _WIN32
    sprintf_s(title, 100, "poti %s", POTI_VER);
#else
    sprintf(title, "poti %s", POTI_VER);
#endif

#ifdef __EMSCRIPTEN__
    u32 flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER |
    SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_SENSOR;
#else
    u32 flags = SDL_INIT_EVERYTHING;
#endif
    if (SDL_Init(flags)) {
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

    poti()->init_font(&poti()->default_font, _font_ttf, _font_ttf_size, 8);

    // mo_init(0);
    ma_context_config ctx_config = ma_context_config_init();
    ma_result result = ma_context_init(NULL, 0, &ctx_config, &poti()->audio.ctx);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to init audio context\n");
        exit(0);
    }

    ma_device_config dev_config = ma_device_config_init(ma_device_type_playback);
    dev_config.playback.pDeviceID = NULL;
    dev_config.playback.format = AUDIO_DEVICE_FORMAT;
    dev_config.playback.channels = AUDIO_DEVICE_CHANNELS;
    dev_config.sampleRate = AUDIO_DEVICE_SAMPLE_RATE;
    dev_config.pUserData = poti();
    dev_config.dataCallback = s_audio_callback;

    result = ma_device_init(&poti()->audio.ctx, &dev_config, &poti()->audio.device);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to init audio device\n");
        ma_context_uninit(&poti()->audio.ctx);
        exit(0);
    }

    if (ma_device_start(&poti()->audio.device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start audio device\n");
        ma_context_uninit(&poti()->audio.ctx);
        ma_device_uninit(&poti()->audio.device);
        exit(0);
    }

    if (ma_mutex_init(&poti()->audio.ctx, &poti()->audio.lock) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start audio mutex\n");
        ma_device_uninit(&poti()->audio.device);
        ma_context_uninit(&poti()->audio.ctx);
        exit(0);
    }

    i32 i;
    for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer *buffer = &poti()->audio.buffers[i];
        memset(buffer, 0, sizeof(*buffer));
        buffer->playing = 0;
        buffer->volume = 1.f;
        buffer->pitch = 1.f;
        buffer->loaded = 0;
        buffer->paused = 0;
        buffer->loop = 0;
    }

    poti()->audio.is_ready = 1;

    lua_newtable(L);
    lua_newtable(L);
    lua_rawseti(L, -2, AUDIO_STREAM);
    lua_newtable(L);
    lua_rawseti(L, -2, AUDIO_STATIC);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &_audio_data);


    if (luaL_dostring(L, _err_func) != LUA_OK) {
        const char *error_buf = lua_tostring(L, -1);
        fprintf(stderr, "Failed to load poti lua error handler: %s\n", error_buf);
        exit(0);
    }
    lua_rawsetp(L, LUA_REGISTRYINDEX, _err_func);

    if (luaL_dostring(L, _initialize) != LUA_OK) {
	    const char *error_buf = lua_tostring(L, -1);
	    fprintf(stderr, "Failed to load poti lua initialize: %s\n", error_buf);
	    exit(0);
    }
    lua_pcall(L, 0, 1, 0);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &_poti_step);

    fprintf(stderr, "Is Packed: %d\n", poti()->is_packed);
    return 1;
}

int poti_quit(void) {
    SDL_DestroyWindow(poti()->window);
    SDL_DestroyRenderer(poti()->render);
    SDL_Quit();
    // mo_deinit();
    if (poti()->audio.is_ready) {
        ma_mutex_uninit(&poti()->audio.lock);
        ma_device_uninit(&poti()->audio.device);
        ma_context_uninit(&poti()->audio.ctx);
    } else
        fprintf(stderr, "Audio Module could not be closed, not initialized\n");

    lua_close(poti()->L);
    return 1;
}

int poti_step(void) {
    lua_State *L = poti()->L;
    SDL_Event *event = &poti()->event;
    lua_rawgetp(poti()->L, LUA_REGISTRYINDEX, &_callbacks);
    while (SDL_PollEvent(event)) {
        lua_rawgeti(L, -1, event->type);
        if (!lua_isnil(L, -1)) {
            lua_pushlightuserdata(L, event);
            lua_pcall(L, 1, 0, 0);
        } else lua_pop(L, 1);
    }
    lua_pop(L, 1);
    double current_time = SDL_GetTicks();
    poti()->delta = (current_time - poti()->last_time) / 1000.f;
    poti()->last_time = current_time;

    lua_rawgetp(L, LUA_REGISTRYINDEX, &_poti_step);
    lua_pcall(L, 0, 0, 0);

    SDL_RenderPresent(poti()->render);
}

int poti_loop(void) {
#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(poti_step, 0, 1);
#else
    while (!poti()->should_close) poti_step();
#endif
    return 1;
}

int poti_call(const char *name, int args, int ret) {
    lua_State* L = poti()->L;
    lua_getglobal(L, "poti");
    if (!lua_isnil(L, -1)) {
        lua_getfield(L, -1, name);
        lua_remove(L, -2);
        if (!lua_isnil(L, -1)) {
            for (int i = 0; i < args; i++) {
                lua_pushvalue(L, -args - 1);
            }
            int err = lua_pcall(L, args, ret, 0);
            if (err) {
                const char* str = lua_tostring(L, -1);
                fprintf(stderr, "Lua error: %s", str);
                exit(0);
            }
        }
        else lua_pop(L, 1);
        for (int i = 0; i < args; i++)
            lua_remove(L, -ret-1);
    }
    return 0;
}

int luaopen_poti(lua_State *L) {
    luaL_Reg reg[] = {
        {"ver", l_poti_ver},
        {"delta", l_poti_delta},
        {"start_textinput", l_poti_start_textinput},
        {"stop_textinput", l_poti_stop_textinput},
        {"textinput_rect", l_poti_textinput_rect},
        /* Draw */
        {"clear", l_poti_clear},
        {"mode", l_poti_mode},
        {"color", l_poti_color},
        {"target", l_poti_set_target},
        {"point", l_poti_draw_point},
        {"line", l_poti_draw_line},
        {"circle", l_poti_draw_circle},
        {"rectangle", l_poti_draw_rectangle},
        {"triangle", l_poti_draw_triangle},
        {"print", l_poti_print},
        /* Audio */
        {"volume", poti_volume},
        /* Types */
        {"Texture", l_poti_new_texture},
        {"Font", l_poti_new_font},
        {"Audio", l_poti_new_audio},
        {"Joystick", poti_joystick},
        {"Gamepad", l_poti_gamepad},
        /* Joystick */
        {"num_joysticks", poti_num_joysticks},
        /* Gamepad */
        {"is_gamepad", l_poti_is_gamepad},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    struct { char *name; int(*fn)(lua_State*); } libs[] = {
        {"_Texture", luaopen_texture},
        {"_Font", luaopen_font},
        {"_Audio", luaopen_audio},
        {"_Joystick", luaopen_joystick},
        {"_Gamepad", luaopen_gamepad},
        {"mouse", luaopen_mouse},
        {"keyboard", luaopen_keyboard},
        {"event", luaopen_event},
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

/*=================================*
 *              Core               *
 *=================================*/
int l_poti_ver(lua_State *L) {
    lua_pushstring(L, POTI_VER);
    return 1;
}

int l_poti_delta(lua_State *L) {
    lua_pushnumber(L, poti()->delta);
    return 1;
}

/*=================================*
 *              Event              *
 *=================================*/

int luaopen_event(lua_State *L) {
    lua_newtable(L);

    struct {
        i32 type;
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

    i32 i;
    for (i = 0; fn[i].type != -1; i++) {
        lua_pushinteger(L, fn[i].type);
        lua_pushcfunction(L, fn[i].fn);
        lua_settable(L, -3);
    }
    lua_rawsetp(L, LUA_REGISTRYINDEX, &_callbacks);

    luaL_Reg reg[] = {
        {NULL, NULL}
    };

    luaL_newlib(L, reg);


    return 1;
}

int l_poti_start_textinput(lua_State* L) {
    SDL_StartTextInput();
    return 0;
}

int l_poti_stop_textinput(lua_State* L) {
    SDL_StopTextInput();
    return 0;
}

int l_poti_textinput_rect(lua_State* L) {
    SDL_Rect r;
    s_get_rect_from_table(L, 1, &r);
    SDL_SetTextInputRect(&r);
    return 0;
}

int l_poti_callback_quit(lua_State* L) {
    poti_call("quit", 0, 1);
    poti()->should_close = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return 0;
}

int l_poti_callback_keypressed(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "lower");
    lua_remove(L, -2);
    lua_pushstring(L, SDL_GetKeyName(event->key.keysym.sym));
    lua_call(L, 1, 1);
    lua_pushboolean(L, event->key.repeat);
    poti_call("key_pressed", 2, 0);
    return 0;
}

int l_poti_callback_keyreleased(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushstring(L, SDL_GetKeyName(event->key.keysym.sym));
    poti_call("key_released", 1, 0);
    return 0;
}

int l_poti_callback_mousemotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->motion.x);
    lua_pushnumber(L, event->motion.y);
    lua_pushnumber(L, event->motion.xrel);
    lua_pushnumber(L, event->motion.yrel);
    lua_pushnumber(L, event->motion.state);
    poti_call("mouse_motion", 5, 0);
    return 0;
}

int l_poti_callback_mousebutton(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->button.button);
    lua_pushboolean(L, event->button.state);
    lua_pushnumber(L, event->button.clicks);
    poti_call("mouse_button", 3, 0);
    return 0;
}

int l_poti_callback_mousewheel(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->wheel.x);
    lua_pushnumber(L, event->wheel.y);
    poti_call("mouse_wheel", 2, 0);
    return 0;
}

int l_poti_callback_joyaxismotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->jaxis.which);
    lua_pushnumber(L, event->jaxis.axis);
    lua_pushnumber(L, event->jaxis.value);
    poti_call("joy_axismotion", 3, 0);
    return 0;
}

int l_poti_callback_joyballmotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->jball.which);
    lua_pushnumber(L, event->jball.ball);
    lua_pushnumber(L, event->jball.xrel);
    lua_pushnumber(L, event->jball.yrel);
    poti_call("joy_ballmotion", 4, 0);
    return 0;
}

int l_poti_callback_joyhatmotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->jhat.which);
    lua_pushnumber(L, event->jhat.hat);
    lua_pushnumber(L, event->jhat.value);
    poti_call("joy_hatmotion", 3, 0);
    return 0;
}

int l_poti_callback_joybutton(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->jbutton.which);
    lua_pushnumber(L, event->jbutton.button);
    lua_pushboolean(L, event->jbutton.state);
    poti_call("joy_button", 3, 0);
    return 0;
}

int l_poti_callback_joydevice(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->jdevice.which);
    lua_pushboolean(L, SDL_JOYDEVICEREMOVED - event->jdevice.type);
    poti_call("joy_device", 2, 0);
    return 0;
}

int l_poti_callback_gamepadaxismotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->caxis.which);
    lua_pushstring(L, SDL_GameControllerGetStringForAxis(event->caxis.axis));
    lua_pushnumber(L, event->caxis.value);
    poti_call("gpad_axismotion", 3, 0);
    return 0;
}

int l_poti_callback_gamepadbutton(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->cbutton.which);
    lua_pushstring(L, SDL_GameControllerGetStringForButton(event->cbutton.button));
    lua_pushboolean(L, event->cbutton.state);
    poti_call("gpad_button", 3, 0);
    return 0;
}

int l_poti_callback_gamepaddevice(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->cdevice.which);
    lua_pushboolean(L, SDL_CONTROLLERDEVICEREMOVED - event->cdevice.type);
    poti_call("gpad_device", 2, 0);
    return 0;
}

int l_poti_callback_gamepadremap(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->cdevice.which);
    lua_pushstring(L, SDL_GameControllerNameForIndex(event->cdevice.which));
    poti_call("gpad_remap", 2, 0);
    return 0;
}

static int l_poti_callback_finger(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->tfinger.touchId);
    lua_pushboolean(L, SDL_FINGERUP - event->type);
    lua_pushnumber(L, event->tfinger.fingerId);
    lua_pushnumber(L, event->tfinger.x);
    lua_pushnumber(L, event->tfinger.y);
    lua_pushnumber(L, event->tfinger.dx);
    lua_pushnumber(L, event->tfinger.dy);
    lua_pushnumber(L, event->tfinger.pressure);
    poti_call("finger_down", 8, 0);
    return 0;
}

static int l_poti_callback_fingermotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->tfinger.touchId);
    lua_pushnumber(L, event->tfinger.fingerId);
    lua_pushnumber(L, event->tfinger.x);
    lua_pushnumber(L, event->tfinger.y);
    lua_pushnumber(L, event->tfinger.dx);
    lua_pushnumber(L, event->tfinger.dy);
    lua_pushnumber(L, event->tfinger.pressure);
    poti_call("finger_motion", 7, 0);
    return 0;
}

static int l_poti_callback_dollargesture(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->dgesture.touchId);
    lua_pushnumber(L, event->dgesture.gestureId);
    lua_pushnumber(L, event->dgesture.numFingers);
    lua_pushnumber(L, event->dgesture.error);
    lua_pushnumber(L, event->dgesture.x);
    lua_pushnumber(L, event->dgesture.y);
    poti_call("dgesture", 6, 0);
    return 0;
}

static int l_poti_callback_dollarrecord(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->dgesture.touchId);
    lua_pushnumber(L, event->dgesture.gestureId);
    lua_pushnumber(L, event->dgesture.numFingers);
    lua_pushnumber(L, event->dgesture.error);
    lua_pushnumber(L, event->dgesture.x);
    lua_pushnumber(L, event->dgesture.y);
    poti_call("dgesture_record", 6, 0);
    return 0;
}

int l_poti_callback_multigesture(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->mgesture.touchId);
    lua_pushnumber(L, event->mgesture.numFingers);
    lua_pushnumber(L, event->mgesture.x);
    lua_pushnumber(L, event->mgesture.y);
    lua_pushnumber(L, event->mgesture.dTheta);
    lua_pushnumber(L, event->mgesture.dDist);
    poti_call("mgesture", 6, 0);
    return 0;
}

int l_poti_callback_clipboardupdate(lua_State* L) {
    // SDL_Event *event = lua_touserdata(L, 1);
    char *text = SDL_GetClipboardText();
    lua_pushstring(L, text);
    poti_call("clipboard_update", 1, 0);
    SDL_free(text);
    return 0;
}

int l_poti_callback_dropfile(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->drop.windowID);
    lua_pushstring(L, event->drop.file);
    poti_call("drop_file", 1, 0);
    return 0;
}

int l_poti_callback_droptext(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->drop.windowID);
    lua_pushstring(L, event->drop.file);
    poti_call("drop_text", 1, 0);
    return 0;
}

int l_poti_callback_dropbegin(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->drop.windowID);
    poti_call("drop_begin", 1, 0);
    return 0;
}

int l_poti_callback_dropcomplete(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->drop.windowID);
    poti_call("drop_complete", 1, 0);
    return 0;
}

int l_poti_callback_audiodevice(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->adevice.which);
    lua_pushboolean(L, SDL_AUDIODEVICEREMOVED - event->type);
    lua_pushboolean(L, event->adevice.iscapture);
    poti_call("audio_device", 3, 0);
    return 0;
}

static int poti_windowevent_null(lua_State* L) {
    return 0;
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
};

int l_poti_callback_windowevent(lua_State *L) {
    SDL_Event *event = lua_touserdata(L, 1);
    window_events[event->window.event](L);
    return 0;
}

int l_poti_callback_window_moved(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->window.windowID);
    lua_pushnumber(L, event->window.data1);
    lua_pushnumber(L, event->window.data2);
    poti_call("window_moved", 3, 0);
    return 0;
}

int l_poti_callback_window_resized(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->window.windowID);
    lua_pushnumber(L, event->window.data1);
    lua_pushnumber(L, event->window.data2);
    poti_call("window_resized", 3, 0);
    return 0;
}

int l_poti_callback_window_minimized(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->window.windowID);
    poti_call("window_minimized", 1, 0);
    return 0;
}

int l_poti_callback_window_maximized(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->window.windowID);
    poti_call("window_maximized", 1, 0);
    return 0;
}

int l_poti_callback_window_restored(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->window.windowID);
    poti_call("window_restored", 1, 0);
    return 0;
}

int l_poti_callback_window_shown(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->window.windowID);
    poti_call("window_shown", 1, 0);
    return 0;
}

int l_poti_callback_window_hidden(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->window.windowID);
    poti_call("window_hidden", 1, 0);
    return 0;
}

int l_poti_callback_window_mouse(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->window.windowID);
    lua_pushboolean(L, SDL_WINDOWEVENT_LEAVE - event->window.event);
    poti_call("window_mouse", 2, 0);
    return 0;
}

int l_poti_callback_window_focus(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->window.windowID);
    lua_pushboolean(L, SDL_WINDOWEVENT_FOCUS_LOST - event->window.event);
    poti_call("window_focus", 2, 0);
    return 0;
}

int l_poti_callback_textinput(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->text.windowID);
    lua_pushstring(L, event->text.text);
    poti_call("text_input", 2, 0);
    return 0;
}

int l_poti_callback_textediting(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushnumber(L, event->edit.windowID);
    lua_pushstring(L, event->edit.text);
    lua_pushnumber(L, event->edit.start);
    lua_pushnumber(L, event->edit.length);
    poti_call("text_edit", 4, 0);
    return 0;
}

/*********************************
 * Texture
 *********************************/

int luaopen_texture(lua_State *L) {
    luaL_Reg reg[] = {
        {"draw", l_poti_texture_draw},
        {"filter", NULL},
        {"wrap", NULL},
        {"width", l_poti_texture_width},
        {"height", l_poti_texture_height},
        {"size", l_poti_texture_size},
        {"__gc", l_poti_texture__gc},
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
    // int top = lua_gettop(L)-1;
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
    u8 *img = (u8*)poti()->read_file(path, &fsize);
    if (!img) {
        int len = strlen(path) + 50;
        char err[len];
        sprintf(err, "failed to open image: %s\n", path);
        lua_pushstring(L, err);
        lua_error(L);
        return 1;
    }

    u8 *pixels = stbi_load_from_memory(img, fsize, &w, &h, &format, req_format);
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
        lua_pushstring(L, "Failed to create SDL texture: ");
        lua_pushstring(L, SDL_GetError());
        lua_concat(L, 2);
        lua_error(L);
        return 1;
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

int l_poti_new_texture(lua_State *L) {
    Texture **tex = lua_newuserdata(L, sizeof(*tex));
    luaL_setmetatable(L, TEXTURE_CLASS);

    switch(lua_type(L, 1)) {
        case LUA_TSTRING: _texture_from_path(L, tex); break;
        case LUA_TNUMBER: _texture_from_size(L, tex); break;
    }
    
    return 1;
}

int s_get_rect_from_table(lua_State *L, int index, SDL_Rect *r) {
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

int s_get_point_from_table(lua_State *L, int index, SDL_Point *p) {
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


int l_poti_texture_draw(lua_State *L) {
    Texture **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);
    int index = 2;
    // int offset = 0;
    // int top = lua_gettop(L);
    // if (top < 2) tea_texture_draw(*tex, NULL, NULL);
    int w, h;
    SDL_QueryTexture(*tex, NULL, NULL, &w, &h);

    SDL_Rect s = {0, 0, w, h};
    s_get_rect_from_table(L, index++, &s);
    SDL_Rect d = { 0, 0, s.w, s.h };
    s_get_rect_from_table(L, index++, &d);

    if (index > 3) {
        float angle = luaL_optnumber(L, index++, 0);

        SDL_Point origin = {0, 0};
        s_get_point_from_table(L, index++, &origin);

        int flip = SDL_FLIP_NONE;
        get_flip_from_table(L, index++, &flip);

        SDL_RenderCopyEx(poti()->render, *tex, &s, &d, angle, &origin, flip);
    } else {
        SDL_RenderCopy(poti()->render, *tex, &s, &d);
    }

    return 0;
}

int l_poti_texture_width(lua_State *L) {
    Texture **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);

    int width;
    SDL_QueryTexture(*tex, NULL, NULL, &width, NULL);

    lua_pushnumber(L, width);
    return 1;
}

int l_poti_texture_height(lua_State *L) {
    Texture **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);

    int height;
    SDL_QueryTexture(*tex, NULL, NULL, &height, NULL);

    lua_pushnumber(L, height);
    return 1;
}

int l_poti_texture_size(lua_State *L) {
    Texture **tex = luaL_checkudata(L, 1, TEXTURE_CLASS);

    int width, height;
    SDL_QueryTexture(*tex, NULL, NULL, &width, &height);

    lua_pushnumber(L, width);
    lua_pushnumber(L, height);
    return 2;
}

int l_poti_texture__gc(lua_State *L) {
    return 0;
}

/*********************************
 * Font
 *********************************/

int luaopen_font(lua_State *L) {
    luaL_Reg reg[] = {
        {"print", l_poti_font_print},
        {"__gc", l_poti_font__gc},
        {NULL, NULL}   
    };

    luaL_newmetatable(L, FONT_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

int l_poti_new_font(lua_State *L) {
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

int l_poti_font_print(lua_State *L) {
    int index = 1;
    Font *font = luaL_checkudata(L, index++, FONT_CLASS);
    if (!font) {
        font = &poti()->default_font;
        --index;
    }

    const i8 *text = luaL_checkstring(L, index++);
    float x, y;
    x = luaL_optnumber(L, index++, 0);
    y = luaL_optnumber(L, index++, 0);

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

    return 0;
}

int l_poti_font__gc(lua_State *L) {
    // te_font_t **font = luaL_checkudata(L, 1, FONT_CLASS);
    // tea_destroy_font(*font);
    return 0;
}

/*********************************
 * Audio
 *********************************/

int luaopen_audio(lua_State *L) {
    luaL_Reg reg[] = {
        {"play", l_poti_audio_play},
        {"stop", l_poti_audio_stop},
        {"pause", l_poti_audio_pause},
        {"playing", l_poti_audio_playing},
        {"volume", l_poti_audio_volume},
        {"pitch", l_poti_audio_pitch},
        {"__gc", l_poti_audio__gc},
        {NULL, NULL}
    };

    luaL_newmetatable(L, AUDIO_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

static int s_register_audio_data(lua_State *L, u8 usage, const char *path) {
    AudioData *adata = NULL;
    lua_rawgetp(L, LUA_REGISTRYINDEX, &_audio_data);
    lua_rawgeti(L, -1, usage);
    lua_remove(L, -2);
    lua_pushstring(L, path);
    lua_gettable(L, -2);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        size_t size;
        void *data = poti()->read_file(path, &size);
        if (!data) {
            int len = strlen(path) + 1;
            char buf[50 + len];
            sprintf(buf, "failed to load audio: %s\n", path);
            lua_pushstring(L, buf);
            lua_error(L);
            return 1;
        }
        adata = malloc(sizeof(*data));
        adata->usage = usage;
        if (!adata) {
            int len = strlen(path) + 1;
            char buf[50 + len];
            sprintf(buf, "failed to alloc memory for audio: %s\n", path);
            lua_pushstring(L, buf);
            lua_error(L);
            return 1;
        }
        if (usage == AUDIO_STATIC) {
            ma_decoder_config dec_config = ma_decoder_config_init(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, AUDIO_DEVICE_SAMPLE_RATE);
            ma_uint64 frame_count_out;
            void *dec_data;
            ma_result result = ma_decode_memory(data, size, &dec_config, &frame_count_out, &dec_data);
            if (result != MA_SUCCESS) {
                luaL_error(L, "Failed to decode audio data");
                return 1;
            }
            adata->data = dec_data;
            adata->size = frame_count_out;
            free(data);
        } else {
            adata->data = data;
            adata->size = size;
        }
        lua_pushstring(L, path);
        lua_pushlightuserdata(L, adata);
        lua_settable(L, -3);
    } else {
        adata = lua_touserdata(L, -1);
    }
    lua_pop(L, 2);
    lua_pushlightuserdata(L, adata);
    return 1;
}

int l_poti_new_audio(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const char *s_usage = luaL_optstring(L, 2, "stream");
    u8 need_register = 0;

    int usage = AUDIO_STREAM;
    if (!strcmp(s_usage, "static")) usage = AUDIO_STATIC;

    s_register_audio_data(L, usage, path);
    AudioData *a_data = lua_touserdata(L, -1);
    lua_pop(L, 1);
    Audio **audio = lua_newuserdata(L, sizeof(*audio));
    luaL_setmetatable(L, AUDIO_CLASS);
    *audio = malloc(sizeof(Audio));
    (*audio)->data.data = a_data->data;
    (*audio)->data.size = a_data->size;
    (*audio)->data.usage = a_data->usage;

    // *buf = mo_audio(data, size, usage);
    // AudioBuffer *buffer = &poti()->audio.buffers[index];

    return 1;
}

int poti_volume(lua_State *L) {
    float volume = luaL_optnumber(L, 1, 0);
    // mo_volume(NULL, volume);

    return 0;
}

int l_poti_audio_play(lua_State *L) {
    Audio **audio = luaL_checkudata(L, 1, AUDIO_CLASS);
    // mo_play(*buf);
    i32 index = 0;
    i32 i;
    for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        if (!poti()->audio.buffers[i].loaded) {
            index = i;
            break;
        }
    }

    AudioBuffer *buffer = &poti()->audio.buffers[index];
    ma_decoder_config dec_config = ma_decoder_config_init(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, AUDIO_DEVICE_SAMPLE_RATE);
    ma_result result = -1;
    AudioData *a_data = &(*audio)->data;
    buffer->usage = a_data->usage;
    buffer->size = a_data->size;
    if (a_data->usage == AUDIO_STREAM) {
        buffer->offset = 0;
        result = ma_decoder_init_memory(a_data->data, a_data->size, &dec_config, &buffer->decoder);
        buffer->data = a_data->data;
        ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
        buffer->offset = 0;
    } else {
        buffer->data = a_data->data;
        result = 0;
    }

    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to load sound %s\n", ma_result_description(result));
        exit(0);
    }

    buffer->loaded = 1;
    buffer->playing = 1;
    buffer->loop = 0;
    buffer->paused = 0;

    return 0;
}

int l_poti_audio_stop(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    // mo_stop(*buf);

    return 0;
}

int l_poti_audio_pause(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    // mo_pause(*buf);
    return 0;
}

int l_poti_audio_playing(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    lua_pushboolean(L, 1);
    return 1;
}

int l_poti_audio_volume(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    float volume = luaL_optnumber(L, 2, 0);
    // mo_volume(*buf, volume);
    return 0;
}

int l_poti_audio_pitch(lua_State *L) { return 0; }

int l_poti_audio__gc(lua_State *L) {
    Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    // mo_audio_destroy(*buf); 
    return 0;
}

/*********************************
 * Draw
 *********************************/

int l_poti_clear(lua_State *L) {
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

int l_poti_mode(lua_State *L) {
    const char *mode_str = luaL_checkstring(L, 1);
    int mode = 0;
    if (!strcmp(mode_str, "line")) mode = 0;
    else if (!strcmp(mode_str, "fill")) mode = 1;
    poti()->draw_mode = mode;
    return 0;
}

int l_poti_color(lua_State *L) {
    int args = lua_gettop(L);

    color_t col = {0, 0, 0, 255};
    for (int i = 0; i < args; i++) { 
        col[i] = luaL_checknumber(L, i+1);
    }
    SDL_SetRenderDrawColor(poti()->render, col[0], col[1], col[2], col[3]);
    memcpy(&poti()->color, &col, sizeof(color_t));

    return 0;
}

int l_poti_set_target(lua_State *L) {
    Texture **tex = luaL_testudata(L, 1, TEXTURE_CLASS);
    Texture *t = NULL;
    if (tex) t = *tex;

    SDL_SetRenderTarget(poti()->render, t);
    return 0;
}

int l_poti_draw_point(lua_State *L) {
    float x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);

    SDL_RenderDrawPointF(poti()->render, x, y);
    return 0;
}

int l_poti_draw_line(lua_State *L) {
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

int l_poti_draw_circle(lua_State *L) {

    float x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);

    float radius = luaL_checknumber(L, 3);

    float segments = luaL_optnumber(L, 4, 32);

    DrawCircle fn[2] = {
        draw_lined_circle,
        draw_filled_circle
    };

    fn[(int)poti()->draw_mode](x, y, radius, segments);

    return 0;
}

typedef void(*DrawRect)(SDL_Rect*);

static void draw_filled_rect(SDL_Rect *r) {
    SDL_RenderFillRect(poti()->render, r);
}

static void draw_lined_rect(SDL_Rect *r) {
    SDL_RenderDrawRect(poti()->render, r);
}

int l_poti_draw_rectangle(lua_State *L) {

    SDL_Rect r = {0, 0, 0, 0};

    r.x = luaL_checknumber(L, 1);
    r.y = luaL_checknumber(L, 2);
    r.w = luaL_checknumber(L, 3);
    r.h = luaL_checknumber(L, 4);

    DrawRect fn[2] = {
        draw_lined_rect,
        draw_filled_rect
    };

    fn[(int)poti()->draw_mode](&r);
    
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

int l_poti_draw_triangle(lua_State *L) {

    float points[6];

    for (int i = 0; i < 6; i++) {
        points[i] = luaL_checknumber(L, i+1);
    }

    const DrawTriangle fn[] = {
        draw_lined_triangle,
        draw_filled_triangle
    };

    fn[(int)poti()->draw_mode](points[0], points[1], points[2], points[3], points[4], points[5]);

    return 0;
}

void s_char_rect(Font *font, const i32 c, f32 *x, f32 *y, SDL_Point *out_pos, SDL_Rect *rect, int width) {
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

int l_poti_print(lua_State *L) {
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
        {"down", l_poti_keyboard_down},
        {"up", l_poti_keyboard_up},
        //{"pressed", poti_key_pressed},
        //{"released", poti_key_released},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}

int luaopen_mouse(lua_State *L) {
    luaL_Reg reg[] = {
        {"pos", l_poti_mouse_pos},
        {"down", l_poti_mouse_down},
        {"up", l_poti_mouse_up},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}

int luaopen_joystick(lua_State *L) {
    luaL_Reg reg[] = {
        {"num_axes", l_poti_joystick_num_axes},
        {"num_hats", l_poti_joystick_num_hats},
        {"num_balls", l_poti_joystick_num_balls},
        {"num_buttons", l_poti_joystick_num_buttons},
        {"axis", l_poti_joystick_axis},
        {"button", l_poti_joystick_button},
        {"hat", l_poti_joystick_hat},
        {"ball", l_poti_joystick_ball},
        {"name", l_poti_joystick_name},
        {"vendor", l_poti_joystick_vendor},
        {"product", l_poti_joystick_product},
        {"product_version", l_poti_joystick_product_version},
        {"type", l_poti_joystick_type},
        {"rumble", l_poti_joystick_rumble},
        {"powerlevel", l_poti_joystick_powerlevel},
        {"__gc", l_poti_joystick__gc},
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
    luaL_newmetatable(L, GAMEPAD_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

int l_poti_keyboard_down(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = SDL_GetScancodeFromName(key);
    lua_pushboolean(L, poti()->keys[code]);
    
    return 1;
}

int l_poti_keyboard_up(lua_State *L) {
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

int l_poti_mouse_pos(lua_State *L) {
    int x, y;
    SDL_GetMouseState(&x, &y);

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

static int mouse_down(int btn) {
    return SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(btn);
}

int l_poti_mouse_down(lua_State *L) {
    int button = luaL_checknumber(L, 1);

    lua_pushboolean(L, mouse_down(button));
    
    return 1;
}

int l_poti_mouse_up(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, !mouse_down(button));
    
    return 1;
}

int l_poti_mouse_pressed(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, mouse_down(button));
    
    return 1;
}

int l_poti_mouse_released(lua_State *L) {
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

int l_poti_joystick_name(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushstring(L, SDL_JoystickName(*j));
    return 1;
}

int l_poti_joystick_vendor(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickGetVendor(*j));
    return 1;
}

int l_poti_joystick_product(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickGetProduct(*j));
    return 1;
}

int l_poti_joystick_product_version(lua_State *L) {
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

int l_poti_joystick_type(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushstring(L, joy_types[SDL_JoystickGetType(*j)]);
    return 1;
}


int l_poti_joystick_num_axes(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumAxes(*j));
    return 1;
}

int l_poti_joystick_num_balls(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumBalls(*j));
    return 1;
}

int l_poti_joystick_num_hats(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumHats(*j));
    return 1;
}

int l_poti_joystick_num_buttons(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumButtons(*j));
    return 1;
}
int l_poti_joystick_axis(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int axis = luaL_checknumber(L, 2);
    lua_pushnumber(L, SDL_JoystickGetAxis(*j, axis));
    return 1;
}

int l_poti_joystick_ball(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int ball = luaL_checknumber(L, 2);
    int dx, dy;
    SDL_JoystickGetBall(*j, ball, &dx, &dy);
    lua_pushnumber(L, dx);
    lua_pushnumber(L, dy);
    return 2;
}

int l_poti_joystick_hat(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int hat = luaL_checknumber(L, 2);
    lua_pushnumber(L, SDL_JoystickGetHat(*j, hat));
    return 1;
}

int l_poti_joystick_button(lua_State* L) {
    Joystick** j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int button = luaL_checknumber(L, 2);
    int res = SDL_JoystickGetButton(*j, button);

    lua_pushboolean(L, res);
    return 1;
}

int l_poti_joystick_rumble(lua_State *L) {
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

int l_poti_joystick_powerlevel(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushstring(L, joy_powerlevels[SDL_JoystickCurrentPowerLevel(*j) + 1]);
    return 1;
}

int l_poti_joystick_close(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    if (SDL_JoystickGetAttached(*j)) {
        SDL_JoystickClose(*j);
    }
    return 0;
}

int l_poti_joystick__gc(lua_State* L) {
    l_poti_joystick_close(L);
    return 0;
}

/* Game Controller */
int l_poti_gamepad(lua_State *L) {
    GameController **g = lua_newuserdata(L, sizeof(*g));
    *g = SDL_GameControllerOpen(luaL_checknumber(L, 1));
    luaL_setmetatable(L, GAMEPAD_CLASS);
    return 1;
}

int l_poti_is_gamepad(lua_State *L) {
    lua_pushboolean(L, SDL_IsGameController(luaL_checknumber(L, 1)));
    return 1;
}

int l_poti_gamepad_name(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushstring(L, SDL_GameControllerName(*g));
    return 1;
}

int l_poti_gamepad_vendor(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushnumber(L, SDL_GameControllerGetVendor(*g));
    return 1;
}

int l_poti_gamepad_product(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushnumber(L, SDL_GameControllerGetProduct(*g));
    return 1;
}

int l_poti_gamepad_product_version(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushnumber(L, SDL_GameControllerGetProductVersion(*g));
    return 1;
}

const char *gpad_axes[] = {
    "leftx", "lefty", "rightx", "righty", "triggerleft", "triggerright"
};

int l_poti_gamepad_axis(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    const char *str = luaL_checkstring(L, 2);
    int axis = SDL_GameControllerGetAxisFromString(str);
    if (axis < 0) {
        luaL_argerror(L, 2, "invalid axis");
    }
    lua_pushnumber(L, SDL_GameControllerGetAxis(*g, axis));
    return 1;
}

int l_poti_gamepad_button(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    const char *str = luaL_checkstring(L, 2);
    int button = SDL_GameControllerGetButtonFromString(str);
    if (button < 0) {
        luaL_argerror(L, 2, "invalid button");
    }
    lua_pushboolean(L, SDL_GameControllerGetButton(*g, button));
    return 1;
}

int l_poti_gamepad_rumble(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    u16 low = luaL_checknumber(L, 2);
    u16 high = luaL_checknumber(L, 3);
    u32 freq = luaL_optnumber(L, 4, 100);
    lua_pushboolean(L, SDL_GameControllerRumble(*g, low, high, freq) == 0);
    return 1;
}

int l_poti_gamepad_close(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    if (SDL_GameControllerGetAttached(*g)) {
        SDL_GameControllerClose(*g);
    }
    return 0;
}

int l_poti_gamepad__gc(lua_State* L) {
    l_poti_gamepad_close(L);
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
    i8 *str = malloc(sz);
    fread(str, 1, sz, fp);
    fclose(fp);

    return str;
}

static i8* s_read_file_packed(const i8 *filename, size_t *size) {
    sbtar_t *tar = &poti()->pkg;
    if (!sbtar_find(tar, filename)) return NULL;
    sbtar_header_t h;
    sbtar_header(tar, &h);
    i8 *str = malloc(h.size);
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

static i32 s_init_font(Font *font, const void *data, size_t buf_size, i32 font_size) {
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

    i32 ascent, descent, line_gap;
    font->size = font_size;
    f32 fsize = font_size;
    font->scale = stbtt_ScaleForMappingEmToPixels(&font->info, fsize);
    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
    font->baseline = ascent * font->scale;

    i32 tw = 0, th = 0;

    i32 i;
    for (i = 0; i < 256; i++) {
        i32 ax, bl;
        i32 x0, y0, x1, y1;
        i32 w, h;

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
    return 1;
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
    ctx->load_font = s_load_font;
    ctx->utf8_codepoint = s_utf8_codepoint;

    return 1;
}

u32 s_read_and_mix_pcm_frames(AudioBuffer *buffer, f32 *output, u32 frames) {
    f32 temp[4096];
    u32 temp_cap_in_frames = ma_countof(temp) / AUDIO_DEVICE_CHANNELS;
    u32 total_frames_read = 0;
    ma_decoder *decoder = &buffer->decoder;
    f32 volume = buffer->volume;
    f32 size = buffer->size * ma_get_bytes_per_frame(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS);

    while (total_frames_read < frames) {
        u32 sample;
        u32 frames_read_this_iteration;
        u32 total_frames_remaining = frames - total_frames_read;
        u32 frames_to_read_this_iteration = temp_cap_in_frames;
        if (frames_to_read_this_iteration > total_frames_remaining) {
            frames_to_read_this_iteration = total_frames_remaining;
        }

        if (buffer->usage == AUDIO_STREAM) {
            frames_read_this_iteration = (u32)ma_decoder_read_pcm_frames(decoder, temp, frames_to_read_this_iteration);
        } else {
            frames_read_this_iteration = frames_to_read_this_iteration;
            u32 aux = frames_to_read_this_iteration * ma_get_bytes_per_frame(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS);
            memcpy(temp, buffer->data + buffer->offset, aux);
            if (buffer->offset > size) frames_read_this_iteration = 0;
            buffer->offset += aux;
        }

        if (frames_read_this_iteration == 0) {
            break;
        }

        for (sample = 0; sample < frames_read_this_iteration * AUDIO_DEVICE_CHANNELS; ++sample) {
            output[total_frames_read * AUDIO_DEVICE_CHANNELS + sample] += temp[sample] * volume;
        }

        total_frames_read += frames_read_this_iteration;

        if (frames_read_this_iteration < frames_to_read_this_iteration) {
            break;
        }
    }

    return total_frames_read;
}

void s_audio_callback(ma_device *device, void *output, const void *input, ma_uint32 frame_count) {
    f32 *out = (f32*)output;
    
    ma_mutex_lock(&poti()->audio.lock);
    i32 i;
    for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer *buffer = &poti()->audio.buffers[i];
        if (buffer->playing && buffer->loaded && !buffer->paused) {
            u32 frames_read = s_read_and_mix_pcm_frames(buffer, out, frame_count);
            if (frames_read < frame_count) {
                if (buffer->loop) {
                    ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
                    buffer->offset = 0;
                } else {
                    buffer->playing = 0;
                }
            }
        }
    }
    ma_mutex_unlock(&poti()->audio.lock);
    (void)input;
}

int main(int argc, char ** argv) {
    poti_init();
    poti_loop();
    poti_quit();
    return 1;
}