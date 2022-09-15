#include "poti.h"
#if !defined(POTI_NO_GRAPHICS)

#include "stb_image.h"
#include "stb_truetype.h"
#include "linmath.h"

#define RENDER() (&_render)
#define VERTEX() (RENDER()->vertex)

extern SDL_Window* _window;
extern SDL_GLContext _gl_ctx;
extern lua_State* _L;

#ifndef NO_EMBED
extern const char _embed_font_ttf[];
extern const long _embed_font_ttf_size;

extern const char _embed_shader_factory_lua[];
#else
static int _shader_factory_lua;
#endif

typedef struct Vertex Vertex;

struct gl_info_t {
    u8 major, minor;
    u16 glsl;
    u16 es; 
};

static struct gl_info_t s_gl_info;

struct Texture {
    u32 fbo;
    u32 handle;
    i32 width, height;
    u32 filter[2];
    u32 wrap[2];
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

struct Shader {
    u32 handle;
    u32 u_world, u_modelview;
};

typedef union {
    struct {
	f32 x, y;
	f32 r, g, b, a;
	f32 u, v;
    };
    f32 data[8];
} VertexFormat;

struct Vertex {
    u32 vao;
    u32 vbo, ibo;
    u32 offset, size;
    void* data;
};

struct Render {
    u32 clear_flags;
    u32 draw_calls;
    i8 draw_mode;
    f32 color[4];

    mat4x4 world, modelview;

    Shader* def_shader;
    Vertex def_vertex;
    Texture def_target;
    Texture* white;
    Font* def_font;

    Texture* tex;
    Texture* target;
    Shader* shader;
    Vertex* vertex;
    Font* font;
};

static struct Render _render;

static int l_perspective_func(lua_State*L);

static u8* utf8_codepoint(u8* p, i32* codepoint);
static i32 init_font(Font* font, const void* data, size_t buf_size, i32 font_size);

static void init_vertex(Vertex*, u32);
static void bind_vertex(Vertex*);
static void clear_vertex(Vertex*);

static void set_texture(Texture*);
static void set_shader(Shader*);
static void set_target(Texture*);
static void set_draw_mode(u8 draw_mode);

static void push_vertex(Vertex*, VertexFormat);
static void push_vertices(Vertex*, u32, VertexFormat*);

static void draw_vertex(Vertex*);

static int l_poti_graphics_new_texture(lua_State*);
static int l_poti_graphics_new_shader(lua_State*);
static int l_poti_graphics_new_font(lua_State* L);

#if 0
const i8* _vert_source =
"#version 140\n"
"uniform mat4 u_World;\n"
"uniform mat4 u_ModelView;\n"
"in vec2 a_Position;\n"
"in vec4 a_Color;\n"
"in vec2 a_TexCoord;\n"
"out vec4 v_Color;\n"
"out vec2 v_TexCoord;\n"
"void main() {\n"
"   v_Color = a_Color;\n"
"   v_TexCoord = a_TexCoord;\n"
"   gl_Position = u_World * u_ModelView * vec4(a_Position.x, a_Position.y, 0, 1);\n"
"}";

const i8* _frag_source =
"#version 140\n"
"uniform sampler2D u_Texture;\n"
"in vec4 v_Color;\n"
"in vec2 v_TexCoord;\n"
"out vec4 o_FragColor;\n"
"void main() {\n"
"   o_FragColor = v_Color * texture(u_Texture, v_TexCoord);\n"
"}";
#else
#endif

static int l_poti_graphics_init(lua_State* L) {
    memset(&s_gl_info, 0, sizeof(struct gl_info_t));
    memset(RENDER(), 0, sizeof(struct Render));

    struct Render* r = &_render;
    r->clear_flags = GL_COLOR_BUFFER_BIT;
    r->color[0] = r->color[1] =
    r->color[2] = r->color[3] = 1.f;
    r->draw_mode = POTI_FILL;

    lua_pushcfunction(L, l_perspective_func);
    lua_rawsetp(L, LUA_REGISTRYINDEX, l_perspective_func);

    SDL_Window* window = lua_touserdata(L, 2);
    lua_getfield(L, 1, "gl");
    i32 profile, major, minor;
#if defined(__EMSCRIPTEN__)
    profile = SDL_GL_CONTEXT_PROFILE_ES; 
    major = 2;
    minor = 0;
#else
    profile = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
    lua_getfield(L, -1, "es");
    if (!lua_isnil(L, -1) && lua_toboolean(L, -1)) {
	profile = SDL_GL_CONTEXT_PROFILE_ES;
    }
    lua_pop(L, 1);
    lua_getfield(L, -1, "major");
    major = luaL_optinteger(L, -1, 3);
    lua_pop(L, 1);
    lua_getfield(L, -1, "minor");
    minor = luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, ctx);
#if !defined(__EMSCRIPTEN__)
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
	fprintf(stderr, "failed to initialize OpenGL context\n");
	exit(EXIT_FAILURE);
    }
