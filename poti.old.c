#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "embed.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include "glad/glad.h"

#ifdef _WIN32
#define SDL_MAIN_HANDLED
#ifdef __MINGW32__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#endif

#include "miniaudio.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include "sbtar.h"

#include "microui/src/microui.h"

#include "poti.h"
#include "linmath.h"

#define MAX_AUDIO_BUFFER_CHANNELS 255

#define POTI_VER "0.1.0"

#define POINT_CLASS "Point"
#define RECT_CLASS "Rect"

#define TEXTURE_CLASS "Texture"
#define FONT_CLASS "Font"
#define SHADER_CLASS "Shader"

#define AUDIO_CLASS "Audio"

#define JOYSTICK_CLASS "Joystick"
#define GAMEPAD_CLASS "GamePad"

#define FILE_CLASS "File"

#define POTI_LINE 0
#define POTI_FILL 1

#define poti() (&_ctx)
#define POTI() (&_ctx)
#define GL() (&POTI()->gl)
#define RENDER() (&POTI()->render)
#define TIMER() (&POTI()->timer)
#define INPUT() (&POTI()->input)
#define VERTEX() (RENDER()->vertex)

#define M_PI 3.14159
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(v, a, b) (MAX(a, MIN(v, b)))
#define DEG2RAD(deg) ((deg)*(M_PI/180.0))
#define RAD2DEG(rad) ((rad)*(180.0/M_PI))

#define AUDIO_DEVICE_FORMAT ma_format_f32
#define AUDIO_DEVICE_CHANNELS 2
#define AUDIO_DEVICE_SAMPLE_RATE 44100

#define AUDIO_STREAM 0
#define AUDIO_STATIC 1

typedef u8 color_t[4];

typedef struct Audio Audio;
typedef struct AudioData AudioData;
typedef struct AudioBuffer AudioBuffer;

typedef struct Texture Texture;
typedef struct Font Font;
typedef struct Shader Shader;

typedef struct Vertex Vertex;

typedef struct File File;

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

struct Texture {
    u32 handle;
    u32 fbo;
    i32 width, height;
    i32 wrap[2];
    i32 filter[2];
};

struct Shader {
    u32 handle;
    u32 u_world, u_modelview;
};

struct Vertex {
    u32 vao;
    u32 vbo;
    u32 index, size;
    void *data;
};

struct File {
    FILE *fp;
    u32 index, size;
};

struct Context {
    lua_State *L;
    Window *window;
    Event event;
    //Render *render;
    struct {
        SDL_GLContext context;
        i8 major, minor;
        i16 glsl;
        i8 es;
    } gl;

    struct {
        u32 draw_calls;
        i8 draw_mode;
        float color[4];
        Shader *def_shader;
        Vertex def_vertex;
        Texture def_target;

        Texture *tex;
        Texture *target;

        Texture *white;
        Shader *shader;

        Vertex *vertex;

        void(*init_vertex)(Vertex*, u32);
        void(*bind_vertex)(Vertex*);
        void(*clear_vertex)(Vertex*);

        void(*set_texture)(Texture*);
        void(*set_shader)(Shader*);
        void(*set_target)(Texture*);

        void(*push_vertex)(Vertex*, f32, f32, f32, f32, f32, f32, f32, f32);
        void(*push_vertices)(Vertex*, u32, f32*);
    #if 0
        void(*push_point)(Vertex*, f32, f32);
        void(*push_line)(Vertex*, f32, f32, f32, f32);
        void(*push_triangle)(Vertex*, f32, f32, f32, f32, f32, f32);
        void(*push_rectangle)(Vertex*, f32, f32, f32, f32);
        void(*push_circle)(Vertex*, f32, f32, f32, u8);
    #endif

        void(*draw_vertex)(Vertex*);
    } render;

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
    } timer;

    struct {
        const Uint8 *keys;
    } input;

    const Uint8 *keys;
    sbtar_t pkg;

    i8 is_packed;
    i8*(*read_file)(const i8*, size_t*);

    Font default_font;
    Font *font;
    i32(*init_font)(Font*, const void*, size_t, i32);
    i32(*init_texture)(Texture*, i32, i32, i32, i32, void*);

    u8*(*utf8_codepoint)(u8*, i32*);

    i32(*flip_from_table)(lua_State*, i32, i32*);
    i32(*point_from_table)(lua_State*, i32, SDL_Point*);
    i32(*rect_from_table)(lua_State*, i32, SDL_Rect*);

    char basepath[512];

    mu_Context ui;
};

struct Context _ctx;
static const i8 lr_callbacks;
static const i8 lr_step;
static const i8 lr_audio_data;
static const i8 lr_meta;

static const i8 *_err_func =
"function poti.error(msg, trace)\n"
"   local trace = debug.traceback('', 1)\n"
"    poti.update = function() end\n"
"    poti.draw = function()\n"
"       poti.render.clear(0, 0, 0)\n"
"       poti.render.print('error', 32)\n"
"       poti.render.print(msg, 32, 16)\n"
"       poti.render.print(trace, 32, 32)\n"
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
"local path = poti.filesystem.base_path()\n"
"package.path = path .. '/?.lua;' .. path .. '/?/init.lua;' .. package.path\n" 
"local main_state = false\n"
"local function _err(msg)\n"
"   local trace = traceback('', 1)\n"
"   print(msg, trace)\n"
"   poti.error(msg, trace)\n"
"end\n"
"local function _mainLoop()\n"
"   if poti.update then poti.update(poti.timer.delta()) end\n"
"   if poti.draw then poti.draw() end\n"
"   poti.gui_begin()\n"
"   if poti.draw_gui then poti.draw_gui() end\n"
"   poti.gui_end()\n"
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

#if 0
const char *vert_shader =
"in vec2 in_Pos;\n"
"in vec4 in_Color;\n"
"in vec2 in_Texcoord;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_Texcoord;\n"
"uniform mat4 u_World;\n"
"uniform mat4 u_Modelview;\n"
"void main() {\n"
"  gl_Position = u_World * u_Modelview * vec4(in_Pos.x, in_Pos.y, 0.0, 1.0);\n"
"  v_Color = in_Color;\n"
"  v_Texcoord = in_Texcoord;\n"
"}";

const char *frag_shader =
"out vec4 FragColor;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_Texcoord;\n"
"uniform sampler2D u_Texture;\n"
"void main() {\n"
"  FragColor = v_Color * texture2D(u_Texture, v_Texcoord);\n"
"}";
#endif
const char *vert_shader =
"vec4 position(vec2 pos, mat4 world, mat4 modelview) {\n"
"   return world * modelview * vec4(pos.x, pos.y, 0, 1);\n"
"}";

const char *frag_shader =
"vec4 pixel(vec4 color, vec2 uv, sampler2D tex) {\n"
"   return color * texture(tex, uv);\n"
"}";

static int s_get_rect_from_table(lua_State* L, int idx, SDL_Rect* out);
static int s_get_point_from_table(lua_State* L, int idx, SDL_Point* out);
static void s_char_rect(Font *font, const i32 c, f32 *x, f32 *y, SDL_Point *out_pos, SDL_Rect *rect, int width);
static u32 s_read_and_mix_pcm_frames(AudioBuffer *buffer, f32 *output, u32 frames);
static void s_audio_callback(ma_device *device, void *output, const void *input, ma_uint32 frameCount);

static void s_font_print(Font* font, float x, float y, const char* text);
static i32 s_get_text_width(Font *font, const i8* text, i32 len);
static i32 s_get_text_height(Font *font);

static int s_compile_shader(const char *source, int type);
static int s_load_program(int vert, int frag);

static int s_setup_function_ptrs(struct Context *ctx);

static int poti_call(const char *name, int args, int ret);

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
#if SDL_VERSION_ATLEAST(2, 0, 14)
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
*           Filesystem            *
*=================================*/
// static int l_poti_file(lua_State *L);
static int l_poti_file_read(lua_State *L);
static int l_poti_file_write(lua_State *L);
static int l_poti_file_seek(lua_State *L);
static int l_poti_file_tell(lua_State *L);
static int poti_list_folder(lua_State *L);


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

/*=================================*
*              Core               *
*=================================*/
static int l_poti_ver(lua_State *L) {
    lua_pushstring(L, POTI_VER);
    return 1;
}

static int l_poti_meta(lua_State *L) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_meta);
    lua_getfield(L, -1, luaL_checkstring(L, 1));
    lua_remove(L, -2);
    return 1;
}

/*=================================*
*              Timer              *
*=================================*/

static int l_poti_timer_delta(lua_State *L) {
    lua_pushnumber(L, poti()->timer.delta);
    return 1;
}

static int l_poti_timer_ticks(lua_State *L) {
    lua_pushinteger(L, SDL_GetTicks());
    return 1;
}

static int l_poti_timer_delay(lua_State *L) {
    u32 ms = luaL_checkinteger(L, 1);
    SDL_Delay(ms);
    return 0;
}

static int l_poti_timer_performance_counter(lua_State *L) {
    lua_pushinteger(L, SDL_GetPerformanceCounter());
    return 1;
}

static int l_poti_timer_performance_freq(lua_State *L) {
    lua_pushinteger(L, SDL_GetPerformanceFrequency());
    return 1;
}

static int luaopen_timer(lua_State *L) {
    luaL_Reg reg[] = {
        {"delta", l_poti_timer_delta},
        {"ticks", l_poti_timer_ticks},
        {"delay", l_poti_timer_delay},
        {"performance_counter", l_poti_timer_performance_counter},
        {"performance_freq", l_poti_timer_performance_freq},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    return 1;
}

/*=================================*
*              Event              *
*=================================*/

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

static const char key_map[256] = {
    [SDLK_LSHIFT & 0xff] = MU_KEY_SHIFT,
    [SDLK_RSHIFT & 0xff] = MU_KEY_SHIFT,
    [SDLK_LCTRL & 0xff] = MU_KEY_CTRL,
    [SDLK_RCTRL & 0xff] = MU_KEY_CTRL,
    [SDLK_LALT & 0xff] = MU_KEY_ALT,
    [SDLK_RALT & 0xff] = MU_KEY_ALT,
    [SDLK_RETURN & 0xff] = MU_KEY_RETURN,
    [SDLK_BACKSPACE & 0xff] = MU_KEY_BACKSPACE,
};

int l_poti_callback_keypressed(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    i32 c = key_map[event->key.keysym.sym & 0xff];
    mu_input_keydown(&poti()->ui, c);
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
    i32 c = key_map[event->key.keysym.sym & 0xff];
    mu_input_keyup(&poti()->ui, c);
    lua_pushstring(L, SDL_GetKeyName(event->key.keysym.sym));
    poti_call("key_released", 1, 0);
    return 0;
}

int l_poti_callback_mousemotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    mu_input_mousemove(&poti()->ui, event->motion.x, event->motion.y);
    lua_pushnumber(L, event->motion.x);
    lua_pushnumber(L, event->motion.y);
    lua_pushnumber(L, event->motion.xrel);
    lua_pushnumber(L, event->motion.yrel);
    lua_pushnumber(L, event->motion.state);
    poti_call("mouse_motion", 5, 0);
    return 0;
}

static const char button_map[256] = {
    [SDL_BUTTON_LEFT & 0xff] = MU_MOUSE_LEFT,
    [SDL_BUTTON_RIGHT & 0xff] = MU_MOUSE_RIGHT,
    [SDL_BUTTON_MIDDLE & 0xff] = MU_MOUSE_MIDDLE,
};

int l_poti_callback_mousebutton(lua_State* L) {
    SDL_Event *e = lua_touserdata(L, 1);
    i32 b = button_map[e->button.button & 0xff];
    if (e->type == SDL_MOUSEBUTTONDOWN) mu_input_mousedown(&poti()->ui, e->button.x, e->button.y, b);
    else mu_input_mouseup(&poti()->ui, e->button.x, e->button.y, b);
    lua_pushnumber(L, e->button.button);
    lua_pushboolean(L, e->button.state);
    lua_pushnumber(L, e->button.clicks);
    poti_call("mouse_button", 3, 0);
    return 0;
}

