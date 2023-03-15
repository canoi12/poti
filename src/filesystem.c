#include "poti.h"
#if !WIN32
#include <dirent.h>
#include <unistd.h>
#endif
#include <sys/stat.h>
#if !defined(POTI_NO_FILESYSTEM)

enum {
    FILE_NODE = 0,
    DIR_NODE
};

#if !WIN32
struct File {
    char type;
    int offset, size;
    union {
        FILE* fp;
        DIR* dir;
    };
};
#endif

typedef struct FileHeader FileHeader;
struct FileHeader {
    char type;
    unsigned int size;
    unsigned int mode;
    int uid;
    int gid;
    int checksum;
    unsigned int mtime;
    char name[100];
    char linkname[100];
    char uname[32];
    char gname[32];
};

typedef struct Node Node;
struct Node {
    char type;
    int offset;
    struct FileHeader header;
    Node* child;
    Node* next;
};

struct Filesystem {
    char basepath[1024];
    char packed;
    char fused;
    Node* root;
};

struct Filesystem _fs;
extern lua_State* _L;

static void set_basepath(const char* path);

int poti_init_filesystem(lua_State* L) {
    memset(&_fs, 0, sizeof(_fs));
    const char* basepath = luaL_optstring(L, 1, "./");
    set_basepath(basepath);
    return 0;
}

void set_basepath(const char* path) {
    int len = strlen(path);
    strcpy(_fs.basepath, path);
    // strcpy(_basepath, path);
    // char* c = &(_basepath[len-1]);
    char* c = &(_fs.basepath[len-1]);
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

const char* poti_fs_read_file(const char* filename, size_t* out_size) {
#if 1
    lua_State* L = _L;
    lua_pushstring(L, _fs.basepath);
    lua_pushstring(L, filename);
    lua_concat(L, 2);
    char* data = NULL;
    const char* res_path = lua_tostring(L, -1);
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
    const char* path = luaL_checkstring(L, 1);
    set_basepath(path);
    return 0;
}

static int l_poti_filesystem_get_basepath(lua_State* L) {
    lua_pushstring(L, _fs.basepath);
    return 1;
}

static int l_poti_filesystem_set_basepath(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    set_basepath(path);
    return 0;
}

static int l_poti_filesystem_resolve(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    lua_pushstring(L, _fs.basepath);
    lua_pushstring(L, name);
    lua_concat(L, 2);
    return 1;
}

static int l_poti_filesystem_exists(lua_State* L) {
    struct stat info;
    const char* path = luaL_checkstring(L, 1);
    lua_pushstring(L, _fs.basepath);
    lua_pushstring(L, path);
    lua_concat(L, 2);
    const char* rpath = lua_tostring(L, -1);
    int exists = 1;
    if (stat(rpath, &info) == -1) exists = 0;
    lua_pop(L, 1);
    lua_pushboolean(L, exists);
    return 1;
}

static int l_poti_filesystem_read(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    size_t out_size;
    char* data = (char*)poti_fs_read_file(path, &out_size);
    lua_pushstring(L, (const char*)data);
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
	const char* path = luaL_checkstring(L, 1);
    lua_pushstring(L, _fs.basepath);
    lua_pushstring(L, path);
    lua_concat(L, 2);
    const char* _rpath = lua_tostring(L, -1);
    lua_pop(L, 1);

#ifdef _WIN32
    mkdir(_rpath);
#else
    mkdir(_rpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
    return 0;
}

static int l_poti_filesystem_rmdir(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
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

static int l_poti_filesystem_is_fused(lua_State* L) {
    lua_pushboolean(L, _fs.fused);
    return 1;
}

static int l_poti_filesystem_is_packed(lua_State* L) {
    lua_pushboolean(L, _fs.packed);
    return 1;
}

int luaopen_filesystem(lua_State* L) {
    luaL_Reg reg[] = {
		{"init", l_poti_filesystem_init},
		{"get_basepath", l_poti_filesystem_get_basepath},
		{"set_basepath", l_poti_filesystem_set_basepath},
		{"resolve", l_poti_filesystem_resolve},
		{"exists", l_poti_filesystem_exists},
		{"read", l_poti_filesystem_read},
		{"write", l_poti_filesystem_write},
		{"mkdir", l_poti_filesystem_mkdir},
		{"rmdir", l_poti_filesystem_rmdir},
        {"load", l_poti_filesystem_load},
        {"is_fused", l_poti_filesystem_is_fused},
        {"is_packed", l_poti_filesystem_is_packed},
        {"mount", NULL},
		{NULL, NULL}
    };
    luaL_newlib(L, reg);
    return 1;
}

#endif