#endif
    lua_pop(L, 1);

    const u8 *version = glGetString(GL_VERSION);
    const u8 *glsl = glGetString(GL_SHADING_LANGUAGE_VERSION);

    fprintf(stderr, "%s // %s\n", version, glsl);

    const i8 *prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL,
    };
    struct gl_info_t *info = &s_gl_info;
    info->es = 0;
    i8* ver = (i8*)version;
    for (u32 i = 0; prefixes[i] != NULL; i++) {
	if (strncmp(ver, prefixes[i], strlen(prefixes[i])) == 0) {
	    ver += strlen(prefixes[i]);
	    info->es = 1;
	    break;
	}
    }
    info->major = ver[0] - '0';
    info->minor = ver[2] - '0';
    if (info->es) {
	info->glsl = 100;
	if (info->major == 3) {
	    info->glsl = info->major * 100 + info->minor * 10;
	}
    } else {
	float fglsl = atof((const i8*)glsl);
	info->glsl = 100 * fglsl;
    }

    fprintf(stderr, "GL: { ver: %d.%d, glsl: %d, es: %s }\n", info->major, info->minor, info->glsl, info->es ? "true" : "false");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef NO_EMBED
    if (luaL_dostring(L, _embed_shader_factory_lua) != LUA_OK) {
	const i8* error_buf = lua_tostring(L, -1);
	fprintf(stderr, "Failed to init shader factory: %s\n", error_buf);
	exit(EXIT_FAILURE);
    }
#else
    if (luaL_dofile(L, "embed/shader_factory.lua") != LUA_OK) {
	const i8* error_buf = lua_tostring(L, -1);
	fprintf(stderr, "Failed to init shader factory: %s\n", error_buf);
	exit(EXIT_FAILURE);
    }
#endif
    lua_pushinteger(L, info->glsl);
    lua_pushboolean(L, info->es);
    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
	const i8* error_buf = lua_tostring(L, -1);
	fprintf(stderr, "Failed to setup shader factory: %s\n", error_buf);
	exit(EXIT_FAILURE);
    }
#ifndef NO_EMBED
    lua_rawsetp(L, LUA_REGISTRYINDEX, _embed_shader_factory_lua);
#else
    lua_rawsetp(L, LUA_REGISTRYINDEX, &_shader_factory_lua);
#endif

    Texture* target = &(RENDER()->def_target);
    target->fbo = 0;
    target->handle = 0;
    i32 ww, hh;
    SDL_GetWindowSize(_window, &ww, &hh);
    target->width = ww;
    target->height = hh;
    glViewport(0, 0, ww, hh);
    mat4x4_ortho(RENDER()->world, 0, ww, hh, 0, -1, 1);
    mat4x4_identity(RENDER()->modelview);

    RENDER()->target = target;

    RENDER()->vertex = &(RENDER()->def_vertex);
    init_vertex(VERTEX(), 2000);

    lua_newtable(L);
    lua_pushvalue(L, -1);
    /* register context table */
    lua_rawsetp(L, LUA_REGISTRYINDEX, RENDER());
    /* white texture */
    lua_pushcfunction(L, l_poti_graphics_new_texture);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 1);
    lua_pushnil(L);
    u8 pixels[] = {255, 255, 255, 255};
    lua_pushlightuserdata(L, pixels);
    lua_pcall(L, 4, 1, 0);
    RENDER()->tex = NULL;
    RENDER()->white = lua_touserdata(L, -1);
    set_texture(RENDER()->white);
    lua_setfield(L, -2, "white");
    /* default shader */
    lua_pushcfunction(L, l_poti_graphics_new_shader);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pcall(L, 2, 1, 0);

    RENDER()->shader = NULL;
    RENDER()->def_shader = lua_touserdata(L, -1);
    set_shader(RENDER()->def_shader);
    lua_setfield(L, -2, "shader");
    /* default font */
    lua_pushcfunction(L, l_poti_graphics_new_font);
#ifndef NO_EMBED
    lua_pushlightuserdata(L, (void*)_embed_font_ttf);
    lua_pushinteger(L, _embed_font_ttf_size);