int l_poti_callback_mousewheel(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    mu_input_scroll(&poti()->ui, event->wheel.x, event->wheel.y);
    lua_pushnumber(L, event->wheel.x);
    lua_pushnumber(L, event->wheel.y);
    poti_call("mouse_wheel", 2, 0);
    return 0;
}

int l_poti_callback_joyaxismotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->jaxis.which);
    lua_pushnumber(L, event->jaxis.axis);
    lua_pushnumber(L, event->jaxis.value);
    poti_call("joy_axismotion", 3, 0);
    return 0;
}

int l_poti_callback_joyballmotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->jball.which);
    lua_pushnumber(L, event->jball.ball);
    lua_pushnumber(L, event->jball.xrel);
    lua_pushnumber(L, event->jball.yrel);
    poti_call("joy_ballmotion", 4, 0);
    return 0;
}

int l_poti_callback_joyhatmotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->jhat.which);
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
    lua_pushinteger(L, event->jdevice.which);
    lua_pushboolean(L, SDL_JOYDEVICEREMOVED - event->jdevice.type);
    poti_call("joy_device", 2, 0);
    return 0;
}

int l_poti_callback_gamepadaxismotion(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->caxis.which);
    lua_pushstring(L, SDL_GameControllerGetStringForAxis(event->caxis.axis));
    lua_pushnumber(L, event->caxis.value);
    poti_call("gpad_axismotion", 3, 0);
    return 0;
}

int l_poti_callback_gamepadbutton(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->cbutton.which);
    lua_pushstring(L, SDL_GameControllerGetStringForButton(event->cbutton.button));
    lua_pushboolean(L, event->cbutton.state);
    poti_call("gpad_button", 3, 0);
    return 0;
}

int l_poti_callback_gamepaddevice(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->cdevice.which);
    lua_pushboolean(L, SDL_CONTROLLERDEVICEREMOVED - event->cdevice.type);
    poti_call("gpad_device", 2, 0);
    return 0;
}

int l_poti_callback_gamepadremap(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->cdevice.which);
    lua_pushstring(L, SDL_GameControllerNameForIndex(event->cdevice.which));
    poti_call("gpad_remap", 2, 0);
    return 0;
}


#if SDL_VERSION_ATLEAST(2, 0, 14)
int l_poti_callback_gamepadtouchpad(lua_State* L) {
// SDL_Event *event = lua_touserdata(L, 1);
    return 0;
}

int l_poti_callback_gamepadtouchpadmotion(lua_State* L) {

    return 0;
}
#endif

static int l_poti_callback_finger(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->tfinger.touchId);
    lua_pushboolean(L, SDL_FINGERUP - event->type);
    lua_pushinteger(L, event->tfinger.fingerId);
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
    lua_pushinteger(L, event->tfinger.touchId);
    lua_pushinteger(L, event->tfinger.fingerId);
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
    lua_pushinteger(L, event->dgesture.touchId);
    lua_pushinteger(L, event->dgesture.gestureId);
    lua_pushinteger(L, event->dgesture.numFingers);
    lua_pushnumber(L, event->dgesture.error);
    lua_pushnumber(L, event->dgesture.x);
    lua_pushnumber(L, event->dgesture.y);
    poti_call("dgesture", 6, 0);
    return 0;
}

static int l_poti_callback_dollarrecord(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->dgesture.touchId);
    lua_pushinteger(L, event->dgesture.gestureId);
    lua_pushinteger(L, event->dgesture.numFingers);
    lua_pushnumber(L, event->dgesture.error);
    lua_pushnumber(L, event->dgesture.x);
    lua_pushnumber(L, event->dgesture.y);
    poti_call("dgesture_record", 6, 0);
    return 0;
}

int l_poti_callback_multigesture(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->mgesture.touchId);
    lua_pushinteger(L, event->mgesture.numFingers);
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
    lua_pushinteger(L, event->drop.windowID);
    lua_pushstring(L, event->drop.file);
    poti_call("drop_file", 1, 0);
    return 0;
}

int l_poti_callback_droptext(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->drop.windowID);
    lua_pushstring(L, event->drop.file);
    poti_call("drop_text", 1, 0);
    return 0;
}

int l_poti_callback_dropbegin(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->drop.windowID);
    poti_call("drop_begin", 1, 0);
    return 0;
}

int l_poti_callback_dropcomplete(lua_State* L) {
    SDL_Event *event = lua_touserdata(L, 1);
    lua_pushinteger(L, event->drop.windowID);
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
#if SDL_VERSION_ATLEAST(2, 0, 22)
    [SDL_WINDOWEVENT_ICCPROF_CHANGED] = poti_windowevent_null,
    [SDL_WINDOWEVENT_DISPLAY_CHANGED] = poti_windowevent_null,
#endif
};

