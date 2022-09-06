#include "poti.h"
#ifndef POTI_NO_AUDIO

#include "miniaudio.h"

#define MAX_AUDIO_BUFFER_CHANNELS 256

#define AUDIO_DEVICE_FORMAT ma_format_f32
#define AUDIO_DEVICE_CHANNELS 2
#define AUDIO_DEVICE_SAMPLE_RATE 44100

#define AUDIO_STREAM 0
#define AUDIO_STATIC 1

#ifndef ma_countof
#define ma_countof(x)               (sizeof(x) / sizeof(x[0]))
#endif

const i8 lr_audio_data;

typedef struct Audio Audio;
typedef struct AudioBuffer AudioBuffer;
typedef struct AudioData AudioData;

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
    u8 playing;
    u8 loop, loaded;
    i64 offset;
    u8 usage;
    u32 size;
};

struct Audio {
    AudioData data;
};

struct AudioSystem {
    ma_context ctx;
    ma_device device;
    ma_mutex lock;

    u8 is_ready;
    AudioBuffer buffers[MAX_AUDIO_BUFFER_CHANNELS];
};

static struct AudioSystem _audio;

static int s_register_audio_data(lua_State* L, u8 usage, const char *path) {
    AudioData *adata = NULL;
	lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_audio_data);
    lua_rawgeti(L, -1, usage);
    lua_remove(L, -2);
    lua_pushstring(L, path);
    lua_gettable(L, -2);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        size_t size;
	void* data = poti_fs_read_file(path, &size);
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

static int l_poti_new_audio(lua_State* L) {
	const char *path = luaL_checkstring(L, 1);
	const char* s_usage = luaL_optstring(L, 2, "stream");

	int usage = AUDIO_STREAM;
	if (!strcmp(s_usage, "static")) usage = AUDIO_STATIC;

	s_register_audio_data(L, usage, path);
	AudioData *a_data = lua_touserdata(L, -1);
	lua_pop(L, 1);
	Audio* audio = lua_newuserdata(L, sizeof(*audio));
	luaL_setmetatable(L, AUDIO_META);
	memcpy(&(audio->data), &a_data, sizeof(AudioData));
	return 1;
}

static int l_poti_audio_volume(lua_State* L) {
	return 0;
}

static int l_poti_audio__play(lua_State* L) {
	Audio* audio = luaL_checkudata(L, 1, AUDIO_META);
	i32 i;
	for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
		if (_audio.buffers[i].loaded) break;
	}

	AudioBuffer* buffer = &_audio.buffers[i];
	ma_decoder_config dec_config = ma_decoder_config_init(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, AUDIO_DEVICE_SAMPLE_RATE);
    AudioData *a_data = &(audio->data);
    buffer->usage = a_data->usage;
    buffer->size = a_data->size;
    ma_result result;
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

    return 0;
}

static int l_poti_audio__stop(lua_State* L) {
	return 0;
}

static int l_poti_audio__pause(lua_State* L) {
	return 1;
}

static int l_poti_audio__playing(lua_State* L) {
	lua_pushboolean(L, 1);
	return 1;
}

static int l_poti_audio__volume(lua_State* L) { return 0; }

static int l_poti_audio__pitch(lua_State* L) { return 0; }

int l_poti_audio__gc(lua_State* L) { return 0; }

int luaopen_audio(lua_State* L) {
#if 0
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
#endif
	luaL_Reg reg[] = {
		{"load_audio", NULL},
		{"volume", NULL},
		{NULL, NULL}
	};

    luaL_newlib(L, reg);
    return 1;
}

#endif