#else
    void* data;
    size_t fsize;
    data = (void*)poti_fs_read_file("embed/font.ttf", &fsize);
    lua_pushlightuserdata(L, data);
    lua_pushinteger(L, fsize);
#endif
    lua_pushinteger(L, 16);
    lua_pcall(L, 3, 1, 0);
    RENDER()->def_font = lua_touserdata(L, -1);
    RENDER()->font = RENDER()->def_font;
    lua_setfield(L, -2, "font");

    /* pop context table */
    lua_pop(L, 1);

    lua_pushlightuserdata(L, ctx);
    return 1;
}

static int l_poti_graphics_deinit(lua_State* L) {
    SDL_GL_DeleteContext(_gl_ctx);
    return 0;
}

static int l_poti_graphics_size(lua_State* L) {
    i32 view[4];
    glGetIntegerv(GL_VIEWPORT, view);
    lua_pushinteger(L, view[2]);
    lua_pushinteger(L, view[3]);
    return 2;
}

/***********************
 * Draw
 ***********************/

static int l_poti_graphics_clear(lua_State* L) {
    f32 color[4] = {0.f, 0.f, 0.f, 1.f};
    i32 args = lua_gettop(L);
    for (i32 i = 0; i < args; i++)
	color[i] = lua_tonumber(L, i+1) / 255.f;
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(RENDER()->clear_flags);
    return 0;
}

static int l_poti_graphics_set_color(lua_State* L) {
    i32 args = lua_gettop(L);
    for (i32 i = 0; i < args; i++)
		RENDER()->color[i] = luaL_checknumber(L, i+1) / 255.f;

    return 0;
}

static int l_poti_graphics_set_target(lua_State* L) {
    Texture* target = luaL_testudata(L, 1, TEXTURE_META);
    target = target ? target : &RENDER()->def_target;
    set_target(target);
    return 0;
}

static int l_poti_graphics_set_shader(lua_State* L) {
    Shader* shader = luaL_testudata(L, 1, SHADER_META);
    shader = shader ? shader : RENDER()->def_shader;
    set_shader(shader);
    return 0;
}

static int l_poti_graphics_set_draw(lua_State* L) {
    const i8* s_mode = luaL_checkstring(L, 1);
    u8 mode = POTI_LINE;
    if (!strcmp(s_mode, "line")) mode = POTI_LINE;
    else if (!strcmp(s_mode, "fill")) mode = POTI_FILL;
    else {
        luaL_error(L, "invalid draw mode: %s", s_mode);
        return 1;
    }
    set_draw_mode(mode);
    return 0;
}

static int l_poti_graphics_clear_buffer(lua_State* L) {
    clear_vertex(VERTEX());
    return 0;
}

static int l_poti_graphics_bind_buffer(lua_State* L) {
    bind_vertex(VERTEX());
    return 0;
}

static int l_poti_graphics_draw_buffer(lua_State* L) {
    draw_vertex(VERTEX());
    return 0;
}

static int l_poti_graphics_point(lua_State* L) {
    f32 x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);
    f32 *c = RENDER()->color;
    
    f32 vertices[] = {
	x, y, c[0], c[1], c[2], c[3], 0.f, 0.f
    };
    Texture* tex = RENDER()->white;
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VertexFormat), vertices);
    glDrawArrays(GL_POINTS, 0, 1);
    return 0;
}

static int l_poti_graphics_line(lua_State* L) {
    f32 p[4];
    set_texture(RENDER()->white);

    for (int i = 0; i < 4; i++)
	p[i] = luaL_checknumber(L, i+1);
    f32* c= RENDER()->color;
    f32 vertices[] = {
	p[0], p[1], c[0], c[1], c[2], c[3], 0.f, 0.f,
	p[2], p[3], c[0], c[1], c[2], c[3], 1.f, 1.f,
    };
	push_vertices(VERTEX(), 2, (VertexFormat*)vertices);

	/*
    Texture* tex = RENDER()->white;
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(VertexFormat), vertices);
    glDrawArrays(GL_LINES, 0, 1);
	*/

    return 0;
}