int l_poti_callback_windowevent(lua_State *L) {
    SDL_Event* event = lua_touserdata(L, 1);
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

static int luaopen_event(lua_State *L) {
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

    i32 i;
    for (i = 0; fn[i].type != -1; i++) {
        lua_pushinteger(L, fn[i].type);
        lua_pushcfunction(L, fn[i].fn);
        lua_settable(L, -3);
    }
    lua_rawsetp(L, LUA_REGISTRYINDEX, &lr_callbacks);

    luaL_Reg reg[] = {
        {NULL, NULL}
    };

    luaL_newlib(L, reg);


    return 1;
}

/*=================================*
*           Filesystem            *
*=================================*/

static int l_poti_file(lua_State *L) {
	FILE **fp = lua_newuserdata(L, sizeof(FILE*));
    luaL_setmetatable(L, FILE_CLASS);

    const char *filename = luaL_checkstring(L, 1);
	const char *mode = luaL_optstring(L, 2, "rb");

    lua_pushstring(L, POTI()->basepath);
    lua_pushstring(L, "/");
    lua_pushstring(L, filename);
    lua_concat(L, 3);
	const char *rpath = lua_tostring(L, -1);
	*fp = fopen(rpath, mode);
	lua_pop(L, 1);
    if (!*fp) {
        lua_pushstring(L, "Failed to open file");
        lua_error(L);
        return 1;
    }

    return 1;
}

static int l_poti_file__read(lua_State *L) {
	FILE **f = (FILE**)luaL_checkudata(L, 1, FILE_CLASS);
	size_t size;
	fseek(*f, 0, SEEK_END);
	size = ftell(*f);
	fseek(*f, 0, SEEK_SET);

	i8 str[size+1];
	fread(str, 1, size, *f);
	str[size] = '\0';

	lua_pushstring(L, str);
		
	return 1;
}

static int l_poti_file__write(lua_State *L) {
	return 0;
}

static int l_poti_file__close(lua_State *L) {
	FILE **f = (FILE**)luaL_checkudata(L, 1, FILE_CLASS);
	if (*f) {
		fclose(*f);
		*f = NULL;
	}
	return 0;
}

static int l_poti_file__gc(lua_State *L) {
	FILE **fp = (FILE**)luaL_checkudata(L, 1, FILE_CLASS);
	if (*fp) {
		fclose(*fp);
		*fp = NULL;
	}
	return 0;
}

static int l_poti_filesystem_base_path(lua_State *L) {
    lua_pushstring(L, poti()->basepath);
    return 1;
}

static int l_poti_filesystem_pref_path(lua_State *L) {
    return 0;
}

static int l_poti_filesystem_read(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    char *content = poti()->read_file(filename, NULL);

    lua_pushstring(L, content);
    free(content);

    return 1;
}

static int l_poti_filesystem_write(lua_State *L) {
    return 0;
}

static int luaopen_filesystem(lua_State *L) {
    luaL_Reg reg[] = {
        {"base_path", l_poti_filesystem_base_path},
        {"pref_path", l_poti_filesystem_pref_path},
        {"read", l_poti_filesystem_read},
        {"write", l_poti_filesystem_write},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}

/*=================================*
*              Audio              *
*=================================*/

static int s_register_audio_data(lua_State *L, u8 usage, const char *path) {
    AudioData *adata = NULL;
    lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_audio_data);
    lua_rawgeti(L, -1, usage);
    lua_remove(L, -2);
    lua_pushstring(L, path);
    lua_gettable(L, -2);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        size_t size;
        void *data = poti()->read_file(path, &size);
        if (!data) {
            lua_pushstring(L, "failed to load audio: ");
            lua_pushstring(L, path);
            lua_concat(L, 2);
            lua_error(L);
            return 1;
        }
        adata = (AudioData*)malloc(sizeof(*adata));
        if (!adata)
        {
            fprintf(stderr, "Failed to alloc memory for Audio Data\n");
            exit(EXIT_FAILURE);
        }
        adata->usage = usage;
        if (!adata) {
            lua_pushstring(L, "failed to alloc memory for audio: ");
            lua_pushstring(L, path);
            lua_concat(L, 2);
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

static int l_poti_new_audio(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const char *s_usage = luaL_optstring(L, 2, "stream");

    int usage = AUDIO_STREAM;
    if (!strcmp(s_usage, "static")) usage = AUDIO_STATIC;

    s_register_audio_data(L, usage, path);
    AudioData *a_data = lua_touserdata(L, -1);
    lua_pop(L, 1);
    Audio *audio = lua_newuserdata(L, sizeof(*audio));
    luaL_setmetatable(L, AUDIO_CLASS);
    audio->data.data = a_data->data;
    audio->data.size = a_data->size;
    audio->data.usage = a_data->usage;

    return 1;
}

static int poti_volume(lua_State *L) {
// float volume = luaL_optnumber(L, 1, 0);
// mo_volume(NULL, volume);
    return 0;
}

static int l_poti_audio__play(lua_State *L) {
    Audio *audio = luaL_checkudata(L, 1, AUDIO_CLASS);
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
    AudioData *a_data = &(audio->data);
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

static int l_poti_audio__stop(lua_State *L) {
// Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
// mo_stop(*buf);
    return 0;
}

static int l_poti_audio__pause(lua_State *L) {
// Audio **buf = luaL_checkudata(L, 1, AUDIO_CLASS);
// mo_pause(*buf);
    return 0;
}

static int l_poti_audio__playing(lua_State *L) {
    //Audio *buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    lua_pushboolean(L, 1);
    return 1;
}

static int l_poti_audio__volume(lua_State *L) {
    //Audio *buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    //float volume = luaL_optnumber(L, 2, 0);
    // mo_volume(*buf, volume);
    return 0;
}

static int l_poti_audio__pitch(lua_State *L) { return 0; }

int l_poti_audio__gc(lua_State *L) {
    //Audio *buf = luaL_checkudata(L, 1, AUDIO_CLASS);
    // mo_audio_destroy(*buf); 
    return 0;
}

static int l_register_audio_meta(lua_State *L) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_meta);
    luaL_Reg reg[] = {
        {"play", l_poti_audio__play},
        {"stop", l_poti_audio__stop},
        {"pause", l_poti_audio__pause},
        {"playing", l_poti_audio__playing},
        {"volume", l_poti_audio__volume},
        {"pitch", l_poti_audio__pitch},
        {"__gc", l_poti_audio__gc},
        {NULL, NULL}
    };

    luaL_newmetatable(L, AUDIO_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_setfield(L, -2, AUDIO_CLASS);
    lua_pop(L, 1);

    return 0;
}

/*=================================*
 *             Render              *
 *=================================*/

/*********************************
 * Texture
 *********************************/

static int s_textype_from_string(lua_State *L, const char *name) {
    int usage = SDL_TEXTUREACCESS_STATIC;
    if (!strcmp(name, "static")) usage = SDL_TEXTUREACCESS_STATIC;
    else if (!strcmp(name, "stream")) usage = SDL_TEXTUREACCESS_STREAMING;
    else if (!strcmp(name, "target")) usage = SDL_TEXTUREACCESS_TARGET;
    else {
        lua_pushstring(L, "Invalid texture usage");
        return -1;
    }

    return usage;
}

static int s_init_texture(Texture *tex, int target, int w, int h, int format, void *data) {
    glGenTextures(1, &tex->handle);
    glBindTexture(GL_TEXTURE_2D, tex->handle);

    tex->fbo = 0;
    tex->width = w;
    tex->height = h;
    tex->filter[0] = GL_NEAREST;
    tex->filter[1] = GL_NEAREST;
    tex->wrap[0] = GL_CLAMP_TO_BORDER;
    tex->wrap[1] = GL_CLAMP_TO_BORDER;


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->filter[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex->filter[1]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->wrap[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->wrap[1]);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, GL_FALSE, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (target) {
        // tex->target = 1;
        glGenFramebuffers(1, &tex->fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, tex->fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->handle, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "failed to create framebuffer\n");
            exit(EXIT_FAILURE);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    return 1;
}

static int s_texture_from_path(lua_State *L, Texture *tex) {
    size_t size;
    const char *path = luaL_checklstring(L, 1, &size);

    int w, h, format, req_format;
    req_format = STBI_rgb_alpha;

    size_t fsize;
    u8 *img = (u8*)poti()->read_file(path, &fsize);
    if (!img) {
        int len = strlen(path) + 50;
#ifdef _WIN32
        char* err = malloc(len);
        sprintf_s(err, "failed to open image: %s\n", path);
#else
        char err[len];
        sprintf(err, "failed to open image: %s\n", path);
#endif
        lua_pushstring(L, err);
#ifdef _WIN32
        free(err);
#endif
        lua_error(L);
        return 1;
    }

    u8 *pixels = stbi_load_from_memory(img, fsize, &w, &h, &format, req_format);
    int pixel_format = GL_RGB;
    switch (req_format) {
        case STBI_rgb: pixel_format = GL_RGB; break;
        case STBI_rgb_alpha: pixel_format = GL_RGBA; break;
    }
    s_init_texture(tex, 0, w, h, pixel_format, pixels);
    stbi_image_free(pixels);
    free(img);
    if (!tex->handle) {
        lua_pushstring(L, "Failed to create GL texture");
        lua_error(L);
        return 1;
    }

    return 1;
}

static int s_texture_from_size(lua_State *L, Texture *tex) {
    int top = lua_gettop(L)-1;
    float w, h;
    w = luaL_checknumber(L, 1);
    h = luaL_checknumber(L, 2);
    
    const char* s_usage = NULL;
    if (top > 1 && lua_type(L, top) == LUA_TSTRING)
        s_usage = luaL_checkstring(L, top);

    int usage = s_textype_from_string(L, s_usage);
    s_init_texture(tex, usage, w, h, GL_RGBA, NULL);
    // *tex = tea_texture(NULL, w, h, TEA_RGBA, usage);
    // *tex = SDL_CreateTexture(poti()->render, SDL_PIXELFORMAT_RGBA32, usage, w, h);
    if (!tex->handle) {
        fprintf(stderr, "Failed to load texture: %s\n", SDL_GetError());
        exit(0);
    }

    return 1;
}

int l_poti_new_texture(lua_State *L) {
    Texture *tex = lua_newuserdata(L, sizeof(*tex));
    luaL_setmetatable(L, TEXTURE_CLASS);
    tex->fbo = 0;

    switch(lua_type(L, 1)) {
        case LUA_TSTRING: s_texture_from_path(L, tex); break;
        case LUA_TNUMBER: s_texture_from_size(L, tex); break;
    }
    
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


static int l_poti_texture__draw(lua_State *L) {
    Texture *tex = luaL_checkudata(L, 1, TEXTURE_CLASS);
    i32 index = 2;
    f32 w, h;
    w = tex->width;
    h = tex->height;
    RENDER()->set_texture(tex);

    SDL_Rect s = {0, 0, w, h};
    s_get_rect_from_table(L, index++, &s);

    f32 uv[4];
    uv[0] = (f32)s.x / w;
    uv[1] = (f32)s.y / h;
    uv[2] = (f32)s.w / w;
    uv[3] = (f32)s.h / h;

    SDL_Rect d = { 0, 0, s.w, s.h };
    s_get_rect_from_table(L, index++, &d);

    float *c = RENDER()->color;
    vec2 tex_y = {uv[1], uv[1] + uv[3]};
    if (tex->fbo != 0) {
        tex_y[0] = 1.f - uv[1];
        tex_y[1] = (1.f - uv[1]) - uv[3];
    }

    float vertices[] = {
        d.x, d.y, c[0], c[1], c[2], c[3], uv[0], tex_y[0],
        d.x + d.w, d.y, c[0], c[1], c[2], c[3], uv[0] + uv[2], tex_y[0],
        d.x + d.w, d.y + d.h, c[0], c[1], c[2], c[3], uv[0] + uv[2], tex_y[1],

        d.x, d.y, c[0], c[1], c[2], c[3], uv[0], tex_y[0],
        d.x, d.y + d.h, c[0], c[1], c[2], c[3], uv[0], tex_y[1],
        d.x + d.w, d.y + d.h, c[0], c[1], c[2], c[3], uv[0] + uv[2], tex_y[1],
    };

    if (index > 3) {
        //float angle = luaL_optnumber(L, index++, 0);

        SDL_Point origin = {0, 0};
        s_get_point_from_table(L, index++, &origin);

        int flip = SDL_FLIP_NONE;
        get_flip_from_table(L, index++, &flip);

        // SDL_RenderCopyEx(poti()->render, *tex, &s, &d, angle, &origin, flip);
    } else {
        // SDL_RenderCopy(poti()->render, *tex, &s, &d);
    }
    RENDER()->push_vertices(VERTEX(), 6, vertices);

    return 0;
}

static int l_poti_texture__width(lua_State *L) {
    Texture *tex = luaL_checkudata(L, 1, TEXTURE_CLASS);
    lua_pushnumber(L, tex->width);
    return 1;
}

static int l_poti_texture__height(lua_State *L) {
    Texture *tex = luaL_checkudata(L, 1, TEXTURE_CLASS);
    lua_pushnumber(L, tex->height);
    return 1;
}

static int l_poti_texture__size(lua_State *L) {
    Texture *tex = luaL_checkudata(L, 1, TEXTURE_CLASS);
    lua_pushnumber(L, tex->width);
    lua_pushnumber(L, tex->height);
    return 2;
}

static int l_poti_texture__gc(lua_State *L) {
    Texture* tex = luaL_checkudata(L, 1, TEXTURE_CLASS);
    glDeleteTextures(1, &tex->handle);
    if (tex->fbo != 0) glDeleteFramebuffers(1, &tex->fbo);
    // SDL_DestroyTexture(*tex);
    return 0;
}

static int l_register_texture_meta(lua_State *L) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_meta);
    luaL_Reg reg[] = {
        {"draw", l_poti_texture__draw},
        {"filter", NULL},
        {"wrap", NULL},
        {"width", l_poti_texture__width},
        {"height", l_poti_texture__height},
        {"size", l_poti_texture__size},
        {"__gc", l_poti_texture__gc},
        {NULL, NULL}
    };

    luaL_newmetatable(L, TEXTURE_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_setfield(L, -2, TEXTURE_CLASS);
    lua_pop(L, 1);
    return 0;
}

/*********************************
 * Font
 *********************************/

static int l_poti_new_font(lua_State *L) {
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

static int l_poti_font__print(lua_State *L) {
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
        // SDL_RenderCopy(poti()->render, font->tex, &src, &dest);
    }

    return 0;
}

static int l_poti_font__width(lua_State* L) {
    int index = 1;
    Font* font = luaL_checkudata(L, index++, FONT_CLASS);
    if (!font) {
        font = &poti()->default_font;
        --index;
    }

    const i8* text = luaL_checkstring(L, index++);

    u8* p = (u8*)text;
    float x0 = 0, y0 = 0;
    float width = 0;
    while (*p) {
        int codepoint;
        p = poti()->utf8_codepoint(p, &codepoint);
        SDL_Rect src, dest;
        SDL_Point pos;
        s_char_rect(font, codepoint, &x0, &y0, &pos, &src, 0);
        //dest.x = x + pos.x;
        //dest.y = y + pos.y;
        dest.w = src.w;
        dest.h = src.h;
        width += src.w;
        //SDL_RenderCopy(poti()->render, font->tex, &src, &dest);
    }
    lua_pushnumber(L, width);
    return 1;
}

static int l_poti_font__height(lua_State* L) {
    int index = 1;
    Font* font = luaL_checkudata(L, index++, FONT_CLASS);
    if (!font) {
        font = &poti()->default_font;
        --index;
    }

    const i8* text = luaL_checkstring(L, index++);

    u8* p = (u8*)text;
    float x0 = 0, y0 = 0;
    float height = 0;
    while (*p) {
        int codepoint;
        p = poti()->utf8_codepoint(p, &codepoint);
        SDL_Rect src, dest;
        SDL_Point pos;
        s_char_rect(font, codepoint, &x0, &y0, &pos, &src, 0);
        //dest.x = x + pos.x;
        //dest.y = y + pos.y;
        dest.w = src.w;
        dest.h = src.h;
        height = src.h > height ? src.h : height;
        //SDL_RenderCopy(poti()->render, font->tex, &src, &dest);
    }
    lua_pushnumber(L, height);
    return 1;
}

static int l_poti_font__gc(lua_State *L) {
    Font* font = luaL_checkudata(L, 1, FONT_CLASS);
    glDeleteTextures(1, &font->tex->handle);
    // SDL_DestroyTexture((*font)->tex);
    free(font->data);
    // tea_destroy_font(*font);
    return 0;
}

static int l_register_font_meta(lua_State* L) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_meta);
    luaL_Reg reg[] = {
        {"print", l_poti_font__print},
        {"width", l_poti_font__width},
        {"height", l_poti_font__height},
        {"__gc", l_poti_font__gc},
        {NULL, NULL}
    };

    luaL_newmetatable(L, FONT_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_setfield(L, -2, FONT_CLASS);
    lua_pop(L, 1);
    return 0;
}

/*********************************
 * Shader
 *********************************/

struct ShaderPair {
    const char *vert;
    const char *frag;
};

struct ShaderFactory {
    char version[256];
    struct ShaderPair uniforms;
    struct ShaderPair attributes;
    struct ShaderPair function;
    struct ShaderPair main;
};

static struct ShaderFactory s_factory;

static struct ShaderPair s_uniforms = {
    // vert
    "uniform mat4 u_World;\n"
    "uniform mat4 u_ModelView;\n",
    // frag
    "uniform sampler2D u_Texture;\n"
};

static struct ShaderPair s_100_attributes = {
    // vert
    "attribute vec2 a_Position;\n"
    "attribute vec4 a_Color;\n"
    "attribute vec2 a_TexCoord;\n"
    "varying vec4 v_Color;\n"
    "varying vec2 v_TexCoord;\n",
    // frag
    "varying vec4 v_Color;\n"
    "varying vec2 v_TexCoord;\n"
    "#define o_FragColor gl_FragColor\n"
    "#define texture texture2D\n"
};

static struct ShaderPair s_130_attributes = {
    // vert
    "in vec2 a_Position;\n"
    "in vec4 a_Color;\n"
    "in vec2 a_TexCoord;\n"
    "out vec4 v_Color;\n"
    "out vec2 v_TexCoord;\n",
    // frag
    "in vec4 v_Color;\n"
    "in vec2 v_TexCoord;\n"
    "out vec4 o_FragColor;\n"
};

static struct ShaderPair s_330_attributes = {
    // vert
    "layout (location = 0) in vec2 a_Position;\n"
    "layout (location = 1) in vec4 a_Color;\n"
    "layout (location = 2) in vec2 a_TexCoord;\n"
    "out vec4 v_Color;\n"
    "out vec2 v_TexCoord;\n",
    // frag
    "precision highp float;\n"
    "in vec4 v_Color;\n"
    "in vec2 v_TexCoord;\n"
    "layout (location = 0) out vec4 o_FragColor;\n"
};

static struct ShaderPair s_main = {
    // vert
    "\nvoid main() {\n"
    "   v_Color = a_Color;\n"
    "   v_TexCoord = a_TexCoord;\n"
    "   gl_Position = position(a_Position, u_World, u_ModelView);\n"
    "}",
    // frag
    "\nvoid main() {\n"
    "   o_FragColor = pixel(v_Color, v_TexCoord, u_Texture);\n"
    // "   gl_FragColor = pixel(v_Color, v_TexCoord, u_Texture);\n"
    "}"
};

int s_compile_factory(struct ShaderFactory *f, int *vert, int *frag) {
    int vert_len = strlen(f->version) + strlen(f->uniforms.vert) + strlen(f->attributes.vert) + strlen(f->function.vert) + strlen(f->main.vert);
    int frag_len = strlen(f->version) + strlen(f->uniforms.frag) + strlen(f->attributes.frag) + strlen(f->function.frag) + strlen(f->main.frag);
    
    char vert_src[vert_len+1];
    char frag_src[frag_len+1];
    // char* vert_src = malloc(vert_len + 1);
    // char* frag_src = malloc(frag_len + 1);

    sprintf(vert_src, "");
    sprintf(frag_src, "");

    strcat(vert_src, f->version);
    strcat(vert_src, f->uniforms.vert);
    strcat(vert_src, f->attributes.vert);
    strcat(vert_src, f->function.vert);
    strcat(vert_src, f->main.vert);

    strcat(frag_src, f->version);
    strcat(frag_src, f->uniforms.frag);
    strcat(frag_src, f->attributes.frag);
    strcat(frag_src, f->function.frag);
    strcat(frag_src, f->main.frag);

    vert_src[vert_len] = '\0';
    frag_src[frag_len] = '\0';

    // fprintf(stderr, "%s\n", vert_src);
    // fprintf(stderr, "%s\n", frag_src);

    int vert_shd = s_compile_shader(vert_src, GL_VERTEX_SHADER);
    int frag_shd = s_compile_shader(frag_src, GL_FRAGMENT_SHADER);

    // free(vert_src);
    // free(frag_src);

    if (vert) *vert = vert_shd;
    else glDeleteShader(vert_shd);

    if (frag) *frag = frag_shd;
    else glDeleteShader(frag_shd);

    return 1;
}

int s_compile_shader(const char *source, int type) {
    u32 shader = glCreateShader(type);
    const i8 *type_str = (i8*)(type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
    // char shader_define[128];
    // sprintf(shader_define, "#version 140\n#define %s_SHADER\n", type_str);

    // GLchar const *files[] = {shader_define, source};
    // GLint lengths[] = {strlen(shader_define), strlen(source)};

    // glShaderSource(shader, 2, files, lengths);

    GLchar const *files[] = {source};
    GLint lenghts[] = {strlen(source)};

    glShaderSource(shader, 1, files, lenghts);

    glCompileShader(shader);
    int success;
    char info_log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        fprintf(stderr, "failed to compile %s shader: %s\n", type_str, info_log);
    }

    return shader;
}

int s_load_program(int vertex, int fragment) {
    int success;
    char info_log[512];

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, info_log);
        fprintf(stderr, "failed to link program: %s\n", info_log);
    }

    return program;
}

static int l_poti_new_shader(lua_State *L) {
    Shader *shader = lua_newuserdata(L, sizeof(*shader));
    luaL_setmetatable(L, SHADER_CLASS);

    const char *vert_source = luaL_checkstring(L, 1);
    const char *frag_source = luaL_checkstring(L, 2);

    struct ShaderPair source;
    source.vert = vert_source;
    source.frag = frag_source;

    int vert, frag;

    memcpy(&s_factory.function, &source, sizeof(struct ShaderPair));
    s_compile_factory(&s_factory, &vert, &frag);

    shader->handle = s_load_program(vert, frag);
    shader->u_world = glGetUniformLocation(shader->handle, "u_World");
    shader->u_modelview = glGetUniformLocation(shader->handle, "u_ModelView");

    glDeleteShader(vert);
    glDeleteShader(frag);

    return 1;
}

static int l_poti_shader__set(lua_State *L) {
    Shader *s = luaL_checkudata(L, 1, SHADER_CLASS);
    RENDER()->set_shader(s);

    return 0;
}

/* static int l_poti_shader__unset(lua_State *L) {
    glUseProgram(0);
    return 0;
} */

static int l_poti_shader__location(lua_State *L) {
    Shader *shader = luaL_checkudata(L, 1, SHADER_CLASS);
    const char *name = luaL_checkstring(L, 2);
    lua_pushinteger(L, glGetUniformLocation(shader->handle, name));
    return 1;
}

static int s_send_shader_float(lua_State *L, Shader *s, i32 loc) {
    int top = lua_gettop(L);
    int count = top - 2;
    float v[count];
    for (int i = 0; i < count; i++) {
        v[i] = luaL_checknumber(L, i+3);
    }
    glUniform1fv(loc, count, v);

    return 1;
}

static int s_send_shader_vector(lua_State *L, Shader *s, i32 loc) {
    int top = lua_gettop(L);
    int count = top - 2;

    int len = lua_rawlen(L, 3);
    PFNGLUNIFORM2FVPROC glUniformfv = NULL;
    PFNGLUNIFORM2FVPROC fns[] = {
        glUniform1fv,
        glUniform2fv,
        glUniform3fv,
        glUniform4fv
    };

    // fprintf(stderr, "count: %d, len: %d\n", count, len);
    float v[count*len];

    if ((len - 1) >= 0 && (len - 1) <= 3) glUniformfv = fns[len-1];
    else {
        lua_pushstring(L, "invalid vector size");
        lua_error(L);
        return 0;
    }
    int i, j;
    float *pv = v;
    for (i = 0; i < count; i++) {
        for (j = 0; j < len; j++) {
            lua_pushnumber(L, j+1);
            lua_gettable(L, 3+i);
            *pv = lua_tonumber(L, -1);
            lua_pop(L, 1);
            pv++;
        }
    }

    // fprintf(stderr, "okok: %p %p\n", glUniformfv, glUniform4fv);
    glUniformfv(loc, count, v);


    return 1;
}

static i32 s_send_shader_boolean(lua_State *L, Shader *s, i32 loc) {
    i32 top = lua_gettop(L);
    i32 count = top - 2;
    i32 v[count];
	i32 i;
    for (i = 0; i < count; i++) {
        v[i] = lua_toboolean(L, i+3);
    }
    glUniform1iv(loc, count, v);

    return 1;
}

static int l_poti_shader__send(lua_State *L) {
    Shader *shader = luaL_checkudata(L, 1, SHADER_CLASS);
	i32 loc_tp = lua_type(L, 2);
	i32 loc;
	if (loc_tp == LUA_TSTRING) loc = glGetUniformLocation(shader->handle, lua_tostring(L, 2));
	else if (loc_tp == LUA_TNUMBER) loc = lua_tointeger(L, 2);
    
    i32 tp = lua_type(L, 3);
    if (tp == LUA_TNUMBER) s_send_shader_float(L, shader, loc);
    else if (tp == LUA_TTABLE) s_send_shader_vector(L, shader, loc);
    else if (tp == LUA_TBOOLEAN) s_send_shader_boolean(L, shader, loc);

    return 0;
}

static int l_poti_shader__gc(lua_State *L) {
    Shader *s = luaL_checkudata(L, 1, SHADER_CLASS);
    glDeleteProgram(s->handle);
    return 0;
}

static void s_setup_shader_factory() {
    // fprintf(stderr, "GL { version: %s, glsl: %s }\n", gl_version, glsl_version);
    // fprintf(stderr, "GL maj: %d, GL min: %d\n", GLVersion.major, GLVersion.minor);
    sprintf(s_factory.version, "#version %d", GL()->glsl);
    if (GL()->es && GL()->glsl > 100) strcat(s_factory.version, " es\n");
    else strcat(s_factory.version, "\n");
    fprintf(stderr, "%s", s_factory.version);
    if (GL()->glsl >= 300) {
        memcpy(&s_factory.attributes, &s_330_attributes, sizeof(struct ShaderPair));
    } else if (GL()->glsl >= 130) {
        memcpy(&s_factory.attributes, &s_130_attributes, sizeof(struct ShaderPair));
    } else if (GL()->glsl >= 100) {
        memcpy(&s_factory.attributes, &s_100_attributes, sizeof(struct ShaderPair));
    }
    memcpy(&s_factory.uniforms, &s_uniforms, sizeof(struct ShaderPair));
    memcpy(&s_factory.main, &s_main, sizeof(struct ShaderPair));
}

static int l_register_shader_meta(lua_State *L) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_meta);
    luaL_Reg reg[] = {
        {"set", l_poti_shader__set},
        {"location", l_poti_shader__location},
        {"send", l_poti_shader__send},
        // {"unset", l_poti_shader__unset},
        {"__gc", l_poti_shader__gc},
        {NULL, NULL}
    };

    luaL_newmetatable(L, SHADER_CLASS);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_setfield(L, -2, SHADER_CLASS);
    lua_pop(L, 1);
    return 0;
}

