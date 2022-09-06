#include "poti.h"
#include <sys/stat.h>
#if !defined(POTI_NO_FILESYSTEM)

char _basepath[1024] = "./";

int poti_init_filesystem(lua_State* L) {
    const i8* basepath = luaL_checkstring(L, 1);
    if (basepath) {
	i32 len = strlen(basepath);
	strcpy(_basepath, basepath);
	i8* c = &_basepath[len-2];
	if (*c != '\\' || *c != '/') {
#ifdef _WIN32
	   c[1] = '\\';
#else
	   c[1] = '/';
#endif
	   c[2] = '\0';
	}
    }
    return 0;
}

const i8* poti_fs_read_file(const i8* filename, i32* out_size) {
    return 0;
}

static int l_poti_filesystem_init(lua_State* L) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_context_reg);
    lua_getfield(L, -1, "basepath");
    strcpy(_basepath, lua_tostring(L, -1));
    lua_pop(L, 2);
    return 0;
}

static int l_poti_filesystem_basepath(lua_State* L) {
    lua_pushstring(L, _basepath);
    return 1;
}

static int l_poti_filesystem_set_basepath(lua_State* L) {
    strcpy(_basepath, luaL_checkstring(L, 1));
    return 0;
}

static int l_poti_filesystem_exists(lua_State* L) {
    struct stat info;
    const i8* path = luaL_checkstring(L, 1);
    i32 exists = 1;
    if (stat(path, &info) == -1) exists = 0;
    lua_pushboolean(L, exists);
    return 1;
}

int luaopen_filesystem(lua_State* L) {
    luaL_Reg reg[] = {
	{"init", l_poti_filesystem_init},
	{"basepath", l_poti_filesystem_basepath},
	{"set_basepath", l_poti_filesystem_set_basepath},
	{"resolve", NULL},
	{"exists", l_poti_filesystem_exists},
	{"read", NULL},
	{"write", NULL},
	{"mkdir", NULL},
	{"rmdir", NULL},
	{NULL, NULL}
    };

    luaL_newlib(L, reg);

    return 1;
}

#endif