typedef void(*DrawCircle)(f32, f32, f32, i32);
static void push_filled_circle(float xc, float yc, float radius, int segments) {
    float sides = segments >= 4 ? segments : 4;
    int bsize = (3*sides);
    VertexFormat vertices[bsize];
    // float vertices[bsize];
    double pi2 = 2.0 * M_PI;

    // float *v = vertices;
    VertexFormat* v = vertices;
    for (int i = 0; i < sides; i++) {

	v->x = xc;
	v->y = yc;
	v->r = RENDER()->color[0];
	v->g = RENDER()->color[1];
	v->b = RENDER()->color[2];
	v->a = RENDER()->color[3];
	v->u = 0.f;
	v->v = 0.f;
	v++;

        float tetha = (i * pi2) / sides;

        v->x = xc + (cosf(tetha) * radius);
        v->y = yc + (sinf(tetha) * radius);
	v->r = RENDER()->color[0];
	v->g = RENDER()->color[1];
	v->b = RENDER()->color[2];
	v->a = RENDER()->color[3];
	v->u = 0.f;
	v->v = 0.f;
        v++;

        tetha = ((i+1) * pi2) / sides;

        v->x = xc + (cosf(tetha) * radius);
        v->y = yc + (sinf(tetha) * radius);
	v->r = RENDER()->color[0];
	v->g = RENDER()->color[1];
	v->b = RENDER()->color[2];
	v->a = RENDER()->color[3];
	v->u = 0.f;
	v->v = 0.f;
        v++;
    }

    // glBufferSubData(GL_ARRAY_BUFFER, 0, bsize, vertices);
    // glDrawArrays(GL_TRIANGLES, 0, 3*sides);
    push_vertices(VERTEX(), 3*sides, vertices);
}
static void push_lined_circle(float xc, float yc, float radius, int segments) {
    float sides = segments >= 4 ? segments : 4;
    int bsize = (2*sides);
    // float vertices[bsize];
    VertexFormat vertices[bsize];
    double pi2 = 2.0 * M_PI;

    VertexFormat* v = vertices;
    for (int i = 0; i < sides; i++) {
        float tetha = (i * pi2) / sides;

        v->x = xc + (cosf(tetha) * radius);
        v->y = yc + (sinf(tetha) * radius);
        v->r = RENDER()->color[0];
        v->g = RENDER()->color[1];
        v->b = RENDER()->color[2];
        v->a = RENDER()->color[3];
        v->u = 0.f;
        v->v = 0.f;
	v++;

        tetha = ((i+1) * pi2) / sides;

        v->x = xc + (cosf(tetha) * radius);
        v->y = yc + (sinf(tetha) * radius);
        v->r = RENDER()->color[0];
        v->g = RENDER()->color[1];
        v->b = RENDER()->color[2];
        v->a = RENDER()->color[3];
        v->u = 0.f;
        v->v = 0.f;
	v++;
    }
    push_vertices(VERTEX(), 2*sides, vertices);

    // glBufferSubData(GL_ARRAY_BUFFER, 0, bsize, vertices);
    // glDrawArrays(GL_LINES, 0, sides*2);
}

static int l_poti_graphics_circle(lua_State *L) {

    float x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);

    float radius = luaL_checknumber(L, 3);
    float segments = luaL_optnumber(L, 4, 16);
    set_texture(RENDER()->white);

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
    VertexFormat* v = (VertexFormat*)vertices;

    // glBufferSubData(GL_ARRAY_BUFFER, 0, 48 * sizeof(float), vertices);
    // glDrawArrays(GL_TRIANGLES, 0, 6);
    push_vertices(VERTEX(), 6, v);
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
    VertexFormat* v = (VertexFormat*)vertices;
    push_vertices(VERTEX(), 8, v);
}

static int l_poti_graphics_rectangle(lua_State *L) {

    SDL_Rect r = {0, 0, 0, 0};

    r.x = luaL_checknumber(L, 1);
    r.y = luaL_checknumber(L, 2);
    r.w = luaL_checknumber(L, 3);
    r.h = luaL_checknumber(L, 4);
    set_texture(RENDER()->white);

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
    VertexFormat* v = (VertexFormat*)vertices;

    push_vertices(VERTEX(), 3, v);
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
    VertexFormat* v = (VertexFormat*)vertices;
    push_vertices(VERTEX(), 6, v);
}

static int l_poti_graphics_triangle(lua_State *L) {

    float points[6];

    for (int i = 0; i < 6; i++) {
        points[i] = luaL_checknumber(L, i+1);
    }

    set_texture(RENDER()->white);
    const DrawTriangle fn[] = {
        push_lined_triangle,
        push_filled_triangle
    };

    fn[(int)RENDER()->draw_mode](points[0], points[1], points[2], points[3], points[4], points[5]);

    return 0;
}