/*********************************
 * Draw
 *********************************/

static int l_poti_render_clear(lua_State *L) {
    float color[4] = {0.f, 0.f, 0.f, 1.f};
    int args = lua_gettop(L);

    for (int i = 0; i < args; i++) {
        color[i] = lua_tonumber(L, i+1) / 255.f;
    }
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}

static int l_poti_render_mode(lua_State *L) {
    const char *mode_str = luaL_checkstring(L, 1);
    int mode = 0;
    if (!strcmp(mode_str, "line")) mode = POTI_LINE;
    else if (!strcmp(mode_str, "fill")) mode = POTI_FILL;
    RENDER()->draw_vertex(VERTEX());
    RENDER()->clear_vertex(VERTEX());
    RENDER()->draw_mode = mode;
    return 0;
}

static int l_poti_render_color(lua_State *L) {
    int args = lua_gettop(L);

    for (int i = 0; i < args; i++) { 
        RENDER()->color[i] = luaL_checknumber(L, i+1) / 255.f;
    }

    return 0;
}

static int l_poti_render_target(lua_State *L) {
    Texture *target = luaL_testudata(L, 1, TEXTURE_CLASS);
    target = target ? target : &RENDER()->def_target;
    RENDER()->set_target(target);
    return 0;
}

// not working
static int l_poti_render_clip(lua_State *L) {
Texture *target = RENDER()->target;
i32 rect[] = {0, 0, target->width, target->height};
for (i32 i = 0; i < lua_gettop(L); i++)
    rect[i] = luaL_checknumber(L, i+1);
RENDER()->draw_vertex(VERTEX());
RENDER()->clear_vertex(VERTEX());
glScissor(rect[0], (target->height-rect[1])-rect[3], rect[2], rect[3]);
return 0;
}

static int l_poti_render_point(lua_State *L) {
    float x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);
    float *c = RENDER()->color;

    float vertices[] = {
        x, y, c[0], c[1], c[2], c[3], 0.f, 0.f
    };

    Texture *tex = RENDER()->white;
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(float), vertices);
    glDrawArrays(GL_POINTS, 0, 1);
    return 0;
}

