#include "poti.h"
#ifndef POTI_NO_AUDIO

#include "miniaudio.h"

#define MAX_AUDIO_BUFFER_CHANNELS 256

#define AUDIO_DEVICE_FORMAT ma_format_s16
#define AUDIO_DEVICE_CHANNELS 2
#define AUDIO_DEVICE_SAMPLE_RATE 44100

#define AUDIO_STREAM 0
#define AUDIO_STATIC 1

#ifndef ma_countof
#define ma_countof(x)               (sizeof(x) / sizeof(x[0]))
#endif

const char lr_audio_data;

const char* audio_bank =
"local audio_bank = { ['stream'] = {}, ['static'] = {} }\n"
"local function check(usage, path)\n"
"   if not audio_bank[usage] then error('Invalid audio usage') end\n"
"   return audio_bank[usage][path]\n"
"end\n"
"local function register(usage, path, id, data)\n"
"   audio_bank[usage][path] = data\n"
"   audio_bank[id] = data\n" 
"end\n"
"return register, check, audio_bank";

int l_register_audio_reg;
int l_check_audio_reg;
int l_audio_bank_reg;

static Uint32 s_audio_id = 0;

struct AudioData {
    Uint32 id;
    Uint8 usage;
    Uint32 size;
    Uint8* data;
    Uint8 loop;
    float volume, pitch;
};

struct AudioBuffer {
    Uint16 id;
    ma_decoder decoder;
    Uint8 playing, loaded;
    Sint64 offset;
    AudioData data;
};

struct AudioSystem {
    ma_context ctx;
    ma_device device;
    ma_mutex lock;

    char is_ready;
    AudioBuffer buffers[MAX_AUDIO_BUFFER_CHANNELS];
};

static struct AudioSystem _audio;

static inline void s_audio_callback(ma_device *device, void *output, const void *input, ma_uint32 frameCount);

static int l_poti_audio_init(lua_State* L) {
    if (luaL_dostring(L, audio_bank) != LUA_OK) {
        fprintf(stderr, "Failed to load audio bank function: %s\n", lua_tostring(L, -1));
        exit(EXIT_FAILURE);
    }
    // fprintf(stderr, "%d %d %d\n", lua_type(L, -1), lua_type(L, -2), lua_type(L, -3));
    lua_rawsetp(L, LUA_REGISTRYINDEX, &l_audio_bank_reg);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &l_check_audio_reg);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &l_register_audio_reg);

    ma_context_config ctx_config = ma_context_config_init();
    ma_result result = ma_context_init(NULL, 0, &ctx_config, &(_audio.ctx));
    if (result != MA_SUCCESS) {
		fprintf(stderr, "Failed to init audio context\n");
		exit(EXIT_FAILURE);
    }

    ma_device_config dev_config = ma_device_config_init(ma_device_type_playback);
    dev_config.playback.pDeviceID = NULL;
    dev_config.playback.format = AUDIO_DEVICE_FORMAT;
    dev_config.playback.channels = AUDIO_DEVICE_CHANNELS;
    dev_config.sampleRate = AUDIO_DEVICE_SAMPLE_RATE;
    dev_config.pUserData = &_audio;
    dev_config.dataCallback = s_audio_callback;

    result = ma_device_init(&_audio.ctx, &dev_config, &_audio.device);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to init audio device\n");
        ma_context_uninit(&_audio.ctx);
        exit(EXIT_FAILURE);
    }

    if (ma_device_start(&_audio.device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start audio device\n");
        ma_context_uninit(&_audio.ctx);
        ma_device_uninit(&_audio.device);
        exit(EXIT_FAILURE);
    }

    if (ma_mutex_init(&_audio.ctx, &_audio.lock) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start audio mutex\n");
        ma_device_uninit(&_audio.device);
        ma_context_uninit(&_audio.ctx);
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer *buffer = &_audio.buffers[i];
        memset(buffer, 0, sizeof(*buffer));
        buffer->playing = 0;
        buffer->data.volume = 1.f;
        buffer->data.pitch = 1.f;
        buffer->loaded = 0;
        buffer->data.loop = 0;
    }

    _audio.is_ready = 1;
    return 0;
}