static int l_poti_graphics_push_vertex(lua_State *L) {
    float *c = RENDER()->color;
    VertexFormat vertex = {
		{
		0.f, 0.f,
		c[0], c[1], c[2], c[3],
		0.f, 0.f
		}
    };
	// fprintf(stderr, "color: %f %f %f %f\n", c[0], c[1], c[2], c[3]);
    vertex.data[0] = luaL_checknumber(L, 1);
    vertex.data[1] = luaL_checknumber(L, 2);
	i32 top = lua_gettop(L) - 2;
    for (i32 i = 0; i < top; i++) {
        vertex.data[i+2] = luaL_optnumber(L, 3+i, vertex.data[i+2]);
    }

    push_vertex(VERTEX(), vertex);
    return 0;
}

static void char_rect(Font *font, const i32 c, f32 *x, f32 *y, SDL_Point *out_pos, SDL_Rect *rect, int width) {
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

static int l_poti_graphics_print(lua_State* L) {
    Font* font = RENDER()->font;
    f32 x, y;
    const i8* text = luaL_checkstring(L, 1);
    x = luaL_optnumber(L, 2, 0);
    y = luaL_optnumber(L, 3, 0);

    u8* p = (u8*)text;
    float x0 = 0, y0 = 0;
    float w, h;
    float *c = RENDER()->color;
    w = font->tex->width;
    h = font->tex->height;
    set_texture(font->tex);
    while (*p) {
        int codepoint;
        p = utf8_codepoint(p, &codepoint);
        SDL_Rect src, dest;
        SDL_Point pos;
        char_rect(font, codepoint, &x0, &y0, &pos, &src, 0);
        dest.x = x + pos.x;
        dest.y = y + pos.y;
        dest.w = src.w;
        dest.h = src.h;

        SDL_FRect t;
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
	push_vertices(VERTEX(), 6, (VertexFormat*)letter);
    }
    return 0;
}

static int l_poti_graphics_swap(lua_State* L) {
    return 0;
}

/*******************
 * Matrix          *
 *******************/

static int l_poti_graphics_identity(lua_State* L) {
    mat4x4_identity(RENDER()->modelview);
    return 0;
}

static int l_poti_graphics_translate(lua_State* L) {
    f32 pos[3] = {0.f, 0.f, 0.f};
    i32 top = lua_gettop(L);
    for (i32 i = 0; i < top; i++) {
	pos[i] = luaL_checknumber(L, i+1);
    }
    mat4x4_translate_in_place(RENDER()->modelview, pos[0], pos[1], pos[2]);
    return 0;
}

static int l_poti_graphics_scale(lua_State* L) {
    f32 pos[3] = {0.f, 0.f, 0.f};
    i32 top = lua_gettop(L);
    for (i32 i = 0; i < top; i++) {
	pos[i] = luaL_checknumber(L, i+1);
    }
    mat4x4_scale_aniso(RENDER()->modelview, RENDER()->modelview, pos[0], pos[1], pos[2]);
    return 0;
}

static int l_poti_graphics_rotate(lua_State* L) {
    f32 angle = luaL_checknumber(L, 1);
    mat4x4_rotate_Z(RENDER()->modelview, RENDER()->modelview, angle);
    return 0;
}

/*******************
 * Texture         *
 *******************/

static int s_init_texture(Texture *tex, int target, int w, int h, int format, void *data) {
    glGenTextures(1, &tex->handle);
    i32 curr_tex = 0;
    if (RENDER()->tex) curr_tex = RENDER()->tex->handle;
    glBindTexture(GL_TEXTURE_2D, tex->handle);

    tex->fbo = 0;
    tex->width = w;
    tex->height = h;
    tex->filter[0] = GL_NEAREST;
    tex->filter[1] = GL_NEAREST;
    tex->wrap[0] = GL_CLAMP_TO_EDGE;
    tex->wrap[1] = GL_CLAMP_TO_EDGE;


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->filter[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex->filter[1]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->wrap[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->wrap[1]);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, GL_FALSE, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, curr_tex);

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

int l_poti_graphics_new_texture(lua_State *L) {
    Texture* tex = (Texture*)lua_newuserdata(L, sizeof(*tex));
    luaL_setmetatable(L, TEXTURE_META);
    tex->fbo = 0;
    i32 w, h;
    w = luaL_checknumber(L, 1);
    h = luaL_checknumber(L, 2);
    i32 top = lua_gettop(L);
    const i8* s_usage = NULL;
    s_usage = luaL_optstring(L, 3, "static");
    void* data = NULL;
    if (top > 3) data = lua_touserdata(L, 4);

    i32 usage = s_textype_from_string(L, s_usage);
    s_init_texture(tex, usage, w, h, GL_RGBA, data);
    // s_texture_from_size(L, tex);
    return 1;
}

static int l_poti_graphics_load_texture(lua_State* L) {
#if !defined(POTI_NO_FILESYSTEM)
    Texture* tex = (Texture*)lua_newuserdata(L, sizeof(*tex));
    luaL_setmetatable(L, TEXTURE_META);
    tex->fbo = 0;

    size_t size;
    const char *path = luaL_checklstring(L, 1, &size);

    int w, h, format, req_format;
    req_format = STBI_rgb_alpha;

    size_t fsize;
    u8* img = (u8*)poti_fs_read_file(path, &fsize);
    if (!img) {
	luaL_error(L, "failed to open image: %s", path);
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
#else
    return 0;
#endif
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

static int l_poti_texture__draw(lua_State* L) {
    Texture* tex = luaL_checkudata(L, 1, TEXTURE_META);
    i32 index = 2;
    f32 w, h;

    w = tex->width;
    h = tex->height;
    set_texture(tex);

    SDL_Rect s = {0, 0, w, h};
    get_rect_from_table(L, index++, &s);

    f32 uv[4];
    uv[0] = (f32)s.x / w;
    uv[1] = (f32)s.y / h;
    uv[2] = (f32)s.w / w;
    uv[3] = (f32)s.h / h;

    SDL_Rect d = { 0, 0, s.w, s.h };
    get_rect_from_table(L, index++, &d);

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
        get_point_from_table(L, index++, &origin);

        int flip = SDL_FLIP_NONE;
        get_flip_from_table(L, index++, &flip);

        // SDL_RenderCopyEx(poti()->render, *tex, &s, &d, angle, &origin, flip);
    } else {
        // SDL_RenderCopy(poti()->render, *tex, &s, &d);
    }
    push_vertices(VERTEX(), 6, (VertexFormat*)vertices);

    return 0;
}

static int luaopen_texture(lua_State* L) {
    luaL_Reg reg[] = {
        {"draw", l_poti_texture__draw},
#if 0
        {"filter", NULL},
        {"wrap", NULL},
        {"width", l_poti_texture__width},
        {"height", l_poti_texture__height},
        {"size", l_poti_texture__size},
        {"__gc", l_poti_texture__gc},
#endif
        {NULL, NULL}
    };

    luaL_newmetatable(L, TEXTURE_META);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    return 1;
}

/*******************
 * Font            *
 *******************/

int l_poti_graphics_new_font(lua_State* L) {
    void* data = lua_touserdata(L, 1);
    i32 data_size = luaL_checkinteger(L, 2);
    i32 size = luaL_checkinteger(L, 3);

    Font* font = lua_newuserdata(L, sizeof(*font));
    luaL_setmetatable(L, FONT_META);
    init_font(font, data, data_size, size);

    return 1;
}

static int l_poti_graphics_load_font(lua_State* L) {
    const i8* path = luaL_checkstring(L, 1);
    i32 size = luaL_optnumber(L, 2, 16);
    Font* font = lua_newuserdata(L, sizeof(*font));
    luaL_setmetatable(L, FONT_META);

    size_t buf_size;
    i8* font_data = (i8*)poti_fs_read_file(path, &buf_size);
    init_font(font, font_data, buf_size, size);
    free(font_data);
    return 1;
}

/*******************
 * Shader          *
 *******************/

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

int l_poti_graphics_new_shader(lua_State* L) {
#ifndef NO_EMBED
    lua_rawgetp(L, LUA_REGISTRYINDEX, _embed_shader_factory_lua); 
#else
    lua_rawgetp(L, LUA_REGISTRYINDEX, &_shader_factory_lua); 
#endif
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    if (lua_pcall(L, 2, 2, 0) != LUA_OK) {
	luaL_error(L, "failed to make glsl string\n");
	return 1;
    }

    const i8* vert_src = lua_tostring(L, -1);
    const i8* frag_src = lua_tostring(L, -2);
    lua_pop(L, 2);

    Shader* shader = lua_newuserdata(L, sizeof(*shader));
    luaL_setmetatable(L, SHADER_META);

    i32 vert, frag;
    vert = s_compile_shader(vert_src, GL_VERTEX_SHADER);
    frag = s_compile_shader(frag_src, GL_FRAGMENT_SHADER);

    shader->handle = s_load_program(vert, frag);
    shader->u_world = glGetUniformLocation(shader->handle, "u_World");
    shader->u_modelview = glGetUniformLocation(shader->handle, "u_ModelView");
    glDeleteShader(vert);
    glDeleteShader(frag);
    return 1;
}

static int l_poti_shader__location(lua_State *L) {
    Shader *shader = luaL_checkudata(L, 1, SHADER_META);
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
    Shader *shader = luaL_checkudata(L, 1, SHADER_META);
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
    Shader *s = luaL_checkudata(L, 1, SHADER_META);
    glDeleteProgram(s->handle);
    return 0;
}

static int luaopen_shader(lua_State* L) {
    luaL_Reg reg[] = {
        {"location", l_poti_shader__location},
        {"send", l_poti_shader__send},
        {"__gc", l_poti_shader__gc},
        {NULL, NULL}
    };

    luaL_newmetatable(L, SHADER_META);
    luaL_setfuncs(L, reg, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    return 1;
}

int luaopen_graphics(lua_State* L) {

    luaL_Reg reg[] = {
	{"init", l_poti_graphics_init},
	{"deinit", l_poti_graphics_deinit},
	{"size", l_poti_graphics_size},
	/* texture */
	{"new_texture", l_poti_graphics_new_texture},
	{"load_texture", l_poti_graphics_load_texture},
	{"set_target", l_poti_graphics_set_target},
	/* font */
	{"load_font", l_poti_graphics_load_font},
	{"set_font", NULL},
	/* shader */
	{"new_shader", l_poti_graphics_new_shader},
	{"set_shader", l_poti_graphics_set_shader},
	/* transform */
	{"identity", l_poti_graphics_identity},
	{"translate", l_poti_graphics_translate},
	{"scale", l_poti_graphics_scale},
	{"rotate", l_poti_graphics_rotate},
	/* draw */
	{"blend_mode", NULL},
	{"clear_buffer", l_poti_graphics_clear_buffer},
	{"bind_buffer", l_poti_graphics_bind_buffer},
	{"draw_buffer", l_poti_graphics_draw_buffer},
	{"clear", l_poti_graphics_clear},
	{"set_color", l_poti_graphics_set_color},
    {"set_draw", l_poti_graphics_set_draw},
	{"point", l_poti_graphics_point},
	{"line", l_poti_graphics_line},
	{"circle", l_poti_graphics_circle},
	{"rectangle", l_poti_graphics_rectangle},
	{"triangle", l_poti_graphics_triangle},
	{"print", l_poti_graphics_print},
	{"push_vertex", l_poti_graphics_push_vertex},
	{"draw", NULL},
	{"swap", l_poti_graphics_swap},
	{NULL, NULL}
    };
    luaL_newlib(L, reg);
    luaopen_texture(L);
    lua_setfield(L, -2, TEXTURE_META);
    luaopen_shader(L);
    lua_setfield(L, -2, SHADER_META);
    return 1;
}

int poti_graphics_begin(lua_State* L) {
    int ww, hh;
    SDL_GetWindowSize(_window, &ww, &hh);
    glViewport(0, 0, ww, hh);
    Texture* target = &RENDER()->def_target;
    target->fbo = 0;
    target->width = ww;
    target->height = hh;

    glEnable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    RENDER()->draw_calls = 0;

    clear_vertex(VERTEX());
    set_shader(RENDER()->def_shader);
    set_texture(RENDER()->white);
    set_target(&RENDER()->def_target);
    bind_vertex(VERTEX());

    return 0;
}

int poti_graphics_end(lua_State* L) {
    glDisable(GL_SCISSOR_TEST);
    draw_vertex(VERTEX());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
#if !defined(__EMSCRIPTEN__)
    glBindVertexArray(0);
#endif
    return 0;
}

/* Internal functions */

int l_perspective_func(lua_State* L) {
    f32 width, height;
    f32 near, far;
    width = luaL_checknumber(L, 1);
    height = luaL_checknumber(L, 2);
    near = luaL_optnumber(L, 3, -1.f);
    far = luaL_optnumber(L, 4, 1.f);

    mat4x4_ortho(RENDER()->world, 0.f, width, height, 0.f, near, far);
    return 0;
}

void init_vertex(Vertex* v, u32 size) {
    glGenBuffers(1, &v->vbo);
#if !defined(__EMSCRIPTEN__)
    glGenVertexArrays(1, &v->vao);
    glBindVertexArray(v->vao);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, v->vbo);

    v->offset = 0;
    v->size = size;
    v->data = malloc(size);
    
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(sizeof(f32) * 2));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void*)(sizeof(f32) * 6));

#if !defined(__EMSCRIPTEN__)
    glBindVertexArray(0);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void bind_vertex(Vertex* v) {
#if !defined(__EMSCRIPTEN__)
    glBindVertexArray(v->vao);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, v->vbo);
}

