#include "poti.h"
#ifndef POTI_NO_GRAPHICS

typedef struct Texture Texture;
typedef struct Font Font;
typedef struct Shader Shader;

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

/*******************
 * Texture         *
 *******************/

static int l_poti_graphics_new_texture(lua_State *L) {
}

static int l_poti_graphics_load_texture(lua_State* L) {
	Texture* tex = (Texture*)lua_newuserdata(L, sizeof(*tex));
	luaL_setmetatable(L, TEXTURE_CLASS);
	tex->fbo = 0;

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

#endif
