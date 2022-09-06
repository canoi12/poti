#include "poti.h"
#ifndef POTI_NO_GRAPHICS

#include "stb_image.h"

#define RENDER() (&_render)
#define VERTEX() (RENDER()->vertex)

extern SDL_Window* _window;

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

    Shader* def_shader;
    Vertex def_vertex;
    Texture def_target;
    Texture* white;

    Texture* tex;
    Texture* target;
    Shader* shader;
    Vertex* vertex;

    void(*init_vertex)(Vertex*, u32);
    void(*bind_vertex)(Vertex*);
    void(*clear_vertex)(Vertex*);

    void(*set_texture)(Texture*);
    void(*set_shader)(Shader*);
    void(*set_target)(Texture*);

    void(*push_vertex)(Vertex*, VertexFormat);
    void(*push_vertices)(Vertex*, u32, VertexFormat*);

    void(*draw_vertex)(Vertex*);
};

static struct Render _render;

int poti_init_graphics(lua_State* L) {
    memset(&s_gl_info, 0, sizeof(struct gl_info_t));
    memset(RENDER(), 0, sizeof(struct Render));

    struct Render* r = &_render;
    r->clear_flags = GL_COLOR_BUFFER_BIT;
    r->color[0] = r->color[1] =
    r->color[2] = r->color[3] = 1.f;
    
    SDL_Window* window = lua_touserdata(L, 1);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_config_reg);
    lua_getfield(L, -1, "gl");
#if defined(__EMSCRIPTEN__)
    i32 profile = SDL_GL_CONTEXT_PROFILE_ES; 
#else
    lua_getfield(L, -1, "es");
    i32 profile = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
    if (!lua_isnil(L, -1) && lua_toboolean(L, -1)) {
	profile = SDL_GL_CONTEXT_PROFILE_ES;
    }
    lua_pop(L, 1);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
    lua_getfield(L, -1, "major");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, luaL_optinteger(L, -1, 3));
    lua_pop(L, 1);
    lua_getfield(L, -1, "minor");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, luaL_optinteger(L, -1, 0));
    lua_pop(L, 1);
    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, ctx);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
	fprintf(stderr, "failed to initialize OpenGL context\n");
	exit(EXIT_FAILURE);
    }
    lua_pop(L, 2);
    
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

    lua_pushlightuserdata(L, ctx);
    return 1;
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
    RENDER()->set_target(target);
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

    for (int i = 0; i < 4; i++)
	p[i] = luaL_checknumber(L, i+1);
    f32* c= RENDER()->color;
    f32 vertices[] = {
	p[0], p[1], c[0], c[1], c[2], c[3], 0.f, 0.f,
	p[2], p[2], c[0], c[1], c[2], c[3], 1.f, 1.f,
    };

    Texture* tex = RENDER()->white;
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(VertexFormat), vertices);
    glDrawArrays(GL_LINES, 0, 1);

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
    RENDER()->push_vertices(VERTEX(), 3*sides, vertices);
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
    RENDER()->push_vertices(VERTEX(), 2*sides, vertices);

    // glBufferSubData(GL_ARRAY_BUFFER, 0, bsize, vertices);
    // glDrawArrays(GL_LINES, 0, sides*2);
}

static int l_poti_graphics_circle(lua_State *L) {

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
    VertexFormat* v = (VertexFormat*)vertices;

    // glBufferSubData(GL_ARRAY_BUFFER, 0, 48 * sizeof(float), vertices);
    // glDrawArrays(GL_TRIANGLES, 0, 6);
    RENDER()->push_vertices(VERTEX(), 6, v);
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
    RENDER()->push_vertices(VERTEX(), 8, v);
}

static int l_poti_graphics_rectangle(lua_State *L) {

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
    VertexFormat* v = (VertexFormat*)vertices;

    RENDER()->push_vertices(VERTEX(), 3, v);
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
    RENDER()->push_vertices(VERTEX(), 6, v);
}

static int l_poti_graphics_triangle(lua_State *L) {

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

static int l_poti_graphics_push_vertex(lua_State *L) {
    float *c = RENDER()->color;
    VertexFormat vertex = {
	{0.f, 0.f,
	c[0], c[1], c[2], c[3],
	0.f, 0.f}
    };
    vertex.data[0] = luaL_checknumber(L, 1);
    vertex.data[1] = luaL_checknumber(L, 2);
    for (i32 i = 0; i < 6; i++) {
        vertex.data[i+2] = luaL_optnumber(L, 3+i, vertex.data[i+2]);
    }

    RENDER()->push_vertex(VERTEX(), vertex);
    return 0;
}

/*******************
 * Texture         *
 *******************/

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
    u8* img = poti_fs_read_file(path, &fsize);
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

#if 0
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
#endif

static int l_poti_graphics_new_texture(lua_State *L) {
    Texture* tex = (Texture*)lua_newuserdata(L, sizeof(*tex));
    luaL_setmetatable(L, TEXTURE_META);
    tex->fbo = 0;
    // s_texture_from_size(L, tex);
    return 0;
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
    u8* img = poti_fs_read_file(path, &fsize);
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
#else
    return 0;
#endif
}

int luaopen_graphics(lua_State* L) {

    luaL_Reg reg[] = {
	{"set_shader", NULL},
	{"set_target", l_poti_graphics_set_target},
	{"set_color", l_poti_graphics_set_color},
	{"blend_mode", NULL},
	{"clear", l_poti_graphics_clear},
	{"point", l_poti_graphics_point},
	{"line", l_poti_graphics_line},
	{"circle", l_poti_graphics_circle},
	{"rectangle", l_poti_graphics_rectangle},
	{"triangle", l_poti_graphics_triangle},
	{"print", NULL},
	{"draw", NULL},
	{NULL, NULL}
    };
    luaL_newlib(L, reg);
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
    // RENDER()->clear_vertex(VERTEX());
    RENDER()->set_shader(RENDER()->def_shader);
    RENDER()->set_texture(RENDER()->white);
    RENDER()->set_target(&RENDER()->def_target);
    // RENDER()->bind_vertex(VERTEX());
    return 0;
}

int poti_graphics_end(lua_State* L) {
    glDisable(GL_SCISSOR_TEST);
    RENDER()->draw_vertex(VERTEX());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return 0;
}

#endif