void clear_vertex(Vertex* v) {
    v->offset = 0;
}

void set_texture(Texture* t) {
    if (RENDER()->tex != t) {
	draw_vertex(VERTEX());
        clear_vertex(VERTEX());
        RENDER()->tex = t;
        glBindTexture(GL_TEXTURE_2D, t->handle);
    }
}

void set_shader(Shader *s) {
    if (RENDER()->shader != s) {
        draw_vertex(VERTEX());
        clear_vertex(VERTEX());
        RENDER()->shader = s;
        glUseProgram(s->handle);
        GLint view[4];
        glGetIntegerv(GL_VIEWPORT, view);

	lua_rawgetp(_L, LUA_REGISTRYINDEX, l_perspective_func);
	lua_pushnumber(_L, view[2]);
	lua_pushnumber(_L, view[3]);
	mat4x4_identity(RENDER()->world);
	if (lua_pcall(_L, 2, 0, 0) != LUA_OK) {
	    fprintf(stderr, "Failed to call perspective matrix function"); 
	}

        glUniformMatrix4fv(s->u_world, 1, GL_FALSE, *(RENDER()->world));
        glUniformMatrix4fv(s->u_modelview, 1, GL_FALSE, *(RENDER()->modelview));
        RENDER()->shader = s;
    }
}

