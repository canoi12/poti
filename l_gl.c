#ifndef LUA_GL_H
#define LUA_GL_H

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

int luaopen_gl(lua_State* L);

#endif /* LUA_GL_H */
#if defined(LUA_GL_IMPLEMENTATION)

#define GET_GL(name)\
(gl()->procs[LUA_GL_##name])

#define CALL_GL(name)\
((GL##name##Proc)GET_GL(name))

#if defined(__EMSCRIPTEN__)
	#include <emscripten.h>
#endif

#if defined(_WIN32)
	#include <windows.h>
	#ifndef WINDOWS_LEAN_AND_MEAN
		#define WINDOWS_LEAN_AND_MEAN 1
	#endif
	static HMODULE s_glsym;
#else
	#include <dlfcn.h>
	static void* s_glsym;
	#ifndef RTLD_LAZY
		#define RTLD_LAZY 0x00001
	#endif
	#ifndef RTLD_GLOBAL
		#define RTLD_GLOBAL 0x00100
	#endif
#endif

#define LUA_GL_VERSION 0x1F02
#define LUA_GL_SHADING_LANGUAGE_VERSION 0x8B8C

enum {
    LUA_PROC_OVERRIDE = (1 << 0),
    LUA_PROC_RET_ON_DUP = (1 << 1),
};

enum {
    /* Miscellaenous */
    LUA_GL_ClearColor = 0,
    LUA_GL_ClearDepth,
    LUA_GL_Clear,
    LUA_GL_BlendFunc,
    LUA_GL_LogicOp,
    LUA_GL_CullFace,
    LUA_GL_FrontFace,
    LUA_GL_PolygonMode,
    LUA_GL_Scissor,
    LUA_GL_DrawBuffer,
    LUA_GL_ReadBuffer,
    LUA_GL_Enable,
    LUA_GL_Disable,

    LUA_GL_EnableClientState,
    LUA_GL_DisableClientState,

    LUA_GL_GetBooleanv,
    LUA_GL_GetDoublev,
    LUA_GL_Getfloatv,
    LUA_GL_GetIntegerv,
    LUA_GL_GetError,
    LUA_GL_GetString,

    LUA_GL_GetStringi, /* 3.0 */

    /* Depth */
    LUA_GL_DepthFunc,
    LUA_GL_DepthMask,
    LUA_GL_DepthRange,

    /* Transformation */
    LUA_GL_Viewport,
    LUA_GL_MatrixMode,
    LUA_GL_PushMatrix,
    LUA_GL_PopMatrix,
    LUA_GL_LoadIdentity,

    LUA_GL_LoadMatrixd,
    LUA_GL_MultMatrixd,
    LUA_GL_Rotated,
    LUA_GL_Scaled,
    LUA_GL_Translated,

    LUA_GL_LoadMatrixf,
    LUA_GL_MultMatrixf,
    LUA_GL_Rotatef,
    LUA_GL_Scalef,
    LUA_GL_Translatef,

    LUA_GL_Ortho,
    LUA_GL_Frustum,

    LUA_GL_Orthof, /* GL ES */
    LUA_GL_Frustumf, /* GL ES */

    LUA_GL_LoadTransposeMatrixd,
    LUA_GL_MultTransposeMatrixd,
    LUA_GL_LoadTransposeMatrixf,
    LUA_GL_MultTransposeMatrixf,

    /* Vertex arrays */
    LUA_GL_VertexPointer,
    LUA_GL_NormalPointer,
    LUA_GL_ColorPointer,
    LUA_GL_TexCoordPointer,
    LUA_GL_IndexPointer,
    LUA_GL_EdgeFlagPointer,

    LUA_GL_DrawArrays,
    LUA_GL_DrawElements,

    /* Texture mapping */
    LUA_GL_TexParameterf,
    LUA_GL_TexParameteri,
    LUA_GL_TexParameterfv,
    LUA_GL_TexParameteriv,

    LUA_GL_GetTexParameteriv,
    LUA_GL_GetTexParameterfv,

    LUA_GL_GenTextures,
    LUA_GL_DeleteTextures,
    LUA_GL_BindTexture,
    LUA_GL_IsTexture,

    LUA_GL_TexImage1D,
    LUA_GL_TexImage2D,
    LUA_GL_TexSubImage1D,
    LUA_GL_TexSubImage2D,
    LUA_GL_CopyTexImage1D,
    LUA_GL_CopyTexImage2D,
    LUA_GL_CopyTexSubImage1D,
    LUA_GL_CopyTexSubImage2D,

    LUA_GL_TexImage3D,
    LUA_GL_TexSubImage3D,
    LUA_GL_CopyTexSubImage3D,

    /* GL_ARB_vertex_buffer_object */
    LUA_GL_BindBuffer,
    LUA_GL_DeleteBuffers,
    LUA_GL_GenBuffers,
    LUA_GL_BufferData,
    LUA_GL_BufferSubData,
    LUA_GL_MapBuffer,
    LUA_GL_UnmapBuffer,

    /* GL_ARB_vertex_program */
    LUA_GL_VertexAttribPointer,
    LUA_GL_EnableVertexAttribArray,
    LUA_GL_DisableVertexAttribArray,

    /* GL_ARB_vertex_shader */
    LUA_GL_BindAttribLocation,
    LUA_GL_GetAttribLocation,
    LUA_GL_GetActiveAttrib,

    /* GL_EXT_framebuffer_object */
    LUA_GL_GenFramebuffers,
    LUA_GL_DeleteFramebuffers,
    LUA_GL_BindFramebuffer,
    LUA_GL_CheckFramebufferStatus,
    LUA_GL_FramebufferTexture2D,
    LUA_GL_FramebufferRenderbuffer,
    LUA_GL_GenerateMipmap,
    LUA_GL_BlitFramebuffer,
    LUA_GL_IsFramebuffer,

    LUA_GL_GenRenderbuffers,
    LUA_GL_DeleteRenderbuffers,
    LUA_GL_BindRenderbuffer,
    LUA_GL_RenderbufferStorage,
    LUA_GL_RenderbufferStorageMultisample,
    LUA_GL_IsRenderbuffer,

    /* GL_ARB_shader_objects */
    LUA_GL_CreateProgram,
    LUA_GL_DeleteProgram,
    LUA_GL_UseProgram,
    LUA_GL_CreateShader,
    LUA_GL_DeleteShader,
    LUA_GL_ShaderSource,
    LUA_GL_CompileShader,
    LUA_GL_GetShaderiv,
    LUA_GL_GetShaderInfoLog,
    LUA_GL_AttachShader,
    LUA_GL_DetachShader,
    LUA_GL_LinkProgram,
    LUA_GL_GetProgramiv,
    LUA_GL_GetProgramInfoLog,
    LUA_GL_GetUniformLocation,
    LUA_GL_GetUniformLocation,
    LUA_GL_GetActiveUniform,
    LUA_GL_Uniform1f,
    LUA_GL_Uniform2f,
    LUA_GL_Uniform3f,
    LUA_GL_Uniform4f,
    LUA_GL_Uniform1i,
    LUA_GL_Uniform2i,
    LUA_GL_Uniform3i,
    LUA_GL_Uniform4i,
    LUA_GL_Uniform1fv,
    LUA_GL_Uniform2fv,
    LUA_GL_Uniform3fv,
    LUA_GL_Uniform4fv,
    LUA_GL_Uniform1iv,
    LUA_GL_Uniform2iv,
    LUA_GL_Uniform3iv,
    LUA_GL_Uniform4iv,
    LUA_GL_UniformMatrix2fv,
    LUA_GL_UniformMatrix3fv,
    LUA_GL_UniformMatrix4fv,

    /* GL_ARB_vertex_array_object */
    LUA_GL_BindVertexArray,
    LUA_GL_DeleteVertexArrays,
    LUA_GL_GenVertexArrays,

    LUA_GL_PROC_COUNT,
};

enum {
    LUA_HAS_VBO = (1 << 0),
    LUA_HAS_VAO = (1 << 1),
    LUA_HAS_SHADER = (1 << 2)
};

/* Miscellaneous */
typedef void(*GLClearColorProc)(float, float, float, float);
typedef void(*GLClearProc)(unsigned int);
typedef void(*GLBlendFuncProc)(unsigned int, unsigned int);
typedef void(*GLLogicOpProc)(unsigned int);
typedef void(*GLCullFaceProc)(unsigned int);
typedef void(*GLFrontFaceProc)(unsigned int);
typedef void(*GLPolygonModeProc)(unsigned int, unsigned int);
typedef void(*GLScissorProc)(int, int, int, int);
typedef void(*GLDrawBufferProc)(unsigned int);
typedef void(*GLReadBufferProc)(unsigned int);
typedef void(*GLEnableProc)(unsigned int);
typedef void(*GLDisableProc)(unsigned int);

typedef void(*GLEnableClientStateProc)(unsigned int); /* 1.1 */
typedef void(*GLDisableClientStateProc)(unsigned int); /* 1.1 */

typedef void(*GLGetBooleanvProc)(unsigned int, char*);
typedef void(*GLGetDoublevProc)(unsigned int, double*);
typedef void(*GLGetfloatvProc)(unsigned int, float*);
typedef void(*GLGetIntegervProc)(unsigned int, int*);
typedef void(*GLGetErrorProc)(void);
typedef const unsigned char* (*GLGetStringProc)(unsigned int);

typedef const unsigned char* (*GLGetStringiProc)(unsigned int, unsigned int); /* 3.0 */

/* Depth buffer */
typedef void(*GLClearDepthProc)(float);
typedef void(*GLDepthFuncProc)(unsigned int);
typedef void(*GLDepthMaskProc)(char);
typedef void(*GLDepthRangeProc)(double, double);

/* Transformation */
typedef void(*GLViewportProc)(int, int, int, int);
typedef void(*GLMatrixModeProc)(unsigned int);
typedef void(*GLPushMatrixProc)(void);
typedef void(*GLPopMatrixProc)(void);
typedef void(*GLLoadIdentityProc)(void);
typedef void(*GLLoadMatrixfProc)(const float*);
typedef void(*GLLoadMatrixdProc)(const double*);
typedef void(*GLMultMatrixfProc)(const float*);
typedef void(*GLMultMatrixdProc)(const double*);
typedef void(*GLOrthoProc)(double, double, double, double, double, double);
typedef void(*GLFrustumProc)(double, double, double, double, double, double);
typedef void(*GLTranslatefProc)(float, float, float);
typedef void(*GLRotatefProc)(float, float, float, float);
typedef void(*GLScalefProc)(float, float, float);
typedef void(*GLTranslatedProc)(double, double, double);
typedef void(*GLRotatedProc)(double, double, double, double);
typedef void(*GLScaledProc)(double, double, double);

typedef void(*GLLoadTransposeMatrixdProc)(const double[16]); /* 1.3 */
typedef void(*GLLoadTransposeMatrixfProc)(const float[16]); /* 1.3 */
typedef void(*GLMultTransposeMatrixdProc)(const double[16]); /* 1.3 */
typedef void(*GLMultTransposeMatrixfProc)(const float[16]); /* 1.3 */

/* Vertex Arrays */
typedef void(*GLVertexPointerProc)(int, unsigned int, int, const void*);
typedef void(*GLColorPointerProc)(int, unsigned int, int, const void*);
typedef void(*GLTexCoordPointerProc)(int, unsigned int, int, const void*);
typedef void(*GLNormalPointerProc)(unsigned int, int, const void*);
typedef void(*GLIndexPointerProc)(unsigned int, int, const void*);
typedef void(*GLEdgeFlagPointerProc)(int, int, const void*);

typedef void(*GLDrawArraysProc)(unsigned int, int, int);
typedef void(*GLDrawElementsProc)(unsigned int, int, unsigned int, const void*);

/* Texture mapping */
typedef void(*GLTexParameterfProc)(unsigned int, unsigned int, float);
typedef void(*GLTexParameteriProc)(unsigned int, unsigned int, int);
typedef void(*GLTexParameterfvProc)(unsigned int, unsigned int, const float*);
typedef void(*GLTexParameterivProc)(unsigned int, unsigned int, const int*);

typedef void(*GLGetTexParameterfProc)(unsigned int, unsigned int, float*);
typedef void(*GLGetTexParameteriProc)(unsigned int, unsigned int, int*);
typedef void(*GLGetTexParameterfvProc)(unsigned int, unsigned int, float*);
typedef void(*GLGetTexParameterivProc)(unsigned int, unsigned int, int*);

typedef void(*GLGenTexturesProc)(unsigned int, unsigned int*);
typedef void(*GLDeleteTexturesProc)(unsigned int, unsigned int*);
typedef void(*GLBindTextureProc)(unsigned int, unsigned int);
typedef char(*GLIsTextureProc)(unsigned int);
typedef void(*GLTexImage1DProc)(unsigned int, int, int, int, int, unsigned int, unsigned int, const void*);
typedef void(*GLTexImage2DProc)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*);
typedef void(*GLTexImage3DProc)(unsigned int, int, int, int, int, int, int, unsigned int, unsigned int, const void*);
typedef void(*GLTexSubImage1DProc)(unsigned int, int, int, int, unsigned int, unsigned int, const void*);
typedef void(*GLTexSubImage2DProc)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*);
typedef void(*GLTexSubImage3DProc)(unsigned int, int, int, int, int, int, int, unsigned int, unsigned int, const void*);
typedef void(*GLCopyTexImage1DProc)(unsigned int, int, unsigned int, int, int, int, int);
typedef void(*GLCopyTexImage2DProc)(unsigned int, int, unsigned int, int, int, int, int, int);
typedef void(*GLCopyTexSubImage1DProc)(unsigned int, int, int, int, int, int);
typedef void(*GLCopyTexSubImage2DProc)(unsigned int, int, int, int, int, int, int, int);
typedef void(*GLCopyTexSubImage3DProc)(unsigned int, int, int, int, int, int, int, int, int, int);

/* GL_ARB_vertex_buffer_object */
typedef void(*GLBindBufferProc)(unsigned int, unsigned int);
typedef void(*GLBufferDataProc)(unsigned int, unsigned int, const void*, unsigned int);
typedef void(*GLBufferSubDataProc)(unsigned int, unsigned int, unsigned int, const void*);
typedef void(*GLGenBuffersProc)(unsigned int, unsigned int*);
typedef void(*GLDeleteBuffersProc)(unsigned int, unsigned int*);
typedef void* (*GLMapBufferProc)(unsigned int, unsigned int);
typedef unsigned int(*GLUnmapBufferProc)(unsigned int);

/* GL_ARB_vertex_array_object */
typedef void(*GLGenVertexArraysProc)(unsigned int, unsigned int*);
typedef void(*GLBindVertexArrayProc)(unsigned int);
typedef void(*GLDeleteVertexArraysProc)(unsigned int, unsigned int*);

/* GL_ARB_vertex_array_program */
typedef void(*GLVertexAttribPointerProc)(unsigned int, int, unsigned int, int, int, const void*);
typedef void(*GLEnableVertexAttribArrayProc)(unsigned int);
typedef void(*GLDisableVertexAttribArrayProc)(unsigned int);

/* GL_EXT_framebuffer_object */
typedef char(*GLIsRenderbufferProc)(unsigned int);
typedef void(*GLBindRenderbufferProc)(unsigned int, unsigned int);
typedef void(*GLDeleteRenderbuffersProc)(unsigned int, unsigned int*);
typedef void(*GLGenRenderbuffersProc)(unsigned int, unsigned int*);
typedef void(*GLRenderbufferStorageProc)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef void(*GLGetRenderbufferParameterivProc)(unsigned int, unsigned int, int*);

typedef char(*GLIsFramebufferProc)(unsigned int);
typedef void(*GLBindFramebufferProc)(unsigned int, unsigned int);
typedef void(*GLDeleteFramebuffersProc)(unsigned int, unsigned int*);
typedef void(*GLGenFramebuffersProc)(unsigned int, unsigned int*);
typedef void(*GLFramebufferRenderbufferProc)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef void(*GLFramebufferTexture1DProc)(unsigned int, unsigned int, unsigned int, unsigned int, int);
typedef void(*GLFramebufferTexture2DProc)(unsigned int, unsigned int, unsigned int, unsigned int, int);
typedef void(*GLFramebufferTexture3DProc)(unsigned int, unsigned int, unsigned int, unsigned int, int, int);
typedef void(*GLFramebufferTextureLayerProc)(unsigned int, unsigned int, unsigned int, int, int);
typedef unsigned int(*GLCheckFramebufferStatusProc)(unsigned int);
typedef void(*GLGetFramebufferAttachmentParameterivProc)(unsigned int, unsigned int, unsigned int, int*);
typedef void(*GLBlitFramebufferProc)(int, int, int, int, int, int, int, int, unsigned int);
typedef void(*GLGenerateMipmapProc)(unsigned int);

/* GL_ARB_shader_objects */
typedef void(*GLDeleteShaderProc)(unsigned int);
typedef unsigned int(*GLCreateShaderProc)(unsigned int);
typedef void(*GLShaderSourceProc)(unsigned int, int, const char**, const int*);
typedef void(*GLCompileShaderProc)(unsigned int);
typedef unsigned int(*GLGetShaderivProc)(unsigned int, unsigned int, int*);
typedef unsigned int(*GLGetShaderInfoLogProc)(unsigned int, int, int*, char*);

typedef unsigned int(*GLCreateProgramProc)(void);
typedef void(*GLDeleteProgramProc)(unsigned int);
typedef void(*GLAttachShaderProc)(unsigned int, unsigned int);
typedef void(*GLDetachShaderProc)(unsigned int, unsigned int);
typedef void(*GLLinkProgramProc)(unsigned int);
typedef void(*GLUseProgramProc)(unsigned int);

typedef void(*GLGetProgramivProc)(unsigned int, unsigned int, int*);
typedef void(*GLGetProgramInfoLogProc)(unsigned int, int, int*, char*);
typedef void(*GLGetActiveUniformProc)(unsigned int, unsigned int, int, int*, int*, int*, char*);
typedef int(*GLGetUniformLocationProc)(unsigned int, const char*);

#define GL_UNIFORM_X(X, T)\
typedef void(*GLUniform1##X##Proc)(int, T);\
typedef void(*GLUniform2##X##Proc)(int, T, T);\
typedef void(*GLUniform3##X##Proc)(int, T, T, T);\
typedef void(*GLUniform4##X##Proc)(int, T, T, T, T);\
typedef void(*GLUniform1##X##vProc)(int, int value, const T*);\
typedef void(*GLUniform2##X##vProc)(int, int value, const T*);\
typedef void(*GLUniform3##X##vProc)(int, int value, const T*);\
typedef void(*GLUniform4##X##vProc)(int, int value, const T*)

GL_UNIFORM_X(f, float);
GL_UNIFORM_X(i, int);

typedef void(*GLUniformMatrix2fvProc)(int, int, char, const float*);
typedef void(*GLUniformMatrix3fvProc)(int, int, char, const float*);
typedef void(*GLUniformMatrix4fvProc)(int, int, char, const float*);

/* GL_ARB_vertex_shader */
typedef int(*GLGetAttribLocationProc)(unsigned int prog, const char* name);
typedef void(*GLGetActiveAttribProc)(unsigned int prog, unsigned int index, int bufSize, int* length, int* size, unsigned int* type, char* name);
typedef void(*GLBindAttribLocationProc)(unsigned int prog, unsigned int index, const char* name);

typedef struct proc_s proc_t;
struct proc_s {
    unsigned char tag;
    const char* names[3];
};

static struct {
    char mag, min;
} s_gl_max_ver;

struct gl_s {
    struct {
        unsigned char major, minor;
        te_u16 glsl;
        char es;
    } version;
    unsigned int extensions;
    void* procs[LUA_GL_PROC_COUNT];
};

struct gl_s _ctx;

#define gl() (&_ctx)

static void* s_get_proc(const char* name);
static char s_load_gl(void);
static void s_setup_gl(void);
static void s_close_gl(void);

/* data types */
#define LUA_GL_BYTE           0x1400
#define LUA_GL_UNSIGNED_BYTE  0x1401
#define LUA_GL_SHORT          0x1402
#define LUA_GL_UNSIGNED_SHORT 0x1403
#define LUA_GL_INT            0x1404
#define LUA_GL_UNSIGNED_INT   0x1405
#define LUA_GL_FLOAT          0x1406
#define LUA_GL_2_BYTES        0x1407
#define LUA_GL_3_BYTES        0x1408
#define LUA_GL_4_BYTES        0x1409
#define LUA_GL_DOUBLE         0x140A

/* Primitives */
#define LUA_GL_POINTS         0x0000
#define LUA_GL_LINES          0x0001
#define LUA_GL_LINE_LOOP      0x0002
#define LUA_GL_LINE_STRIP     0x0003
#define LUA_GL_TRIANGLES      0x0004
#define LUA_GL_TRIANGLE_STRIP 0x0005
#define LUA_GL_TRIANGLE_FAN   0x0006
#define LUA_GL_QUADS          0x0007
#define LUA_GL_QUAD_STRIP     0x0008
#define LUA_GL_POLYGON        0x0009

/* Clear buffer bits */
#define LUA_GL_DEPTH_BUFFER_BIT   0x00000100
#define LUA_GL_ACCUM_BUFFER_BIT   0x00000200
#define LUA_GL_STENCIL_BUFFER_BIT 0x00000400
#define LUA_GL_COLOR_BUFFER_BIT   0x00004000

#define LUA_GL_RGB         0x1907
#define LUA_GL_RGBA        0x1908

/* bgra */
#define LUA_GL_BGR  0x80E0
#define LUA_GL_BGRA 0x80E1

#define LUA_GL_ARRAY_BUFFER 0x8892
#define LUA_GL_ELEMENT_ARRAY_BUFFER 0x8893

#define LUA_GL_STREAM_DRAW  0x88E0
#define LUA_GL_STREAM_READ  0x88E1
#define LUA_GL_STREAM_COPY  0x88E2
#define LUA_GL_STATIC_DRAW  0x88E4
#define LUA_GL_STATIC_READ  0x88E5
#define LUA_GL_STATIC_COPY  0x88E6
#define LUA_GL_DYNAMIC_DRAW 0x88E8
#define LUA_GL_DYNAMIC_READ 0x88E9
#define LUA_GL_DYNAMIC_COPY 0x88EA

#define LUA_GL_TEXTURE_2D 0x0DE1
#define LUA_GL_TEXTURE_MIN_FILTER 0x2800
#define LUA_GL_TEXTURE_MAG_FILTER 0x2801
#define LUA_GL_TEXTURE_WRAP_S 0x2802
#define LUA_GL_TEXTURE_WRAP_T 0x2803

#define LUA_GL_NEAREST 0x2600
#define LUA_GL_REPEAT 0x2901
#define LUA_GL_CLAMP 0x2900

#define LUA_GL_CLAMP_TO_EDGE 0x812F /* 1.2 */
#define LUA_CLAMP_TO_BORDER  0x812D /* 1.3 */

static int l_gl_enable(lua_State* L) {
	int flags = 0;
	int i;
	for (i = 0; i < lua_gettop(L); i++) {
		flags |= luaL_checkinteger(L, i + 1);
	}

    CALL_GL(Enable)(flags);
	return 0;
}

static int l_gl_disable(lua_State* L) {
	int flags = 0;
	int i;
	for (i = 0; i < lua_gettop(L); i++) {
		flags |= luaL_checkinteger(L, i + 1);
	}

    CALL_GL(Disable)(flags);
	return 0;
}

int luaopen_gl(lua_State* L) {
    if (s_load_gl()) {
        s_setup_gl();
        s_close_gl();
    }

	luaL_Reg reg[] = {
		{"Enable", l_gl_enable},
		{"Disable", l_gl_disable},
		{NULL, NULL}
	};

	luaL_newlib(L, reg);

	return 1;
}

/*=================================*
 *             Loader              *
 *=================================*/

void s_next_command(void) {
    if (tea()->command.index + 1 > tea()->command.count) {
        tea()->command.count *= 2;
        tea()->command.pool = LUA_REALLOC(tea()->command.pool, tea()->command.count * sizeof(te_draw_command_t));
    }
    te_draw_command_t* old = tea()->command.pool + tea()->command.index;
    tea()->command.index++;
    te_draw_command_t* current = tea()->command.pool + tea()->command.index;
    memcpy(current, old, sizeof * current);
    current->start = old->end;
    current->end = old->end;
    current->clear_needed = LUA_FALSE;
}

void s_next_list(void) {
    te_draw_command_t* command = tea()->command.pool + tea()->command.index;
    unsigned int index = command->end;
    if (index + 1 > tea()->command.count) {
        tea()->command.count *= 2;
        tea()->command.pool = LUA_REALLOC(tea()->command.pool, tea()->command.count * sizeof(te_draw_command_t));
    }
    te_draw_list_t* old = tea()->list.pool + index;
    command->end++;
    te_draw_list_t* current = tea()->list.pool + command->end;
    memcpy(current, old, sizeof * current);
}

static char s_lua_load_procs(proc_t* procs, unsigned int flags);

#if !defined(__APPLE__) && !defined(__HAIKU__)
void* (*s_proc_address)(const char*);
#endif

char s_load_gl(void) {
#if defined(_WIN32)
    s_glsym = LoadLibrary("opengl32.dll");
    LUA_ASSERT(s_glsym != NULL, "failed to load OpenGL32.dll");
#else
#if defined(__APPLE__)
    const char* names[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/Opengl",
        "/System/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL",
        NULL,
    };
#else
    const char* names[] = {
        "libGL.so.1",
        "libGL.so",
        NULL,
    };
#endif
    unsigned int index;
    for (index = 0; names[index] != NULL; ++index) {
        s_glsym = dlopen(names[index], RTLD_LAZY | RTLD_GLOBAL);
        if (s_glsym != NULL) {
#if defined(__APPLE__) || defined(__HAIKU__)
            return 1;
#else
            s_proc_address = (void* (*)(const char*))dlsym(s_glsym, "glXGetProcAddress");
            return s_proc_address != NULL;
#endif
            break;
        }
    }
#endif
    return 0;
}

void s_close_gl(void) {
    if (s_glsym != NULL) {
#if defined(_WIN32)
        FreeLibrary(s_glsym);
#else
        dlclose(s_glsym);
#endif
        s_glsym = NULL;
    }
}

void s_setup_gl(void) {
    GET_GL(GetString) = s_get_proc("glGetString");
    GET_GL(GetStringi) = s_get_proc("glGetStringi");

    const char* version = (const char*)CALL_GL(GetString)(LUA_GL_VERSION);
    const char* glsl = (const char*)CALL_GL(GetString)(LUA_GL_SHADING_LANGUAGE_VERSION);
    // LUA_ASSERT(version != NULL, "Failed to get OpenGL version");
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL,
    };

    char* ver = (char*)version;
    for (unsigned int i = 0; prefixes[i] != NULL; i++) {
        if (strncmp(ver, prefixes[i], strlen(prefixes[i])) == 0) {
            ver += strlen(prefixes[i]);
            gl()->version.es = LUA_TRUE;
            break;
        }
    }
    s_gl_max_ver.mag = ver[0] - '0';
    s_gl_max_ver.min = ver[2] - '0';
    if (gl()->version.major == 0) {
        gl()->version.major = s_gl_max_ver.mag;
        gl()->version.minor = s_gl_max_ver.min;
    }

    fprintf(stderr, "OpenGL version: %s\n", version);
    fprintf(stderr, "OpenGL shading language version: %s\n", glsl);
    static const proc_t glBaseProcs[] = {
        /* Miscellaneous */
        { LUA_GL_ClearColor, { "glClearColor", NULL }},
        { LUA_GL_ClearDepth, { "glClearDepth", NULL }},
        { LUA_GL_Clear, { "glClear", NULL }},
        { LUA_GL_CullFace, { "glCullFace", NULL }},
        { LUA_GL_FrontFace, { "glFrontFace", NULL }},
        { LUA_GL_PolygonMode, { "glPolygonMode", NULL }},
        { LUA_GL_Scissor, { "glScissor", NULL }},
        { LUA_GL_ReadBuffer, { "glReadBuffer", NULL }},
        { LUA_GL_DrawBuffer, { "glDrawBuffer", NULL }},
        { LUA_GL_Enable, { "glEnable", NULL }},
        { LUA_GL_Disable, { "glDisable", NULL }},
        { LUA_GL_EnableClientState, { "glEnableClientState", NULL }},
        { LUA_GL_DisableClientState, { "glDisableClientState", NULL }},
        { LUA_GL_GetBooleanv, { "glGetBooleanv", NULL }},
        { LUA_GL_Gette_f32v, { "glGette_f32v", NULL }},
        { LUA_GL_GetIntegerv, { "glGetIntegerv", NULL }},
        { LUA_GL_GetError, { "glGetError" }},
        /* Depth */
        { LUA_GL_DepthFunc, { "glDepthFunc", NULL }},
        { LUA_GL_DepthMask, { "glDepthMask", NULL }},
        { LUA_GL_DepthRange, { "glDepthRange", NULL }},
        /* Transformation */
        { LUA_GL_Viewport, { "glViewport", NULL }},
        { LUA_GL_MatrixMode, { "glMatrixMode", NULL }},
        { LUA_GL_LoadIdentity, { "glLoadIdentity", NULL }},
        { LUA_GL_LoadMatrixf, { "glLoadMatrixf", NULL }},
        { LUA_GL_LoadMatrixd, { "glLoadMatrixd", NULL }},
        { LUA_GL_MultMatrixf, { "glMultMatrixf", NULL }},
        { LUA_GL_MultMatrixd, { "glMultMatrixd", NULL }},
        { LUA_GL_Rotatef, { "glRotatef", NULL }},
        { LUA_GL_Rotated, { "glRotated", NULL }},
        { LUA_GL_Scalef, { "glScalef", NULL }},
        { LUA_GL_Scaled, { "glScaled", NULL }},
        { LUA_GL_Translatef, { "glTranslatef", NULL }},
        { LUA_GL_Translated, { "glTranslated", NULL }},
        { LUA_GL_Ortho, { "glOrtho", NULL }},
        { LUA_GL_Frustum, { "glFrustum", NULL }},
        { LUA_GL_Orthof, { "glOrthof", NULL }},
        { LUA_GL_Frustumf, { "glFrustumf", NULL }},
        { LUA_GL_PushMatrix, { "glPushMatrix", NULL }},
        { LUA_GL_PopMatrix, { "glPopMatrix", NULL }},
        { LUA_GL_LoadTransposeMatrixd, { "glLoadTransposeMatrixd", NULL }},
        { LUA_GL_LoadTransposeMatrixf, { "glLoadTransposeMatrixf", NULL }},
        { LUA_GL_MultTransposeMatrixd, { "glMultTransposeMatrixd", NULL }},
        { LUA_GL_MultTransposeMatrixf, { "glMultTransposeMatrixf", NULL }},
        /* Vertex arrays */
        { LUA_GL_VertexPointer, { "glVertexPointer", NULL }},
        { LUA_GL_NormalPointer, { "glNormalPointer", NULL }},
        { LUA_GL_ColorPointer, { "glColorPointer", NULL }},
        { LUA_GL_TexCoordPointer, { "glTexCoordPointer", NULL }},
        { LUA_GL_IndexPointer, { "glIndexPointer", NULL }},
        { LUA_GL_EdgeFlagPointer, { "glEdgeFlatPointer", NULL }},
        { LUA_GL_DrawArrays, { "glDrawArrays", NULL }},
        { LUA_GL_DrawElements, { "glDrawElements", NULL }},
    #if 0
        { LUA_GL_InterleavedArrays, { "glInterleavedArrays", NULL }},
        { LUA_GL_ClientActiveTexture, { "glClientActiveTexture", NULL }},
        { LUA_GL_ActiveTexture, { "glActiveTexture", NULL }},
        { LUA_GL_MultiTexCoord1d, { "glMultiTexCoord1d", NULL }},
        { LUA_GL_MultiTexCoord1dv, { "glMultiTexCoord1dv", NULL }},
    #endif
        /* Texture mapping */
        { LUA_GL_TexParameterf, { "glTexParameterf", NULL }},
        { LUA_GL_TexParameteri, { "glTexParameteri", NULL } },
        { LUA_GL_TexParameterfv, { "glTexParameterfv", NULL }},
        { LUA_GL_TexParameteriv, { "glTexParameteriv", NULL }},
        { LUA_GL_GetTexParameterfv, { "glGetTexParameterfv", NULL }},
        { LUA_GL_GetTexParameteriv, { "glGetTexParameteriv", NULL }},
        { LUA_GL_GenTextures, { "glGenTextures", NULL }},
        { LUA_GL_DeleteTextures, { "glDeleteTextures", NULL }},
        { LUA_GL_BindTexture, { "glBindTexture", NULL }},
        { LUA_GL_IsTexture, { "glIsTexture", NULL }},
        { LUA_GL_TexImage1D, { "glTexImage1D", NULL }},
        { LUA_GL_TexImage2D, { "glTexImage2D", NULL }},
        { LUA_GL_TexSubImage1D, { "glTexSubImage1D", NULL }},
        { LUA_GL_TexSubImage2D, { "glTexSubImage2D", NULL }},
        { LUA_GL_CopyTexImage1D, { "glCopyTexImage1D", NULL }},
        { LUA_GL_CopyTexImage2D, { "glCopyTexImage2D", NULL }},
        { LUA_GL_CopyTexSubImage1D, { "glCopyTexSubImage1D", NULL }},
        { LUA_GL_CopyTexSubImage2D, { "glCopyTexSubImage2D", NULL }},
        { LUA_GL_TexImage3D, { "glTexImage3D", NULL }},
        { LUA_GL_TexSubImage3D, { "glTexSubImage3D", NULL }},
        { LUA_GL_CopyTexSubImage3D, { "glCopyTexSubImage3D", NULL }},
        /* Vertex buffer object */
        { LUA_GL_GenBuffers, { "glGenBuffers", "glGenBuffersARB", NULL }},
        { LUA_GL_DeleteBuffers, { "glDeleteBuffers", "glDeleteBuffersARB", NULL }},
        { LUA_GL_BindBuffer, { "glBindBuffer", "glBindBufferARB", NULL }},
        { LUA_GL_BufferData, { "glBufferData", "glBufferDataARB", NULL }},
        { LUA_GL_BufferSubData, { "glBufferSubData", "glBufferSubDataARB", NULL }},
        { LUA_GL_MapBuffer, { "glMapBuffer", "glMapBufferARB", NULL }},
        { LUA_GL_UnmapBuffer, { "glUnmapBuffer", "glUnmapBufferARB", NULL }},
        /* Vertex program */
        { LUA_GL_VertexAttribPointer, { "glVertexAttribPointer", "glVertexAttribPointerARB", NULL }},
        { LUA_GL_EnableVertexAttribArray, { "glEnableVertexAttribArray", "glEnableVertexAttribArrayARB", NULL }},
        { LUA_GL_DisableVertexAttribArray, { "glDisableVertexAttribArray", "glDisableVertexAttribArrayARB", NULL }},
        /* Framebuffer */
        { LUA_GL_IsRenderbuffer, { "glIsRenderbuffer", "glIsRenderbufferEXT", NULL }},
        { LUA_GL_BindRenderbuffer, { "glBindRenderbuffer", "glBindRenderbufferEXT", NULL }},
        { LUA_GL_DeleteRenderbuffers, { "glDeleteRenderbuffers", "glDeleteRenderbuffersEXT", NULL }},
        { LUA_GL_GenRenderbuffers, { "glGenRenderbuffers", "glGenRenderbuffersEXT", NULL }},
        { LUA_GL_RenderbufferStorage, { "glRenderbufferStorage", "glRenderbufferStorageEXT", NULL }},
        { LUA_GL_IsFramebuffer, { "glIsFramebuffer", "glIsFramebufferEXT", NULL }},
        { LUA_GL_BindFramebuffer, { "glBindFramebuffer", "glBindFramebufferEXT", NULL }},
        { LUA_GL_DeleteFramebuffers, { "glDeleteFramebuffers", "glDeleteFramebuffersEXT", NULL }},
        { LUA_GL_GenFramebuffers, { "glGenFramebuffers", "glGenFramebuffersEXT", NULL }},
        { LUA_GL_CheckFramebufferStatus, { "glCheckFramebufferStatus", "glCheckFramebufferStatusEXT", NULL }},
        { LUA_GL_FramebufferTexture2D, { "glFramebufferTexture2D", "glFramebufferTexture2DEXT", NULL }},
        { LUA_GL_FramebufferRenderbuffer, { "glFramebufferRenderbuffer", "glFramebufferRenderbufferEXT", NULL }},
        { LUA_GL_GenerateMipmap, { "glGenerateMipmap", "glGenerateMipmapEXT", NULL }},
        { LUA_GL_BlitFramebuffer, { "glBlitFramebuffer", "glBlitFramebufferEXT", NULL }},
        /* Shader */
        { LUA_GL_CreateShader, { "glCreateShader", "glCreateShaderObjectARB", NULL }},
        { LUA_GL_DeleteShader, { "glDeleteShader", "glDeleteObjectARB", NULL }},
        { LUA_GL_ShaderSource, { "glShaderSource", "glShaderSourcerARB", NULL }},
        { LUA_GL_CompileShader, { "glCompileShader", "glCompileShaderARB", NULL }},
        { LUA_GL_GetShaderiv, { "glGetShaderiv", "glGetObjectParameterivARB", NULL }},
        { LUA_GL_GetShaderInfoLog, { "glGetShaderInfoLog", "glGetInfoLogARB", NULL }},
        { LUA_GL_CreateProgram, { "glCreateProgram", "glCreateProgramObjectARB", NULL }},
        { LUA_GL_DeleteProgram, { "glDeleteProgram", "glDeleteObjectARB", NULL }},
        { LUA_GL_AttachShader, { "glAttachShader", "glAttachObjectARB", NULL }},
        { LUA_GL_DetachShader, { "glDetachShader", "glDetachObjectARB", NULL }},
        { LUA_GL_LinkProgram, { "glLinkProgram", "glLinkProgramARB", NULL }},
        { LUA_GL_GetProgramiv, { "glGetProgramiv", "glGetObjectParameterivARB", NULL }},
        { LUA_GL_GetProgramInfoLog, { "glGetProgramInfoLog", "glGetInfoLogARB", NULL }},
        { LUA_GL_UseProgram, { "glUseProgram", "glUseProgramObjectARB", NULL }},
        { LUA_GL_GetUniformLocation, { "glGetUniformLocation", "glGetUniformLocationARB", NULL }},
        { LUA_GL_Uniform1f, { "glUniform1f", "glUniform1fARB", NULL }},
        { LUA_GL_Uniform2f, { "glUniform2f", "glUniform2fARB", NULL }},
        { LUA_GL_Uniform3f, { "glUniform3f", "glUniform3fARB", NULL }},
        { LUA_GL_Uniform4f, { "glUniform4f", "glUniform4fARB", NULL }},
        { LUA_GL_Uniform1i, { "glUniform1i", "glUniform1iARB", NULL }},
        { LUA_GL_Uniform2i, { "glUniform2i", "glUniform2iARB", NULL }},
        { LUA_GL_Uniform3i, { "glUniform3i", "glUniform3iARB", NULL }},
        { LUA_GL_Uniform4i, { "glUniform4i", "glUniform4iARB", NULL }},
        { LUA_GL_Uniform1fv, { "glUniform1fv", "glUniform1fvARB", NULL }},
        { LUA_GL_Uniform2fv, { "glUniform2fv", "glUniform2fvARB", NULL }},
        { LUA_GL_Uniform3fv, { "glUniform3fv", "glUniform3fvARB", NULL }},
        { LUA_GL_Uniform4fv, { "glUniform4fv", "glUniform4fvARB", NULL }},
        { LUA_GL_Uniform1iv, { "glUniform1iv", "glUniform1ivARB", NULL }},
        { LUA_GL_Uniform2iv, { "glUniform2iv", "glUniform2ivARB", NULL }},
        { LUA_GL_Uniform3iv, { "glUniform3iv", "glUniform3ivARB", NULL }},
        { LUA_GL_Uniform4iv, { "glUniform4iv", "glUniform4ivARB", NULL }},
        { LUA_GL_UniformMatrix2fv, { "glUniformMatrix2fv", "glUniformMatrix2fvARB", NULL }},
        { LUA_GL_UniformMatrix3fv, { "glUniformMatrix3fv", "glUniformMatrix3fvARB", NULL }},
        { LUA_GL_UniformMatrix4fv, { "glUniformMatrix4fv", "glUniformMatrix4fvARB", NULL }},
        { LUA_GL_GetAttribLocation, { "glGetAttribLocation", "glGetAttribLocationARB", NULL }},
        { LUA_GL_BindAttribLocation, { "glBindAttribLocation", "glBindAttribLocationARB", NULL }},
        { LUA_GL_GetActiveUniform, { "glGetActiveUniform", "glGetActiveUniformARB", NULL }},
        { LUA_GL_GetActiveAttrib, { "glGetActiveAttrib", "glGetActiveAttribARB", NULL }},
        /* Vertex array object */
        { LUA_GL_GenVertexArrays, { "glGenVertexArrays", NULL }},
        { LUA_GL_DeleteVertexArrays, { "glDeleteVertexArrays", NULL }},
        { LUA_GL_BindVertexArray, { "glBindVertexArray", NULL }},
        { 0, { NULL }}
    };

    s_lua_load_procs((proc_t*)glBaseProcs, LUA_PROC_OVERRIDE);

    gl()->extensions |= LUA_HAS_VAO * (GET_GL(GenVertexArrays) != NULL);
    gl()->extensions |= LUA_HAS_VBO * (GET_GL(GenBuffers) != NULL);
    gl()->extensions |= LUA_HAS_SHADER * (GET_GL(CreateShader) != NULL);
#if 0
    if (gl()->extensions & LUA_HAS_VAO) {
        s_lua_bind_vao = s_lua_bind_vao3;
    }
    else if (gl()->extensions & LUA_HAS_VBO) {
        s_lua_bind_vao = s_lua_bind_vao2;
    }
    else {
        s_lua_bind_vao = s_lua_bind_vao1;
    }
#endif

    if (gl()->extensions & LUA_HAS_SHADER) {
        fprintf(stderr, "Using shader\n");
#if 0
        GET_GL(MatrixMode) = s_lua_matrix_mode;
        GET_GL(PushMatrix) = s_lua_push_matrix;
        GET_GL(PopMatrix) = s_lua_pop_matrix;
        GET_GL(LoadIdentity) = s_lua_load_identity;

        GET_GL(LoadMatrixf) = s_lua_load_matrixf;
        GET_GL(LoadTransposeMatrixf) = s_lua_load_transpose_matrixf;
        GET_GL(MultMatrixf) = s_lua_mult_matrixf;
        GET_GL(MultTransposeMatrixf) = s_lua_mult_transpose_matrixf;
        GET_GL(Translatef) = s_lua_translatef;
        GET_GL(Scalef) = s_lua_scalef;
        GET_GL(Rotatef) = s_lua_rotatef;
        GET_GL(Orthof) = s_lua_orthof;
        GET_GL(Frustumf) = s_lua_frustumf;

        GET_GL(LoadMatrixd) = s_lua_load_matrixd;
        GET_GL(LoadTransposeMatrixd) = s_lua_load_transpose_matrixd;
        GET_GL(MultMatrixd) = s_lua_mult_matrixd;
        GET_GL(MultTransposeMatrixd) = s_lua_mult_transpose_matrixd;
        GET_GL(Translated) = s_lua_translated;
        GET_GL(Scaled) = s_lua_scaled;
        GET_GL(Rotated) = s_lua_rotated;

        GET_GL(Ortho) = s_lua_orthod;
        GET_GL(Frustum) = s_lua_frustumd;
#endif
    }
}

void* s_get_proc(const char* name) {
    void* sym = NULL;
    if (s_glsym == NULL) return sym;
#if !defined(__APPLE__) && !defined(__HAIKU__)
    if (s_proc_address != NULL) {
        sym = s_proc_address(name);
    }
#endif
    if (sym == NULL) {
#if defined(_WIN32) || defined(__CYGWIN__)
        sym = (void*)GetProcAddress(s_glsym, name);
#else
        sym = (void*)dlsym(s_glsym, name);
#endif
    }
    return sym;
}

char s_lua_load_procs(proc_t* procs, unsigned int flags) {
    proc_t* proc = procs;
    while (proc->names[0]) {
        if (!gl()->procs[proc->tag] || (flags & LUA_PROC_OVERRIDE)) {
            unsigned int i = 0;
            char** names = (char**)proc->names;
            while (names[i] && i < 3) {
                if ((gl()->procs[proc->tag] = s_get_proc(names[i])))
                    break;
                i++;
            }
        }
        else if (flags & LUA_PROC_RET_ON_DUP)
            return 1;

        proc++;
    }
    return 1;
}

#endif /* LUA_GL_IMPLEMENTATION */