static int l_poti_render_line(lua_State *L) {
    float p[4];

    for (int i = 0; i < 4; i++) {
        p[i] = luaL_checknumber(L, i+1);
    }
    float *c = RENDER()->color;
    float vertices[] = {
        p[0], p[1], c[0], c[1], c[2], c[3], 0.f, 0.f,
        p[2], p[3], c[0], c[1], c[2], c[3], 1.f, 1.f
    };

    Texture *tex = RENDER()->white;
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 16 * sizeof(float), vertices);
    glDrawArrays(GL_LINES, 0, 1);
    return 0;
}

typedef void(*DrawCircle)(float, float, float, int);
static void push_filled_circle(float xc, float yc, float radius, int segments) {
    float sides = segments >= 4 ? segments : 4;
    int bsize = (3*sides) * 8 * sizeof(float);
    float vertices[bsize];
    double pi2 = 2.0 * M_PI;

    float *v = vertices;
    for (int i = 0; i < sides; i++) {

        v[0] = xc;
        v[1] = yc;
        v[2] = RENDER()->color[0];
        v[3] = RENDER()->color[1];
        v[4] = RENDER()->color[2];
        v[5] = RENDER()->color[3];
        v[6] = 0.f;
        v[7] = 0.f;

        v += 8;

        float tetha = (i * pi2) / sides;

        v[0] = xc + (cosf(tetha) * radius);
        v[1] = yc + (sinf(tetha) * radius);
        v[2] = RENDER()->color[0];
        v[3] = RENDER()->color[1];
        v[4] = RENDER()->color[2];
        v[5] = RENDER()->color[3];
        v[6] = 0.f;
        v[7] = 0.f;
        v += 8;

        tetha = ((i+1) * pi2) / sides;

        v[0] = xc + (cosf(tetha) * radius);
        v[1] = yc + (sinf(tetha) * radius);
        v[2] = RENDER()->color[0];
        v[3] = RENDER()->color[1];
        v[4] = RENDER()->color[2];
        v[5] = RENDER()->color[3];
        v[6] = 0.f;
        v[7] = 0.f;
        v += 8;
    }

    // glBufferSubData(GL_ARRAY_BUFFER, 0, bsize, vertices);
    // glDrawArrays(GL_TRIANGLES, 0, 3*sides);
    RENDER()->push_vertices(VERTEX(), 3*sides, vertices);
}
static void push_lined_circle(float xc, float yc, float radius, int segments) {
    float sides = segments >= 4 ? segments : 4;
    int bsize = (2*sides) * 8 * sizeof(float);
    float vertices[bsize];
    double pi2 = 2.0 * M_PI;

    float *v = vertices;
    for (int i = 0; i < sides; i++) {
        float tetha = (i * pi2) / sides;

        v[0] = xc + (cosf(tetha) * radius);
        v[1] = yc + (sinf(tetha) * radius);
        v[2] = RENDER()->color[0];
        v[3] = RENDER()->color[1];
        v[4] = RENDER()->color[2];
        v[5] = RENDER()->color[3];
        v[6] = 0.f;
        v[7] = 0.f;

        v += 8;

        tetha = ((i+1) * pi2) / sides;

        v[0] = xc + (cosf(tetha) * radius);
        v[1] = yc + (sinf(tetha) * radius);
        v[2] = RENDER()->color[0];
        v[3] = RENDER()->color[1];
        v[4] = RENDER()->color[2];
        v[5] = RENDER()->color[3];
        v[6] = 0.f;
        v[7] = 0.f;
        v += 8;
    }
    RENDER()->push_vertices(VERTEX(), 2*sides, vertices);

    // glBufferSubData(GL_ARRAY_BUFFER, 0, bsize, vertices);
    // glDrawArrays(GL_LINES, 0, sides*2);
}

static int l_poti_render_circle(lua_State *L) {

    float x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);

    float radius = luaL_checknumber(L, 3);
    float segments = luaL_optnumber(L, 4, 16);
    RENDER()->set_texture(RENDER()->white);

    DrawCircle fn[2] = {
        push_lined_circle,
        push_filled_circle
    };

    fn[(int)RENDER()->draw_mode](x, y, radius, segments);

    return 0;
}

typedef void(*DrawRect)(SDL_Rect*);
static void push_filled_rect(SDL_Rect *r) {
    float *c = RENDER()->color;
    float vertices[] = {
        r->x, r->y, c[0], c[1], c[2], c[3], 0.f, 0.f,
        r->x + r->w, r->y, c[0], c[1], c[2], c[3], 1.f, 0.f,
        r->x + r->w, r->y + r->h, c[0], c[1], c[2], c[3], 1.f, 1.f,
        
        r->x, r->y, c[0], c[1], c[2], c[3], 0.f, 0.f,
        r->x, r->y + r->h, c[0], c[1], c[2], c[3], 0.f, 1.f,
        r->x + r->w, r->y + r->h, c[0], c[1], c[2], c[3], 1.f, 1.f,
    };

    // glBufferSubData(GL_ARRAY_BUFFER, 0, 48 * sizeof(float), vertices);
    // glDrawArrays(GL_TRIANGLES, 0, 6);
    RENDER()->push_vertices(VERTEX(), 6, vertices);
}
static void push_lined_rect(SDL_Rect *r) {
    float *c = RENDER()->color;
    float vertices[] = {
        r->x, r->y, c[0], c[1], c[2], c[3], 0.f, 0.f,
        r->x + r->w, r->y, c[0], c[1], c[2], c[3], 1.f, 0.f,

        r->x + r->w, r->y, c[0], c[1], c[2], c[3], 1.f, 0.f,
        r->x + r->w, r->y + r->h, c[0], c[1], c[2], c[3], 1.f, 1.f,

        r->x + r->w, r->y + r->h, c[0], c[1], c[2], c[3], 1.f, 1.f,
        r->x, r->y + r->h, c[0], c[1], c[2], c[3], 1.f, 1.f,
        
        r->x, r->y + r->h, c[0], c[1], c[2], c[3], 1.f, 1.f,
        r->x, r->y, c[0], c[1], c[2], c[3], 0.f, 0.f,
    };

    //glBufferSubData(GL_ARRAY_BUFFER, 0, 64 * sizeof(float), vertices);
    //glDrawArrays(GL_LINES, 0, 8);
    RENDER()->push_vertices(VERTEX(), 8, vertices);
}

static int l_poti_render_rectangle(lua_State *L) {

    SDL_Rect r = {0, 0, 0, 0};

    r.x = luaL_checknumber(L, 1);
    r.y = luaL_checknumber(L, 2);
    r.w = luaL_checknumber(L, 3);
    r.h = luaL_checknumber(L, 4);
    RENDER()->set_texture(RENDER()->white);

    DrawRect fn[2] = {
        push_lined_rect,
        push_filled_rect
    };

    fn[(int)RENDER()->draw_mode](&r);
    
    return 0;
}

typedef void(*DrawTriangle)(float, float, float, float, float, float);
static void push_filled_triangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    float *c = RENDER()->color;
    float vertices[] = {
        x0, y0, c[0], c[1], c[2], c[3], 0.5f, 0.f,
        x1, y1, c[0], c[1], c[2], c[3], 0.f, 1.f,
        x2, y2, c[0], c[1], c[2], c[3], 1.f, 1.f
    };

    RENDER()->push_vertices(VERTEX(), 3, vertices);
}

static void push_lined_triangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    float *c = RENDER()->color;
    float vertices[] = {
        x0, y0, c[0], c[1], c[2], c[3], 0.5f, 0.f,
        x1, y1, c[0], c[1], c[2], c[3], 0.f, 1.f,

        x1, y1, c[0], c[1], c[2], c[3], 0.f, 1.f,
        x2, y2, c[0], c[1], c[2], c[3], 1.f, 1.f,
        
        x2, y2, c[0], c[1], c[2], c[3], 1.f, 1.f,
        x0, y0, c[0], c[1], c[2], c[3], 0.5f, 0.f
    };
    RENDER()->push_vertices(VERTEX(), 6, vertices);
}

static int l_poti_render_triangle(lua_State *L) {

    float points[6];

    for (int i = 0; i < 6; i++) {
        points[i] = luaL_checknumber(L, i+1);
    }

    RENDER()->set_texture(RENDER()->white);
    const DrawTriangle fn[] = {
        push_lined_triangle,
        push_filled_triangle
    };

    Texture *tex = RENDER()->white;
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    fn[(int)RENDER()->draw_mode](points[0], points[1], points[2], points[3], points[4], points[5]);

    return 0;
}

static int l_poti_render_push_vertex(lua_State *L) {
    float *c = RENDER()->color;
    f32 vertex[8] = {
        0.f, 0.f,
        c[0], c[1], c[2], c[3],
        0.f, 0.f
    };
    vertex[0] = luaL_checknumber(L, 1);
    vertex[1] = luaL_checknumber(L, 2);
    for (i32 i = 0; i < 6; i++) {
        vertex[i+2] = luaL_optnumber(L, 3+i, vertex[i+2]);
    }
    RENDER()->push_vertex(VERTEX(), vertex[0], vertex[1], vertex[2], vertex[3], vertex[4], vertex[5], vertex[6], vertex[7]);
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

static int l_poti_render_print(lua_State *L) {
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

    s_font_print(font, x, y, text);

    return 0;
}

static struct {
    const char *name;
    int value;
} blend_modes[] = {
    {"none", SDL_BLENDMODE_NONE},
    {"alpha", SDL_BLENDMODE_BLEND},
    {"add", SDL_BLENDMODE_ADD},
    {"mod", SDL_BLENDMODE_MOD}
};

static int l_poti_render_blend_mode(lua_State *L) {
    int i;
    SDL_BlendMode mode;
    // SDL_GetRenderDrawBlendMode(poti()->render, &mode);
    const char *mode_str = luaL_checkstring(L, 1);
    for (i = 0; i < 4; i++) {
        if (!strcmp(mode_str, blend_modes[i].name)) {
            mode = blend_modes[i].value;
            break;
        }
    }

    return 0;
}

typedef struct {
    float x, y, w, h;
} Rectf;

void s_font_print(Font* font, float x, float y, const char* text) {
    u8* p = (u8*)text;
    float x0 = 0, y0 = 0;
    int count = strlen(text) * 6;
    float w, h;
    float *c = RENDER()->color;
    w = font->tex->width;
    h = font->tex->height;
    #if 1
    float vertices[count*8];
    float *v = vertices;
    RENDER()->set_texture(font->tex);
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

        Rectf t;
        t.x = src.x / w;
        t.y = src.y / h;
        t.w = src.w / w;
        t.h = src.h / h;
        float letter[] = {
            dest.x, dest.y, c[0], c[1], c[2], c[3], t.x, t.y,
            dest.x + dest.w, dest.y, c[0], c[1], c[2], c[3], t.x + t.w, t.y,
            dest.x + dest.w, dest.y + dest.h, c[0], c[1], c[2], c[3], t.x + t.w, t.y + t.h,

            dest.x, dest.y, c[0], c[1], c[2], c[3], t.x, t.y,
            dest.x, dest.y + dest.h, c[0], c[1], c[2], c[3], t.x, t.y + t.h,
            dest.x + dest.w, dest.y + dest.h, c[0], c[1], c[2], c[3], t.x + t.w, t.y + t.h,
        };
        memcpy(v, letter, 48*sizeof(float));
        v += 48;
        // SDL_RenderCopy(poti()->render, font->tex, &src, &dest);
    }
    RENDER()->push_vertices(VERTEX(), count, vertices);
    
    // glBindTexture(GL_TEXTURE_2D, font->tex->handle);
    // glBufferSubData(GL_ARRAY_BUFFER, 0, count * 8 * sizeof(float), vertices);
    // glDrawArrays(GL_TRIANGLES, 0, count);
    #else
    glBindTexture(GL_TEXTURE_2D, font->tex->handle);
    float vertices[] = {
        x, y, c[0], c[1], c[2], c[3], 0, 0,
        x+w, y, c[0], c[1], c[2], c[3], 1, 0,
        x+w, y+h, c[0], c[1], c[2], c[3], 1, 1,
        
        x, y, c[0], c[1], c[2], c[3], 0, 0,
        x, y+h, c[0], c[1], c[2], c[3], 0, 1,
        x+w, y+h, c[0], c[1], c[2], c[3], 1, 1,
    };

    glBufferSubData(GL_ARRAY_BUFFER, 0, 48*sizeof(float), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    #endif
}

i32 s_get_text_width(Font *font, const i8* text, i32 len) {
    if (!text) return 0;
    float x0 = 0, y0 = 0;
    i32 width = 0;
    u8* p = (u8*)text;
    while (*p && len--) {
        int codepoint;
        p = poti()->utf8_codepoint(p, &codepoint);
        SDL_Rect src;
        SDL_Point pos;
        s_char_rect(font, codepoint, &x0, &y0, &pos, &src, 0);
        width += src.w;
    }
    return width;
}

i32 s_get_text_height(Font *font) {
    return font->height;
}

static int l_poti_render_default_shader(lua_State *L) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, RENDER()->def_shader);
    return 1;
}

