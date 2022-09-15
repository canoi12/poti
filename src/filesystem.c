#include "poti.h"
#include <sys/stat.h>
#if !defined(POTI_NO_FILESYSTEM)

char _basepath[1024] = "./";
extern lua_State* _L;

static void set_basepath(const i8* path);

int poti_init_filesystem(lua_State* L) {
    const i8* basepath = luaL_checkstring(L, 1);
    if (basepath) {
	set_basepath(basepath);
    }
    return 0;
}

void set_basepath(const char* path) {
    i32 len = strlen(path);
    strcpy(_basepath, path);
    i8* c = &(_basepath[len-1]);
    if (*c != '\\' && *c != '/') {
	c++;
#ifdef _WIN32
    c[0] = '\\';
#else
    c[0] = '/';
#endif
	c[1] = '\0';
    }
}

const i8* poti_fs_read_file(const i8* filename, size_t* out_size) {
#if 1
    lua_State* L = _L;
    lua_pushstring(L, _basepath);
    lua_pushstring(L, filename);
    lua_concat(L, 2);
    i8* data = NULL;
    const i8* res_path = lua_tostring(L, -1);
    lua_pop(L, 1);
    FILE* fp = fopen(res_path, "rb");
    if (!fp) return data;
    size_t size = 0;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = malloc(size);
    fread(data, 1, size, fp);
    fclose(fp);
    if (out_size) *out_size = size;
#endif
    return data;
}

static int l_poti_filesystem_init(lua_State* L) {
    const i8* path = luaL_checkstring(L, 1);
    set_basepath(path);
#if 0
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_context_reg);
    lua_setfield(L, -1, "basepath");
    lua_pop(L, 1);
#endif
    return 0;
}

static int l_poti_filesystem_basepath(lua_State* L) {
    lua_pushstring(L, _basepath);
    return 1;
}

static int l_poti_filesystem_set_basepath(lua_State* L) {
    const i8* path = luaL_checkstring(L, 1);
    set_basepath(path);
    return 0;
}

static int l_poti_filesystem_resolve(lua_State* L) {
    const i8* name = luaL_checkstring(L, 1);
    lua_pushstring(L, _basepath);
    lua_pushstring(L, 1);
    lua_concat(L, 2);
    return 1;
}

static int l_poti_filesystem_exists(lua_State* L) {
    struct stat info;
    const i8* path = luaL_checkstring(L, 1);
    lua_pushstring(L, _basepath);
    lua_pushstring(L, path);
    lua_concat(L, 2);
    const i8* rpath = lua_tostring(L, -1);
    i32 exists = 1;
    if (stat(rpath, &info) == -1) exists = 0;
    lua_pop(L, 1);
    lua_pushboolean(L, exists);
    return 1;
}

static int l_poti_filesystem_read(lua_State* L) {
    const i8* path = luaL_checkstring(L, 1);
    size_t out_size;
    i8* data = (i8*)poti_fs_read_file(path, &out_size);
    lua_pushstring(L, (const i8*)data);
    lua_pushinteger(L, out_size);
    free(data);
    return 2;
}

int luaopen_filesystem(lua_State* L) {
    luaL_Reg reg[] = {
	{"init", l_poti_filesystem_init},
	{"basepath", l_poti_filesystem_basepath},
	{"set_basepath", l_poti_filesystem_set_basepath},
	{"resolve", l_poti_filesystem_resolve},
	{"exists", l_poti_filesystem_exists},
	{"read", l_poti_filesystem_read},
	{"write", NULL},
	{"mkdir", NULL},
	{"rmdir", NULL},
	{NULL, NULL}
    };

    luaL_newlib(L, reg);

    return 1;
}

#endif