void set_target(Texture *t) {
    if (t->handle != 0 && t->fbo == 0) return;
    if (t->handle == 0 && t->fbo == 0) {
		i32 ww, hh;
		SDL_GetWindowSize(_window, &ww, &hh);
		t->width = ww;
		t->height = hh;
    }
    if (RENDER()->target != t) {
        draw_vertex(VERTEX());
        clear_vertex(VERTEX());
        RENDER()->target = t;
    }
	glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
	glViewport(0, 0, t->width, t->height);

	Shader *s = RENDER()->shader;

	lua_rawgetp(_L, LUA_REGISTRYINDEX, l_perspective_func);
	lua_pushnumber(_L, t->width);
	lua_pushnumber(_L, t->height);
	mat4x4_identity(RENDER()->world);
	if (lua_pcall(_L, 2, 0, 0) != LUA_OK) {
		fprintf(stderr, "Failed to call perspective matrix function"); 
	}

	glUniformMatrix4fv(s->u_world, 1, GL_FALSE, *(RENDER()->world));
	glUniformMatrix4fv(s->u_modelview, 1, GL_FALSE, *(RENDER()->modelview));
}

void set_draw_mode(u8 draw_mode) {
    if (draw_mode != RENDER()->draw_mode) {
	draw_vertex(VERTEX());
	clear_vertex(VERTEX());
	RENDER()->draw_mode = draw_mode;
    }
}