static int luaopen_render(lua_State* L) {
    luaL_Reg reg[] = {
        {"mode", l_poti_render_mode},
        {"clear", l_poti_render_clear},
        {"color", l_poti_render_color},
        {"target", l_poti_render_target},
        {"clip", l_poti_render_clip},
        {"point", l_poti_render_point},
        {"line", l_poti_render_line},
        {"circle", l_poti_render_circle},
        {"rectangle", l_poti_render_rectangle},
        {"triangle", l_poti_render_triangle},
        {"push_vertex", l_poti_render_push_vertex},
        {"print", l_poti_render_print},
        {"blend_mode", l_poti_render_blend_mode},
        {"default_shader", l_poti_render_default_shader},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    return 1;
}

/*=================================*
    *              Input              *
    *=================================*/

/**************************
 *  Keyboard 
 **************************/

static int l_poti_keyboard_down(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = SDL_GetScancodeFromName(key);
    lua_pushboolean(L, poti()->keys[code]);
    
    return 1;
}

static int l_poti_keyboard_up(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);

    int code = SDL_GetScancodeFromName(key);
    lua_pushboolean(L, !poti()->keys[code]);
    
    return 1;
}

static int l_poti_keyboard_has_screen_support(lua_State *L) {
    lua_pushboolean(L, SDL_HasScreenKeyboardSupport());
    return 1;
}

static int l_poti_keyboard_is_screen_shown(lua_State *L) {
    lua_pushboolean(L, SDL_IsScreenKeyboardShown(poti()->window));
    return 1;
}

