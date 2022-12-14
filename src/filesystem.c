#include "poti.h"
#include <unistd.h>
#include <sys/stat.h>
#if !defined(POTI_NO_FILESYSTEM)

struct Filesystem {
    char basepath[1024];
    char packed;
    char fused;
};

struct Filesystem _fs;
extern lua_State* _L;

static void set_basepath(const i8* path);

int poti_init_filesystem(lua_State* L) {
    const i8* basepath = luaL_optstring(L, 1, "./");
    set_basepath(basepath);
    return 0;
}

void set_basepath(const char* path) {
    i32 len = strlen(path);
    strcpy(_fs.basepath, path);
    // strcpy(_basepath, path);
    // i8* c = &(_basepath[len-1]);
    i8* c = &(_fs.basepath[len-1]);
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
    lua_pushstring(L, _fs.basepath);
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
    return 0;
}

static int l_poti_filesystem_basepath(lua_State* L) {
    lua_pushstring(L, _fs.basepath);
    return 1;
}

static int l_poti_filesystem_set_basepath(lua_State* L) {
    const i8* path = luaL_checkstring(L, 1);
    set_basepath(path);
    return 0;
}

static int l_poti_filesystem_resolve(lua_State* L) {
    const i8* name = luaL_checkstring(L, 1);
    lua_pushstring(L, _fs.basepath);
    lua_pushstring(L, name);
    lua_concat(L, 2);
    return 1;
}

static int l_poti_filesystem_exists(lua_State* L) {
    struct stat info;
    const i8* path = luaL_checkstring(L, 1);
    lua_pushstring(L, _fs.basepath);
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

static int l_poti_filesystem_write(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	const char* text = luaL_checkstring(L, 2);
    lua_pushstring(L, _fs.basepath);
    lua_pushstring(L, path);
    lua_concat(L, 2);
    const char* _rpath = lua_tostring(L, -1);

	FILE* fp = fopen(_rpath, "w");
    lua_pop(L, 1);
	if (!fp) {
		luaL_error(L, "failed to create file %s\n", path);
		return 1;
	}
	fprintf(fp, "%s", text);
	fclose(fp);
	lua_pushboolean(L, 1);
	return 1;
}

static int l_poti_filesystem_mkdir(lua_State* L) {
	const i8* path = luaL_checkstring(L, 1);
    lua_pushstring(L, _fs.basepath);
    lua_pushstring(L, path);
    lua_concat(L, 2);
    const char* _rpath = lua_tostring(L, -1);
    lua_pop(L, 1);

	mkdir(_rpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	return 0;
}

static int l_poti_filesystem_rmdir(lua_State* L) {
	const i8* path = luaL_checkstring(L, 1);
    lua_pushstring(L, _fs.basepath);
    lua_pushstring(L, path);
    lua_concat(L, 2);
    const char* _rpath = lua_tostring(L, -1);
    lua_pop(L, 1);

	rmdir(_rpath);
	return 0;
}

static int l_poti_filesystem_load(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    lua_pushstring(L, _fs.basepath);
    lua_pushstring(L, path);
    lua_concat(L, 2);
    const char* _rpath = lua_tostring(L, -1);
    lua_pop(L, 1);
    luaL_loadfile(L, _rpath);
    return 1;
}

int luaopen_filesystem(lua_State* L) {
    luaL_Reg reg[] = {
		{"init", l_poti_filesystem_init},
		{"basepath", l_poti_filesystem_basepath},
		{"set_basepath", l_poti_filesystem_set_basepath},
		{"resolve", l_poti_filesystem_resolve},
		{"exists", l_poti_filesystem_exists},
		{"read", l_poti_filesystem_read},
		{"write", l_poti_filesystem_write},
		{"mkdir", l_poti_filesystem_mkdir},
		{"rmdir", l_poti_filesystem_rmdir},
        {"load", l_poti_filesystem_load},
		{NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}

#endif