static int l_poti_audio_deinit(lua_State* L) {
    if (_audio.is_ready) {
		ma_mutex_uninit(&(_audio.lock));
		ma_device_uninit(&(_audio.device));
		ma_context_uninit(&(_audio.ctx));
    } else fprintf(stderr, "Audio module could not be close, not initialized\n");
    return 0;
}

static int l_poti_audio_load_audio(lua_State* L) {
#if !defined(POTI_NO_FILESYSTEM)
	const char *path = luaL_checkstring(L, 1);

	const char* s_usage = luaL_optstring(L, 2, "stream");
	const  char* tests[] = { "stream", "static" };
	int check = 0;
    int i;
	for (i = 0; i < 3; i++) {
        if (!strcmp(s_usage, tests[i])) {
            check = 1;
            break;
        }
	}
	if (!check) return luaL_argerror(L, 2, "Invalid audio usage");

	lua_rawgetp(L, LUA_REGISTRYINDEX, &l_check_audio_reg);	
	lua_pushstring(L, s_usage);
	lua_pushstring(L, path);
	if (lua_pcall(L, 2, 1, 0) != 0) {
        fprintf(stderr, "Failed to check audio register\n");
        exit(EXIT_FAILURE);
	}
	char exists = !lua_isnil(L, -1);

	int usage = AUDIO_STREAM;
	if (!strcmp(s_usage, "static")) usage = AUDIO_STATIC;

	if (!exists) {
        lua_pop(L, 1);
        size_t size;
        void* data = (void*)poti_fs_read_file(path, &size);

        AudioData* adata = (AudioData*)lua_newuserdata(L, sizeof(*adata));
        luaL_setmetatable(L, AUDIO_META);
        adata->id = ++s_audio_id;
        adata->usage = usage;
        adata->loop = 0;
        adata->volume = 1.f;
        adata->pitch = 0.f;

        if (usage == AUDIO_STATIC) {
            ma_decoder_config dec_config = ma_decoder_config_init(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, AUDIO_DEVICE_SAMPLE_RATE);
            ma_uint64 frame_count_out;
            void* dec_data;
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
        int top = lua_gettop(L);
        lua_rawgetp(L, LUA_REGISTRYINDEX, &l_register_audio_reg);
        lua_pushstring(L, s_usage);
        lua_pushstring(L, path);
        lua_pushinteger(L, adata->id);
        lua_pushvalue(L, top);
        if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
            luaL_error(L, "Failed to register audio data");
            return 1;
        }
	} 
	// s_register_audio_data(L, usage, path);
	return 1;
#else
	return 0;
#endif
}

static int l_poti_audio_set_volume(lua_State* L) {
    int top = lua_gettop(L);
    if (top > 1) {
        int index = luaL_checkinteger(L, 1) - 1;
        float volume = luaL_checknumber(L, 2);
        AudioBuffer* bf = &(_audio.buffers[index]);
        bf->data.volume = volume;
    } else {
        float volume = luaL_checknumber(L, 1);
        ma_device_set_master_volume(&(_audio.device), volume);
    }
	return 0;
}

static int l_poti_audio_play(lua_State* L) {
    int index = luaL_checknumber(L, 1) - 1;
    if (index < 0 || index >= MAX_AUDIO_BUFFER_CHANNELS) {
        luaL_error(L, "Invalid audio instance");
        return 1;
    }
    AudioBuffer* bf = &(_audio.buffers[index]);
    if (bf->loaded) bf->playing = 1;
    return 0;
}

static int l_poti_audio_pause(lua_State* L) {
    int index = luaL_checknumber(L, 1) - 1;
    if (index < 0 || index >= MAX_AUDIO_BUFFER_CHANNELS) {
        luaL_error(L, "Invalid audio instance");
        return 1;
    }
    AudioBuffer* bf = &(_audio.buffers[index]);
    if (bf->loaded) bf->playing = 0;
    return 0;
}