static int luaopen_keyboard(lua_State *L) {
    luaL_Reg reg[] = {
        {"down", l_poti_keyboard_down},
        {"up", l_poti_keyboard_up},
        {"has_screen_support", l_poti_keyboard_has_screen_support},
        {"is_screen_shown", l_poti_keyboard_is_screen_shown},
        //{"pressed", poti_key_pressed},
        //{"released", poti_key_released},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}

/**************************
 *  Mouse 
 **************************/

static int l_poti_mouse_pos(lua_State *L) {
    int x, y;
    SDL_GetMouseState(&x, &y);

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

static int mouse_down(int btn) {
    return SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(btn);
}

static int l_poti_mouse_down(lua_State *L) {
    int button = luaL_checknumber(L, 1);

    lua_pushboolean(L, mouse_down(button));
    
    return 1;
}

static int l_poti_mouse_up(lua_State *L) {
    int button = luaL_checknumber(L, 1) - 1;
    lua_pushboolean(L, !mouse_down(button));
    
    return 1;
}

static int luaopen_mouse(lua_State *L) {
    luaL_Reg reg[] = {
        {"pos", l_poti_mouse_pos},
        {"down", l_poti_mouse_down},
        {"up", l_poti_mouse_up},
        {NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}

/**************************
 *  Joystick 
 **************************/

static int l_poti_joystick(lua_State* L) {
    Joystick* joy = SDL_JoystickOpen(luaL_checknumber(L, 1));
    if (joy) {
        Joystick** j = lua_newuserdata(L, sizeof(*j));
        luaL_setmetatable(L, JOYSTICK_CLASS);
        *j = joy;
    }
    else {
        lua_pushnil(L);
    }

    return 1;
}

static int l_poti_num_joysticks(lua_State *L) {
    lua_pushinteger(L, SDL_NumJoysticks());
    return 1;
}

static int l_poti_joystick_name(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushstring(L, SDL_JoystickName(*j));
    return 1;
}

static int l_poti_joystick_vendor(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickGetVendor(*j));
    return 1;
}

static int l_poti_joystick_product(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickGetProduct(*j));
    return 1;
}

static int l_poti_joystick_product_version(lua_State *L) {
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

static int l_poti_joystick_type(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushstring(L, joy_types[SDL_JoystickGetType(*j)]);
    return 1;
}


static int l_poti_joystick_num_axes(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumAxes(*j));
    return 1;
}

static int l_poti_joystick_num_balls(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumBalls(*j));
    return 1;
}

static int l_poti_joystick_num_hats(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumHats(*j));
    return 1;
}

static int l_poti_joystick_num_buttons(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushnumber(L, SDL_JoystickNumButtons(*j));
    return 1;
}
static int l_poti_joystick_axis(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int axis = luaL_checknumber(L, 2);
    lua_pushnumber(L, SDL_JoystickGetAxis(*j, axis) / SDL_JOYSTICK_AXIS_MAX);
    return 1;
}

static int l_poti_joystick_ball(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int ball = luaL_checknumber(L, 2);
    int dx, dy;
    SDL_JoystickGetBall(*j, ball, &dx, &dy);
    lua_pushnumber(L, dx);
    lua_pushnumber(L, dy);
    return 2;
}

static int l_poti_joystick_hat(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int hat = luaL_checknumber(L, 2);
    lua_pushnumber(L, SDL_JoystickGetHat(*j, hat));
    return 1;
}

static int l_poti_joystick_button(lua_State* L) {
    Joystick** j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    int button = luaL_checknumber(L, 2);
    int res = SDL_JoystickGetButton(*j, button);

    lua_pushboolean(L, res);
    return 1;
}

static int l_poti_joystick_rumble(lua_State *L) {
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

static int l_poti_joystick_powerlevel(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    lua_pushstring(L, joy_powerlevels[SDL_JoystickCurrentPowerLevel(*j) + 1]);
    return 1;
}

static int l_poti_joystick_close(lua_State *L) {
    Joystick **j = luaL_checkudata(L, 1, JOYSTICK_CLASS);
    if (!*j) return 0;
    Joystick* jj = *j;
    if (SDL_JoystickGetAttached(jj)) {
        SDL_JoystickClose(jj);
    }
    return 0;
}

static int l_poti_joystick__gc(lua_State* L) {
    l_poti_joystick_close(L);
    return 0;
}

static int luaopen_joystick(lua_State *L) {
    luaL_Reg reg[] = {
        {"open", l_poti_joystick},
        {"num", l_poti_num_joysticks},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    luaL_Reg meta[] = {
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
    luaL_setfuncs(L, meta, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    return 1;
}

/**************************
 *  Gamepad 
 **************************/

static int l_poti_gamepad(lua_State *L) {
    GameController* controller = SDL_GameControllerOpen(luaL_checknumber(L, 1));
    if (controller) {
        GameController** g = lua_newuserdata(L, sizeof(*g));
        *g = controller;
        luaL_setmetatable(L, GAMEPAD_CLASS);
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
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushstring(L, SDL_GameControllerName(*g));
    return 1;
}

static int l_poti_gamepad_vendor(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushnumber(L, SDL_GameControllerGetVendor(*g));
    return 1;
}

static int l_poti_gamepad_product(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushnumber(L, SDL_GameControllerGetProduct(*g));
    return 1;
}

static int l_poti_gamepad_product_version(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    lua_pushnumber(L, SDL_GameControllerGetProductVersion(*g));
    return 1;
}

const char *gpad_axes[] = {
    "leftx", "lefty", "rightx", "righty", "triggerleft", "triggerright"
};

static int l_poti_gamepad_axis(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
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
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    const char *str = luaL_checkstring(L, 2);
    int button = SDL_GameControllerGetButtonFromString(str);
    if (button < 0) {
        luaL_argerror(L, 2, "invalid button");
    }
    lua_pushboolean(L, SDL_GameControllerGetButton(*g, button));
    return 1;
}

static int l_poti_gamepad_rumble(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
    u16 low = (u16)luaL_checknumber(L, 2);
    u16 high = (u16)luaL_checknumber(L, 3);
    u32 freq = (u32)luaL_optnumber(L, 4, 100);
    lua_pushboolean(L, SDL_GameControllerRumble(*g, low, high, freq) == 0);
    return 1;
}

static int l_poti_gamepad_close(lua_State *L) {
    GameController **g = luaL_checkudata(L, 1, GAMEPAD_CLASS);
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
    luaL_newmetatable(L, GAMEPAD_CLASS);
    luaL_setfuncs(L, meta, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    return 1;
}

/*=================================*
    *               GUI               *
    *=================================*/

static int l_poti_gui_begin(lua_State* L) {
    mu_begin(&poti()->ui);
    return 0;
}

static int l_poti_gui_end(lua_State* L) {
    mu_end(&poti()->ui);
    return 0;
}

static int l_poti_gui_begin_window(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    SDL_Rect r;
    s_get_rect_from_table(L, 2, &r);
    lua_pushboolean(L, mu_begin_window(&poti()->ui, title, mu_rect(r.x, r.y, r.w, r.h)));
    return 1;
}

static int l_poti_gui_end_window(lua_State* L) {
    // mu_end_window(&poti()->ui);
    return 0;
}

static int l_poti_gui_label(lua_State* L) {
    //const char* text = luaL_checkstring(L, 1);
    // mu_label(&poti()->ui, text);
    return 0;
}

static int l_poti_gui_button(lua_State* L) {
    //const char* label = luaL_checkstring(L, 1);
    // lua_pushboolean(L, mu_button(&poti()->ui, label));
    return 1;
}

int row_pool[8];

static int l_poti_gui_layout_row(lua_State* L) {
    int index = 1;
    int items = luaL_checkinteger(L, index++);
    int *width = row_pool;
    if (lua_istable(L, index)) {
        int len = lua_rawlen(L, index);
        for (int i = 0; i < len; i++) {
            lua_rawgeti(L, index, i + 1);
            width[i] = lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
        index++;
    }
    else {
        memset(row_pool, 0, sizeof(int) * 8);
    }
    int height = luaL_optinteger(L, index, 0);
    mu_layout_row(&poti()->ui, items, width, height);
    return 0;
}

int luaopen_gui(lua_State* L) {
    luaL_Reg reg[] = {
        {"begin_window", l_poti_gui_begin_window},
        {"end_window", l_poti_gui_end_window},
        {"button", l_poti_gui_button},
        {"label", l_poti_gui_label},
        {"layout_row", l_poti_gui_layout_row},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);
    return 1;
}

/*=================================*
    *             Window              *
    *=================================*/

static int l_poti_window_title(lua_State *L) {
    const char *title = NULL;
    if (!lua_isnil(L, 1)) {
        title = luaL_checkstring(L, 1);
        SDL_SetWindowTitle(poti()->window, title);
    }

    lua_pushstring(L, SDL_GetWindowTitle(poti()->window));

    return 1;
}

static int l_poti_window_width(lua_State *L) {
    int width = 0;
    if (!lua_isnil(L, 1)) {
        int height;
        width = luaL_checkinteger(L, 1);
        SDL_GetWindowSize(poti()->window, NULL, &height);
        SDL_SetWindowSize(poti()->window, width, height);
    }
    else SDL_GetWindowSize(poti()->window, &width, NULL);

    lua_pushinteger(L, width);
    return 1;
}

static int l_poti_window_height(lua_State *L) {
    int height = 0;
    if (!lua_isnil(L, 1)) {
        int width;
        height = luaL_checkinteger(L, 1);
        SDL_GetWindowSize(poti()->window, &width, NULL);
        SDL_SetWindowSize(poti()->window, width, height);
    }
    else SDL_GetWindowSize(poti()->window, NULL, &height);

    lua_pushinteger(L, height);
    return 1;
}

static int l_poti_window_size(lua_State *L) {
    int size[2];

    if (lua_gettop(L) <= 0) SDL_GetWindowSize(poti()->window, &size[0], &size[1]);

    for (int i = 0; i < lua_gettop(L); i++) {
        size[i] = luaL_checkinteger(L, i+1);
    }
    

    lua_pushinteger(L, size[0]);
    lua_pushinteger(L, size[1]);
    return 2;
}

static int l_poti_window_position(lua_State *L) {
    int pos[2];

    SDL_GetWindowPosition(poti()->window, &pos[0], &pos[1]);
    int top = lua_gettop(L);
    
    if (top > 0) {
        for (int i = 0; i < top; i++) {
            pos[i] = luaL_checkinteger(L, i+1);
        }
        SDL_SetWindowPosition(poti()->window, pos[0], pos[1]);
    }

    lua_pushinteger(L, pos[0]);
    lua_pushinteger(L, pos[1]);
    return 2;
}

static int l_poti_window_resizable(lua_State *L) {
    int resizable = 0;
    Uint32 flags = SDL_GetWindowFlags(poti()->window);

    resizable = flags & SDL_WINDOW_RESIZABLE;

    if (!lua_isnil(L, 1)) {
        resizable = lua_toboolean(L, 1);
        SDL_SetWindowResizable(poti()->window, resizable);
    }

    lua_pushboolean(L, resizable);

    return 1;
}

static int l_poti_window_bordered(lua_State *L) {
    int bordered = 0;
    Uint32 flags = SDL_GetWindowFlags(poti()->window);

    bordered = ~flags & SDL_WINDOW_BORDERLESS;

    if (!lua_isnil(L, 1)) {
        bordered = lua_toboolean(L, 1);
        SDL_SetWindowBordered(poti()->window, bordered);
    }

    lua_pushboolean(L, bordered);
    return 1;
}

static int l_poti_window_border_size(lua_State *L) {

    int borders[4];

    SDL_GetWindowBordersSize(poti()->window, &borders[0], &borders[1], &borders[2], &borders[3]);

    for (int i = 0; i < 4; i++) lua_pushinteger(L, borders[i]);

    return 4;
}

static int l_poti_window_maximize(lua_State *L) {
    SDL_MaximizeWindow(poti()->window);
    return 0;
}

static int l_poti_window_minimize(lua_State *L) {
    SDL_MinimizeWindow(poti()->window);
    return 0;
}

static int l_poti_window_restore(lua_State *L) {
    SDL_RestoreWindow(poti()->window);
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
    SDL_ShowSimpleMessageBox(flags, title, message, poti()->window);

    return 0;
}


static int luaopen_window(lua_State *L) {
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

/*=================================*
 *              Poti               *
 *=================================*/

int luaopen_poti(lua_State* L) {
    luaL_Reg reg[] = {
        {"ver", l_poti_ver},
        {"meta", l_poti_meta},
        {"start_textinput", l_poti_start_textinput},
        {"stop_textinput", l_poti_stop_textinput},
        {"textinput_rect", l_poti_textinput_rect},
        /* Audio */
        {"volume", poti_volume},
        /* Types */
        {"Texture", l_poti_new_texture},
        {"Font", l_poti_new_font},
        {"Shader", l_poti_new_shader},
        {"Audio", l_poti_new_audio},
        {"File", l_poti_file},
        /* GUI */
        {"gui_begin", l_poti_gui_begin},
        {"gui_end", l_poti_gui_end},
        {NULL, NULL}
    };

    luaL_newlib(L, reg);

    struct { char* name; int(*fn)(lua_State*); } libs[] = {
        /* Modules */
        {"filesystem", luaopen_filesystem},
        {"gamepad", luaopen_gamepad},
        {"joystick", luaopen_joystick},
        {"mouse", luaopen_mouse},
        {"keyboard", luaopen_keyboard},
        {"event", luaopen_event},
        {"render", luaopen_render},
        {"timer", luaopen_timer},
        {"gui", luaopen_gui},
        {"window", luaopen_window},
        {NULL, NULL}
    };

    int i;
    for (i = 0; libs[i].name; i++) {
        if (libs[i].fn) {
            libs[i].fn(L);
            lua_setfield(L, -2, libs[i].name);
        }
    }

    l_register_texture_meta(L);
    l_register_font_meta(L);
    l_register_shader_meta(L);
    l_register_audio_meta(L);

    return 1;
}

static int poti_init(int argc, char **argv) {
    if (argc > 1) strcpy(poti()->basepath, argv[1]);
    else strcpy(poti()->basepath, ".");

    if (sbtar_open(&poti()->pkg, "game.pkg"))
        poti()->is_packed = 1;
    s_setup_function_ptrs(poti());

    POTI()->L = luaL_newstate();
    lua_State *L = poti()->L;

    /* init table for store types metatables */
    lua_newtable(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &lr_meta);

    /* init table to store audio data */
    lua_newtable(L);
    lua_newtable(L);
    lua_rawseti(L, -2, AUDIO_STREAM);
    lua_newtable(L);
    lua_rawseti(L, -2, AUDIO_STATIC);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &lr_audio_data);

    luaL_openlibs(L);
    luaL_requiref(L, "poti", luaopen_poti, 1);

    char title[100];
    sprintf(title, "poti %s", POTI_VER);

    mu_init(&poti()->ui);
    poti()->ui.text_height = (i32(*)(mu_Font))s_get_text_height;
    poti()->ui.text_width = (i32(*)(mu_Font, const i8*, i32))s_get_text_width;
    poti()->ui.style->font = &poti()->default_font;

#ifdef __EMSCRIPTEN__
    u32 flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER |
    SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_SENSOR | SDL_INIT_AUDIO;
#else
    u32 flags = SDL_INIT_EVERYTHING;
#endif
    if (SDL_Init(flags)) {
        fprintf(stderr, "Failed to init SDL2: %s\n", SDL_GetError());
        exit(0);
    }

    SDL_Window *window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 380, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!window) {
        fprintf(stderr, "Failed to create SDL2 window: %s\n", SDL_GetError());
        exit(0);
    }
    VERTEX() = &RENDER()->def_vertex;
    //SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    GL()->context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, GL()->context);
    poti()->keys = SDL_GetKeyboardState(NULL);
    poti()->window = window;

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "failed to initialize OpenGL context\n");
        exit(0);
    }

    const u8 *version = glGetString(GL_VERSION);
    const u8 *glsl = glGetString(GL_SHADING_LANGUAGE_VERSION);

    fprintf(stderr, "%s // %s\n", version, glsl);

    const i8 *prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL,
    };
    GL()->es = 0;
    i8 *ver = (i8*)version;
    for (u32 i = 0; prefixes[i] != NULL; i++) {
        if (strncmp(ver, prefixes[i], strlen(prefixes[i])) == 0) {
            ver += strlen(prefixes[i]);
            GL()->es = 1;
            break;
        }
    }
    GL()->major = ver[0] - '0';
    GL()->minor = ver[2] - '0';
    if (GL()->es) {
        GL()->glsl = 100;
        if (GL()->major == 3) {
            GL()->glsl = GL()->major * 100 +  GL()->minor * 10;
        }
    } else {
        float fglsl = atof((const i8*)glsl);
        GL()->glsl = 100 * fglsl;
    }

    fprintf(stderr, "GL: { ver: %d.%d, glsl: %d, es: %s }\n", GL()->major, GL()->minor, GL()->glsl, GL()->es ? "true" : "false");
    
    s_setup_shader_factory();

    RENDER()->init_vertex(VERTEX(), 10000);
#if 0
    glGenBuffers(1, &RENDER()->buffer.handle);
    glGenVertexArrays(1, &RENDER()->vao);

    glBindVertexArray(RENDER()->vao);
    glBindBuffer(GL_ARRAY_BUFFER, RENDER()->buffer.handle);
    glBufferData(GL_ARRAY_BUFFER, 4096, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(sizeof(float) * 2));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(sizeof(float) * 6));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
    lua_pushcfunction(L, l_poti_new_shader);
    lua_pushstring(L, vert_shader);
    lua_pushstring(L, frag_shader);

    lua_pcall(L, 2, 1, 0);
    RENDER()->def_shader = lua_touserdata(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, RENDER()->def_shader);

    RENDER()->white = lua_newuserdata(L, sizeof(Texture));
    luaL_setmetatable(L, TEXTURE_CLASS);

    char pixels[] = {255, 255, 255, 255};
    POTI()->init_texture(RENDER()->white, 0, 1, 1, GL_RGBA, (void*)pixels);
    lua_rawsetp(L, LUA_REGISTRYINDEX, RENDER()->white);

    lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_meta);
    lua_getfield(L, -1, TEXTURE_CLASS);
    lua_rawgetp(L, LUA_REGISTRYINDEX, RENDER()->white);
    lua_setfield(L, -2, "white");
    lua_pop(L, 2);

    RENDER()->color[0] = RENDER()->color[1] =
    RENDER()->color[2] = RENDER()->color[3] = 1.f;


    poti()->init_font(&poti()->default_font, _embed_font_ttf, _embed_font_ttf_size, 8);

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
    lua_rawsetp(L, LUA_REGISTRYINDEX, &lr_step);

    // fprintf(stderr, "Is Packed: %d\n", poti()->is_packed);
    return 1;
}

static int poti_step() {
    lua_State *L = poti()->L;
    SDL_Event *event = &poti()->event;
    lua_rawgetp(poti()->L, LUA_REGISTRYINDEX, &lr_callbacks);
    while (SDL_PollEvent(event)) {
        lua_rawgeti(L, -1, event->type);
        if (!lua_isnil(L, -1)) {
            lua_pushlightuserdata(L, event);
            lua_pcall(L, 1, 0, 0);
        } else lua_pop(L, 1);
    }
    lua_pop(L, 1);
    double current_time = SDL_GetTicks();
    poti()->timer.delta = (current_time - poti()->timer.last) / 1000.f;
    poti()->timer.last = current_time;

    int ww, wh;
    SDL_GetWindowSize(poti()->window, &ww, &wh);
    glViewport(0, 0, ww, wh);
    Texture *target = &RENDER()->def_target;
    target->fbo = 0;
    target->width = ww;
    target->height = wh;

    glClearColor(0.3f, 0.4f, 0.4f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if 0
    Shader *shader = RENDER()->def_shader;
    glUseProgram(shader->handle);
    mat4x4 world, modelview;
    mat4x4_identity(world);
    mat4x4_identity(modelview);
    GLint view[4];
    glGetIntegerv(GL_VIEWPORT, view);
    mat4x4_ortho(world, 0, view[2], view[3], 0, 0, 1);

    glUniformMatrix4fv(shader->u_world, 1, GL_FALSE, *world);
    glUniformMatrix4fv(shader->u_modelview, 1, GL_FALSE, *modelview);
#endif
    // setup states
    RENDER()->draw_calls = 0;
    RENDER()->clear_vertex(VERTEX());
    RENDER()->set_shader(RENDER()->def_shader);
    RENDER()->set_texture(RENDER()->white);
    RENDER()->set_target(&RENDER()->def_target);
    RENDER()->bind_vertex(VERTEX());
    
    lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_step);
    lua_pcall(L, 0, 0, 0);
    glDisable(GL_SCISSOR_TEST);
    RENDER()->draw_vertex(VERTEX());
    // printf("draw calls: %d\n", RENDER()->draw_calls);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    mu_Command* cmd = NULL;
    while (mu_next_command(&poti()->ui, &cmd)) {
        switch (cmd->type) {
        case MU_COMMAND_TEXT: s_font_print(&poti()->default_font, cmd->text.pos.x, cmd->text.pos.y, cmd->text.str); break;
        case MU_COMMAND_RECT: {
            lua_pushcfunction(L, l_poti_render_color);
            lua_pushnumber(L, cmd->rect.color.r);
            lua_pushnumber(L, cmd->rect.color.g);
            lua_pushnumber(L, cmd->rect.color.b);
            lua_pushnumber(L, cmd->rect.color.a);
            lua_pcall(L, 4, 0, 0);
            lua_pushcfunction(L, l_poti_render_rectangle);
            lua_pushnumber(L, cmd->rect.rect.x);
            lua_pushnumber(L, cmd->rect.rect.y);
            lua_pushnumber(L, cmd->rect.rect.w);
            lua_pushnumber(L, cmd->rect.rect.h);
            lua_pcall(L, 4, 0, 0);
            // SDL_SetRenderDrawColor(poti()->render, cmd->rect.color.r, cmd->rect.color.g, cmd->rect.color.b, cmd->rect.color.a);
            // SDL_Rect r = { cmd->rect.rect.x, cmd->rect.rect.y, cmd->rect.rect.w, cmd->rect.rect.h };
            // SDL_RenderFillRect(poti()->render, &r);
        }
        break;
        case MU_COMMAND_CLIP: {
            SDL_Rect r = { cmd->clip.rect.x, cmd->clip.rect.y, cmd->clip.rect.w, cmd->clip.rect.h };
            // SDL_RenderSetClipRect(poti()->render, &r);
        }
        break;
        }
    }
    // SDL_RenderSetClipRect(poti()->render, NULL);
    // SDL_SetRenderDrawColor(poti()->render, 255, 255, 255, 255);

    // SDL_RenderPresent(poti()->render);
    SDL_GL_SwapWindow(poti()->window);
    return 1;
}

static int poti_loop(void) {
#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(poti_step, 0, 1);
#else
    while (!poti()->should_close) poti_step();
#endif
    return 1;
}

static int poti_quit(void) {
    lua_close(POTI()->L);
    SDL_DestroyWindow(POTI()->window);
    SDL_GL_DeleteContext(GL()->context);
    SDL_Quit();
    // mo_deinit();
    if (POTI()->audio.is_ready) {
        ma_mutex_uninit(&POTI()->audio.lock);
        ma_device_uninit(&POTI()->audio.device);
        ma_context_uninit(&POTI()->audio.ctx);
    } else
        fprintf(stderr, "Audio Module could not be closed, not initialized\n");
    return 1;
}


int main(int argc, char ** argv) {
    poti_init(argc, argv);
    poti_loop();
    poti_quit();
    return 1;
}

static void s_init_vertex(Vertex *v, u32 size) {
    glGenVertexArrays(1, &v->vao);
    glGenBuffers(1, &v->vbo);
    glBindVertexArray(v->vao);
    glBindBuffer(GL_ARRAY_BUFFER, v->vbo);

    v->index = 0;
    v->size = size;
    v->data = malloc(size);

    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(sizeof(f32) * 2));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(sizeof(f32) * 6));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void s_bind_vertex(Vertex *v) {
    glBindVertexArray(v->vao);
    glBindBuffer(GL_ARRAY_BUFFER, v->vbo);
}

static void s_clear_vertex(Vertex *v) {
    v->index = 0;
}

static void s_set_texture(Texture *t) {
    if (RENDER()->tex != t) {
        RENDER()->draw_vertex(VERTEX());
        RENDER()->clear_vertex(VERTEX());
        RENDER()->tex = t;
        glBindTexture(GL_TEXTURE_2D, t->handle);
    }
}

static void s_set_shader(Shader *s) {
    if (RENDER()->shader != s) {
        RENDER()->draw_vertex(VERTEX());
        RENDER()->clear_vertex(VERTEX());
        RENDER()->shader = s;
        glUseProgram(s->handle);
        mat4x4 world, modelview;
        mat4x4_identity(world);
        mat4x4_identity(modelview);
        GLint view[4];
        glGetIntegerv(GL_VIEWPORT, view);
        mat4x4_ortho(world, 0, view[2], view[3], 0, 0, 1);

        glUniformMatrix4fv(s->u_world, 1, GL_FALSE, *world);
        glUniformMatrix4fv(s->u_modelview, 1, GL_FALSE, *modelview);
        RENDER()->shader = s;
    }
}

static void s_set_target(Texture *t) {
    if (t != &RENDER()->def_target && t->fbo == 0) return;
    if (RENDER()->target != t) {
        RENDER()->draw_vertex(VERTEX());
        RENDER()->clear_vertex(VERTEX());
        RENDER()->target = t;
        glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
        glViewport(0, 0, t->width, t->height);

        Shader *s = RENDER()->shader;

        mat4x4 world, modelview;
        mat4x4_identity(world);
        mat4x4_identity(modelview);
        GLint view[4];
        glGetIntegerv(GL_VIEWPORT, view);
        mat4x4_ortho(world, 0, view[2], view[3], 0, 0, 1);

        glUniformMatrix4fv(s->u_world, 1, GL_FALSE, *world);
        glUniformMatrix4fv(s->u_modelview, 1, GL_FALSE, *modelview);
    }
}

static void s_push_vertex(Vertex *vert, f32 x, f32 y, f32 r, f32 g, f32 b, f32 a, f32 u, f32 v) {
    f32 buf[] = {
        x, y, r, g, b, a, u, v
    };
    u32 size = sizeof(f32) * 8;
    if (vert->index + size > vert->size) {
        vert->size *= 2;
        vert->data = realloc(vert->data, vert->size);
    }
    char *data = ((char*)vert->data)+vert->index;
    vert->index += size;
    memcpy(data, buf, size);
}

static void s_push_vertices(Vertex *vert, u32 count, f32 *vertices) {
    u32 size = sizeof(f32) * 8 * count;
    if (vert->index + size > vert->size) {
        vert->size *= 2;
        vert->data = realloc(vert->data, vert->size);
    }
    char *data = ((char*)vert->data)+vert->index;
    vert->index += size;
    memcpy(data, vertices, size);
}

static void s_flush_vertex(Vertex *v) {
    glBufferSubData(GL_ARRAY_BUFFER, 0, v->index, v->data);
}

static void s_draw_vertex(Vertex *v) {
    if (v->index <= 0) return;
    s_flush_vertex(v); 
    u32 gl_modes[] = {
        GL_LINES, GL_TRIANGLES
    };
    RENDER()->draw_calls++;
    glDrawArrays(gl_modes[(i32)RENDER()->draw_mode], 0, v->index / (sizeof(f32)*8));
    }

    char read_buffer[1024];
    /* Static functions implementations */
    static i8* s_read_file(const char* filename, size_t* size) {
    FILE* fp;
    sprintf(read_buffer, "%s/%s", poti()->basepath, filename);
    #if defined(_WIN32)
    fopen_s(&fp, read_buffer, "rb+");
    #else
    fp = fopen(read_buffer, "rb");
    #endif
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp);
    if (size) *size = sz;
    fseek(fp, 0, SEEK_SET);
    i8* str = malloc(sz);
    if (!str) return NULL;
    fread(str, 1, sz, fp);
    fclose(fp);

    return str;
}