void push_vertex(Vertex* vert, VertexFormat data) {
    u32 size = sizeof(VertexFormat);
    if (vert->offset + size > vert->size) {
        vert->size *= 2;
        vert->data = realloc(vert->data, vert->size);
	glBufferData(GL_ARRAY_BUFFER, vert->size, vert->data, GL_DYNAMIC_DRAW);
    }
    char *vdata = ((char*)vert->data)+vert->offset;
    vert->offset += size;
    memcpy(vdata, &data, size);
}

void push_vertices(Vertex *vert, u32 count, VertexFormat* vertices) {
    u32 size = count * sizeof(VertexFormat);
    if (vert->offset + size > vert->size) {
        vert->size *= 2;
        vert->data = realloc(vert->data, vert->size);
	glBufferData(GL_ARRAY_BUFFER, vert->size, vert->data, GL_DYNAMIC_DRAW);
    }
    char *data = ((char*)vert->data)+vert->offset;
    vert->offset += size;
    memcpy(data, vertices, size);
}

void flush_vertex(Vertex *v) {
    glBufferSubData(GL_ARRAY_BUFFER, 0, v->offset, v->data);
}

void draw_vertex(Vertex *v) {
    if (v->offset <= 0) return;
    flush_vertex(v); 
    u32 gl_modes[] = {
        GL_LINES, GL_TRIANGLES
    };
    RENDER()->draw_calls++;
    glDrawArrays(gl_modes[(i32)RENDER()->draw_mode], 0, v->offset / sizeof(VertexFormat));
}

/* Font */
#define MAX_UNICODE 0x10FFFF
u8* utf8_codepoint(u8* p, i32* codepoint) {
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

i32 init_font(Font* font, const void* data, size_t buf_size, i32 font_size) {
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

#endif
