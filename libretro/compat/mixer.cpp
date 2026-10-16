#include "SDL_mixer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define STB_VORBIS_HEADER_ONLY
#include "../third_party/stb_vorbis.c"

struct MusicData {
    std::vector<Uint8> compressed;
    stb_vorbis *decoder;
    stb_vorbis_info info;
};

struct Voice {
    Mix_Chunk *chunk;
    Uint64 position;
};

static Voice voices[16];
static MusicData *current_music;
static int music_loops;
static int music_volume = MIX_MAX_VOLUME;
static int effects_volume = MIX_MAX_VOLUME;
static short music_buffer[4096];
static int music_buffer_frames, music_buffer_pos;
static Uint32 music_phase;
static int music_left, music_right;
static void next_music_sample(int &left, int &right);

extern "C" int Mix_OpenAudio(int, Uint16, int, int) { return 0; }
extern "C" void Mix_CloseAudio(void) { current_music = nullptr; std::memset(voices, 0, sizeof voices); }
extern "C" const char *Mix_GetError(void) { return "invalid or unsupported audio data"; }
extern "C" int Mix_VolumeMusic(int volume) {
    int old = music_volume;
    if (volume >= 0) music_volume = std::min(volume, MIX_MAX_VOLUME);
    return old;
}
extern "C" int Mix_Volume(int channel, int volume) {
    (void)channel;
    int old = effects_volume;
    if (volume >= 0) effects_volume = std::min(volume, MIX_MAX_VOLUME);
    return old;
}

static Uint16 le16(const Uint8 *p) { return p[0] | (p[1] << 8); }
static Uint32 le32(const Uint8 *p) { return le16(p) | (Uint32(le16(p + 2)) << 16); }

extern "C" Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *rw, int free_rw) {
    if (!rw) return nullptr;
    const Uint8 *p = rw->hidden.mem.base;
    size_t size = rw->hidden.mem.stop - p;
    const Uint8 *data = nullptr;
    size_t data_size = 0;
    int channels = 0, rate = 0, bits = 0;
    if (size >= 12 && !std::memcmp(p, "RIFF", 4) && !std::memcmp(p + 8, "WAVE", 4)) {
        for (size_t at = 12; at + 8 <= size;) {
            Uint32 length = le32(p + at + 4);
            if (length > size - at - 8) break;
            if (!std::memcmp(p + at, "fmt ", 4) && length >= 16 && le16(p + at + 8) == 1) {
                channels = le16(p + at + 10); rate = le32(p + at + 12); bits = le16(p + at + 22);
            } else if (!std::memcmp(p + at, "data", 4)) { data = p + at + 8; data_size = length; }
            at += 8 + length + (length & 1);
        }
    }
    Mix_Chunk *chunk = nullptr;
    if (data && channels >= 1 && channels <= 2 && rate > 0 && (bits == 8 || bits == 16)) {
        size_t count = data_size / (bits / 8);
        if (count <= 16 * 1024 * 1024) {
            chunk = static_cast<Mix_Chunk *>(std::calloc(1, sizeof(Mix_Chunk)));
            if (chunk) {
                chunk->abuf = static_cast<Uint8 *>(std::malloc(count * sizeof(short)));
                if (!chunk->abuf) { std::free(chunk); chunk = nullptr; }
                else {
                    short *samples = reinterpret_cast<short *>(chunk->abuf);
                    for (size_t i = 0; i < count; ++i) samples[i] = bits == 8 ? short((int(data[i]) - 128) << 8) : short(le16(data + 2 * i));
                    chunk->alen = count * sizeof(short);
                    chunk->freq = rate; chunk->channels = channels; chunk->bits = 16; chunk->allocated = 1;
                    chunk->volume = MIX_MAX_VOLUME;
                }
            }
        }
    }
    if (free_rw) SDL_RWclose(rw);
    return chunk;
}

extern "C" void Mix_FreeChunk(Mix_Chunk *chunk) {
    if (!chunk) return;
    for (Voice &voice : voices) if (voice.chunk == chunk) voice = {nullptr, 0};
    std::free(chunk->abuf);
    std::free(chunk);
}
extern "C" int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int) {
    if (!chunk) return -1;
    if (channel < 0) { for (int i = 0; i < 16; ++i) if (!voices[i].chunk) { channel = i; break; } }
    if (channel < 0 || channel >= 16) channel = 0;
    voices[channel] = {chunk, 0};
    return channel;
}