static i8* s_read_file_packed(const i8* filename, size_t* size) {
    sbtar_t* tar = &poti()->pkg;
    if (!sbtar_find(tar, filename)) return NULL;
    sbtar_header_t h;
    sbtar_header(tar, &h);
    i8* str = malloc(h.size);
    if (size) *size = h.size;
    sbtar_data(tar, str, h.size);

    return str;
}

#define MAX_UNICODE 0x10FFFF
static u8* s_utf8_codepoint(u8* p, i32* codepoint) {
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

static i32 s_init_font(Font* font, const void* data, size_t buf_size, i32 font_size) {
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
        th = MAX(th, h);
    }

    font->height = th;

    const int final_size = tw * th;

    u32* bitmap = malloc(final_size * sizeof(u32));
    memset(bitmap, 0, final_size * sizeof(u32));
    int x = 0;
    for (int i = 0; i < 256; i++) {
        int ww = font->c[i].bw;
        int hh = font->c[i].bh;
        int ssize = ww * hh;
        int ox, oy;

        unsigned char* bmp = stbtt_GetCodepointBitmap(&font->info, 0, font->scale, i, NULL, NULL, &ox, &oy);
        // u32 pixels[ssize];
        int xx, yy;
        xx = yy = 0;
        for (int j = 0; j < ssize; j++) {
            // int ii = j / 4;
            xx = (j % ww) + x;
            if (j != 0 && j % ww == 0) {
                yy += tw;
            }
            u8 *b = (u8*)&bitmap[xx + yy];
            b[0] = 255;
            b[1] = 255;
            b[2] = 255;
            b[3] = bmp[j];
        }
        stbtt_FreeBitmap(bmp, font->info.userdata);

        font->c[i].tx = x;

        x += font->c[i].bw;
    }
    // SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(bitmap, tw, th, 1, tw * 4, SDL_PIXELFORMAT_RGBA32);
    // SDL_Texture* tex = SDL_CreateTextureFromSurface(poti()->render, surf);
    Texture *tex = malloc(sizeof(Texture));
    s_init_texture(tex, 0, tw, th, GL_RGBA, bitmap);
    // SDL_FreeSurface(surf);
    free(bitmap);
    font->tex = tex;
    return 1;
}
#if 0
static Font* s_load_font(const i8* filename, i32 font_size) {
Font* font = malloc(sizeof(*font));
if (!font) {
    fprintf(stderr, "Failed to alloc memory for font\n");
    exit(EXIT_FAILURE);
}
size_t buf_size;
i8* buf = poti()->read_file(filename, &buf_size);
poti()->init_font(font, buf, buf_size, font_size);
free(buf);
return font;
}
#endif

u32 s_read_and_mix_pcm_frames(AudioBuffer* buffer, f32* output, u32 frames) {
    f32 temp[4096];
    u32 temp_cap_in_frames = ma_countof(temp) / AUDIO_DEVICE_CHANNELS;
    u32 total_frames_read = 0;
    ma_decoder* decoder = &buffer->decoder;
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
        }
        else {
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

void s_audio_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
    f32* out = (f32*)output;

    ma_mutex_lock(&poti()->audio.lock);
    i32 i;
    for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer* buffer = &poti()->audio.buffers[i];
        if (buffer->playing && buffer->loaded && !buffer->paused) {
            u32 frames_read = s_read_and_mix_pcm_frames(buffer, out, frame_count);
            if (frames_read < frame_count) {
                if (buffer->loop) {
                    ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
                    buffer->offset = 0;
                }
                else {
                    buffer->playing = 0;
                }
            }
        }
    }
    ma_mutex_unlock(&poti()->audio.lock);
    (void)input;
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

int s_setup_function_ptrs(struct Context* ctx) {
    ctx->read_file = s_read_file;
    if (poti()->is_packed)
        ctx->read_file = s_read_file_packed;
    ctx->init_font = s_init_font;
    ctx->init_texture = s_init_texture;
    // ctx->load_font = s_load_font;
    ctx->utf8_codepoint = s_utf8_codepoint;
    ctx->render.init_vertex = s_init_vertex;
    ctx->render.bind_vertex = s_bind_vertex;
    ctx->render.clear_vertex = s_clear_vertex;
    ctx->render.push_vertex = s_push_vertex;
    ctx->render.push_vertices = s_push_vertices;
    ctx->render.draw_vertex = s_draw_vertex;

    ctx->render.set_shader = s_set_shader;
    ctx->render.set_texture = s_set_texture;
    ctx->render.set_target = s_set_target;

    return 1;
}
