#ifndef POTI_H
#define POTI_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#if !defined(POTI_NO_WINDOW)

#if !defined(POTI_NO_GRAPHICS)
#include "glad/glad.h"
#endif

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
#endif

#define POTI_VER "0.1.0"

#define M_PI 3.14159
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(v, a, b) (MAX(a, MIN(v, b)))
#define DEG2RAD(deg) ((deg)*(M_PI/180.0))
#define RAD2DEG(rad) ((rad)*(180.0/M_PI))

#define TEXTURE_META "Texture"
#define FONT_META "Font"
#define SHADER_META "Shader"

#define AUDIO_META "Audio"

#define JOYSTICK_META "Joystick"
#define GAMEPAD_META "Gamepad"

#define FILE_META "File"

#define POTI_LINE 0
#define POTI_FILL 1

#if defined(__cplusplus)
extern "C" {
#endif

typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long int i64;
typedef unsigned long int u64;
typedef float f32;
typedef double f64;

#if !defined(POTI_NO_WINDOW)
typedef struct SDL_Window Window;
#endif
typedef union SDL_Event Event;

#if !defined(POTI_NO_GRAPHICS) && !defined(POTI_NO_WINDOW) 
typedef void* GLContext;
typedef struct Texture Texture;
typedef struct Font Font;
typedef struct Shader Shader;
#endif

#if !defined(POTI_NO_AUDIO)
typedef struct AudioData AudioData;
typedef struct AudioBuffer AudioBuffer;
typedef struct Sound Sound;
typedef struct Music Music;
#endif

#if !defined(POTI_NO_INPUT) && !defined(POTI_NO_JOYSTICK)
typedef struct _SDL_Joystick Joystick;
typedef struct _SDL_GameController GameController;
#endif

extern int l_context_reg;

extern int l_error_reg;
extern int l_config_reg;
extern int l_step_reg;

int luaopen_poti(lua_State* L);

#if !defined(POTI_NO_AUDIO)
extern const i8 lr_audio_data;
int luaopen_audio(lua_State* L);
int poti_init_audio(lua_State* L);
int poti_deinit_audio(lua_State* L);
#endif
#if !defined(POTI_NO_EVENT)
int luaopen_event(lua_State* L);
#endif
#if !defined(POTI_NO_FILESYSTEM)
int luaopen_filesystem(lua_State* L);
int poti_init_filesystem(lua_State* L);
const i8* poti_fs_read_file(const i8* filename, i32* out_size);
int poti_fs_write_file(const i8* filename, i32 size);
#endif
#if !defined(POTI_NO_GRAPHICS) && !defined(POTI_NO_WINDOW)
int luaopen_graphics(lua_State* L);
int poti_init_graphics(lua_State* L);
int poti_deinit_graphics(lua_State* L);
#endif

#if !defined(POTI_NO_INPUT)
    #if !defined(POTI_NO_JOYSTICK)
    int luaopen_gamepad(lua_State* L);
    int luaopen_joystick(lua_State* L);
    #endif
    #if !defined(POTI_NO_KEYBOARD)
    int luaopen_keyboard(lua_State* L);
    int poti_init_keyboard(lua_State* L);
    #endif
    #if !defined(POTI_NO_MOUSE)
    int luaopen_mouse(lua_State* L);
    #endif
#endif

#if !defined(POTI_NO_TIMER)
int luaopen_timer(lua_State *L);
#endif
#if !defined(POTI_NO_WINDOW)
int luaopen_window(lua_State* L);
int poti_init_window(lua_State* L);
int poti_deinit_window(lua_State* L);
#endif

#if defined(__cplusplus)
}
#endif
#endif /* POTI_H */