static int l_poti_audio_stop(lua_State* L) {
    int index = luaL_checknumber(L, 1) - 1;
    if (index < 0 || index >= MAX_AUDIO_BUFFER_CHANNELS) {
        luaL_error(L, "Invalid audio instance");
        return 1;
    }
    AudioBuffer* bf = &(_audio.buffers[index]);
    if (bf->loaded) {
        bf->loaded = 0;
        bf->playing = 0;
    }
    return 0;
}

/*******************
 * AudioData       *
 *******************/

static int l_poti_audio__play(lua_State* L) {
    AudioData* audio = luaL_checkudata(L, 1, AUDIO_META);
    int i;
    for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        if (!_audio.buffers[i].loaded) break;
    }
    if (i == MAX_AUDIO_BUFFER_CHANNELS) {
        luaL_error(L, "Audio buffers limit reached");
		return 1;
    }

    AudioBuffer* buffer = &_audio.buffers[i];
    ma_decoder_config dec_config = ma_decoder_config_init(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, AUDIO_DEVICE_SAMPLE_RATE);
    memcpy(&(buffer->data), audio, sizeof(AudioData));
    ma_result result;
    buffer->offset = 0;
    if (audio->usage == AUDIO_STREAM) {
        result = ma_decoder_init_memory(audio->data, audio->size, &dec_config, &buffer->decoder);
        ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
    } else {
        result = MA_SUCCESS;
    }

    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to load sound %s\n", ma_result_description(result));
        exit(0);
    }

    buffer->loaded = 1;
    buffer->playing = 1;
    buffer->id = audio->id;

    lua_pushinteger(L, i+1);
    return 1;
}

static int l_poti_audio__play_all(lua_State* L) {
    AudioData* ad = luaL_checkudata(L, 1, AUDIO_META);
    int id = ad->id;
    for (int i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer* b = &(_audio.buffers[i]);
        int paused = b->loaded && !b->playing;
        if (b->id == id && paused) {
            b->playing = 1;
        }
    }
    return 0;
}

static int l_poti_audio__stop(lua_State* L) {
    AudioData* ad = luaL_checkudata(L, 1, AUDIO_META);
    int id = ad->id;

    for (int i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer* b = &(_audio.buffers[i]);
        int playing = b->loaded && b->playing;
        if (playing && b->id == id) {
            b->playing = 0;
            b->loaded = 0;
        }
    }
    return 0;
}

static int l_poti_audio__pause(lua_State* L) {
    AudioData* ad = luaL_checkudata(L, 1, AUDIO_META);
    int id = ad->id;
    for (int i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer* b = &(_audio.buffers[i]);
        int playing = b->loaded && b->playing;
        if (playing && b->id == id) {
            b->playing = 0;
        }
    }
	return 1;
}

static int l_poti_audio__playing(lua_State* L) {
    AudioData* ad = luaL_checkudata(L, 1, AUDIO_META);
    int id = ad->id;
    for (int i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer* b = &(_audio.buffers[i]);
        int playing = b->loaded && b->playing;
        if (playing && b->id == id) {
            lua_pushboolean(L, 1);
            return 1;
        }
    }
    lua_pushboolean(L, 0);
	return 1;
}

static int l_poti_audio__volume(lua_State* L) {
    AudioData* ad = luaL_checkudata(L, 1, AUDIO_META);
    float volume = luaL_checknumber(L, 2);
    ad->volume = volume;
    return 0;
}

static int l_poti_audio__pitch(lua_State* L) {
    AudioData* ad = luaL_checkudata(L, 1, AUDIO_META);
    float pitch = luaL_checknumber(L, 2);
    ad->pitch = pitch;
    return 0;
}

int l_poti_audio__gc(lua_State* L) {
    AudioData* ad = luaL_checkudata(L, 1, AUDIO_META);
    int id = ad->id;
    for (int i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer* b = &(_audio.buffers[i]);
        if (b->loaded && b->id == id) b->loaded = 0;
    }
    free(ad->data);
    return 0;
}