extern "C" Mix_Music *Mix_LoadMUS_RW(SDL_RWops *rw, int free_rw) {
    if (!rw) return nullptr;
    std::vector<Uint8> compressed(rw->hidden.mem.base, rw->hidden.mem.stop);
    if (free_rw) SDL_RWclose(rw);
    return vvv_load_music(std::move(compressed));
}

Mix_Music *vvv_load_music(std::vector<Uint8> &&compressed) {
    if (compressed.empty()) return nullptr;
    MusicData *data = new MusicData;
    data->compressed = std::move(compressed);
    int error = 0;
    data->decoder = stb_vorbis_open_memory(data->compressed.data(), data->compressed.size(), &error, nullptr);
    if (!data->decoder) { delete data; return nullptr; }
    data->info = stb_vorbis_get_info(data->decoder);
    Mix_Music *music = static_cast<Mix_Music *>(std::malloc(sizeof(Mix_Music)));
    if (!music) { stb_vorbis_close(data->decoder); delete data; return nullptr; }
    music->data = data;
    return music;
}

extern "C" Mix_Music *Mix_LoadMUS(const char *) { return nullptr; }
extern "C" void Mix_FreeMusic(Mix_Music *music) {
    if (!music) return;
    MusicData *data = static_cast<MusicData *>(music->data);
    if (data == current_music) current_music = nullptr;
    stb_vorbis_close(data->decoder); delete data; std::free(music);
}
extern "C" int Mix_PlayMusic(Mix_Music *music, int loops) {
    if (!music) return -1;
    current_music = static_cast<MusicData *>(music->data);
    stb_vorbis_seek_start(current_music->decoder);
    music_loops = loops;
    music_buffer_frames = music_buffer_pos = 0;
    music_phase = 0;
    music_left = music_right = 0;
    next_music_sample(music_left, music_right);
    return 0;
}
extern "C" int Mix_FadeInMusic(Mix_Music *music, int loops, int) { return Mix_PlayMusic(music, loops); }
extern "C" int Mix_FadeOutMusic(int) { return Mix_HaltMusic(); }
extern "C" int Mix_HaltMusic(void) { current_music = nullptr; return 0; }
extern "C" int Mix_PlayingMusic(void) { return current_music != nullptr; }

static void next_music_sample(int &left, int &right) {
    if (!current_music) return;
    if (music_buffer_pos >= music_buffer_frames) {
        music_buffer_frames = stb_vorbis_get_samples_short_interleaved(current_music->decoder, 2, music_buffer, 4096);
        music_buffer_pos = 0;
        if (!music_buffer_frames && music_loops) {
            if (music_loops > 0) --music_loops;
            stb_vorbis_seek_start(current_music->decoder);
            music_buffer_frames = stb_vorbis_get_samples_short_interleaved(current_music->decoder, 2, music_buffer, 4096);
        }
        if (!music_buffer_frames) { current_music = nullptr; return; }
    }
    left = music_buffer[2 * music_buffer_pos];
    right = music_buffer[2 * music_buffer_pos + 1];
    ++music_buffer_pos;
}

extern "C" void vvv_mix_audio(short *output, size_t frames) {
    for (size_t i = 0; i < frames; ++i) {
        int left = 0, right = 0;
        if (current_music) {
            left += music_left * music_volume / MIX_MAX_VOLUME;
            right += music_right * music_volume / MIX_MAX_VOLUME;
            music_phase += current_music->info.sample_rate;
            while (music_phase >= 44100 && current_music) {
                music_phase -= 44100;
                next_music_sample(music_left, music_right);
            }
        }
        for (Voice &voice : voices) if (voice.chunk) {
            Mix_Chunk *chunk = voice.chunk;
            Uint64 frame = voice.position >> 16;
            Uint32 total = chunk->alen / (2 * chunk->channels);
            if (frame >= total) { voice.chunk = nullptr; continue; }
            short *samples = reinterpret_cast<short *>(chunk->abuf);
            int a = samples[frame * chunk->channels] * effects_volume / MIX_MAX_VOLUME;
            int b = chunk->channels == 2 ? samples[frame * 2 + 1] * effects_volume / MIX_MAX_VOLUME : a;
            left += a; right += b;
            voice.position += (Uint32(chunk->freq) << 16) / 44100;
        }
        output[2 * i] = std::max(-32768, std::min(32767, left));
        output[2 * i + 1] = std::max(-32768, std::min(32767, right));
    }
}