static int l_audio_meta(lua_State* L) {
    luaL_Reg meta[] = {
        {"play", l_poti_audio__play},
        {"play_all", l_poti_audio__play_all},
        {"stop", l_poti_audio__stop},
        {"pause", l_poti_audio__pause},
        {"playing", l_poti_audio__playing},
        {"volume", l_poti_audio__volume},
        {"pitch", l_poti_audio__pitch},
        {"__gc", l_poti_audio__gc},
        {NULL, NULL}
    };
	luaL_newmetatable(L, AUDIO_META);
	luaL_setfuncs(L, meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	return 1;
}

/**************
 *  AudioLib  *
 **************/

int luaopen_audio(lua_State* L) {
    luaL_Reg reg[] = {
		{"init", l_poti_audio_init},
		{"deinit", l_poti_audio_deinit},
		{"load_audio", l_poti_audio_load_audio},
		{"set_volume", l_poti_audio_set_volume},
        {"play", l_poti_audio_play},
        {"pause", l_poti_audio_pause},
        {"stop", l_poti_audio_stop},
		{NULL, NULL}
    };
    luaL_newlib(L, reg);
	l_audio_meta(L);
	lua_setfield(L, -2, AUDIO_META);
    return 1;
}

#if 0
int s_register_audio_data(lua_State* L, u8 usage, const char *path) {
    AudioData *adata = NULL;
    lua_rawgetp(L, LUA_REGISTRYINDEX, &l_check_audio_reg);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &lr_audio_data);
    lua_rawgeti(L, -1, usage);
    lua_remove(L, -2);
    lua_getfield(L, -1, path);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        Uint32 size;
	void* data = (void*)poti_fs_read_file(path, (int*)&size);
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
	adata->volume = 1.f;
	adata->pitch = 1.f;
	adata->id = ++s_audio_id;
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
#endif

Uint32 s_read_and_mix_pcm_frames(AudioBuffer* buffer, short* output, Uint32 frames) {
    short temp[4096];
    Uint32 temp_cap_in_frames = ma_countof(temp) / AUDIO_DEVICE_CHANNELS;
    Uint32 total_frames_read = 0;
    ma_decoder* decoder = &buffer->decoder;
    AudioData* data = &(buffer->data);
    float volume = data->volume;
    float size = data->size * ma_get_bytes_per_frame(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS);

    while (total_frames_read < frames) {
        Uint32 sample;
        Uint32 frames_read_this_iteration;
        Uint32 total_frames_remaining = frames - total_frames_read;
        Uint32 frames_to_read_this_iteration = temp_cap_in_frames;
        if (frames_to_read_this_iteration > total_frames_remaining) {
            frames_to_read_this_iteration = total_frames_remaining;
        }

        if (data->usage == AUDIO_STREAM) {
            frames_read_this_iteration = (Uint32)ma_decoder_read_pcm_frames(decoder, temp, frames_to_read_this_iteration);
        }
        else {
            frames_read_this_iteration = frames_to_read_this_iteration;
            Uint32 aux = frames_to_read_this_iteration * ma_get_bytes_per_frame(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS);
            memcpy(temp, data->data + buffer->offset, aux);
            if (buffer->offset > size) frames_read_this_iteration = 0;
            buffer->offset += aux;
        }

        if (frames_read_this_iteration == 0) {
            break;
        }

        for (sample = 0; sample < frames_read_this_iteration * AUDIO_DEVICE_CHANNELS; ++sample) {
            output[total_frames_read * AUDIO_DEVICE_CHANNELS + sample] += temp[sample] * volume;
        }

        total_frames_read += frames_read_this_iteration;

        if (frames_read_this_iteration < frames_to_read_this_iteration) {
            break;
        }
    }
    return total_frames_read;
}

void s_audio_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
    short* out = (short*)output;

    ma_mutex_lock(&(_audio.lock));
    int i;
    for (i = 0; i < MAX_AUDIO_BUFFER_CHANNELS; i++) {
        AudioBuffer* buffer = &(_audio.buffers[i]);
        AudioData* data = &(buffer->data);
        if (buffer->playing && buffer->loaded) {
            Uint32 frames_read = s_read_and_mix_pcm_frames(buffer, out, frame_count);
            if (frames_read < frame_count) {
                if (data->loop) {
                    ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
                    buffer->offset = 0;
                }
                else {
                    buffer->playing = 0;
                }
            }
        }
    }
    ma_mutex_unlock(&(_audio.lock));
    (void)input;
}
#endif
