#ifndef VVVVVV_LIBRETRO_SOFTWARE_MIXER_H
#define VVVVVV_LIBRETRO_SOFTWARE_MIXER_H
#include "SDL.h"
#ifdef __cplusplus
extern "C" {
#endif
#define MIX_MAX_VOLUME 128
typedef struct Mix_Chunk {
    int allocated;
    Uint8 *abuf;
    Uint32 alen;
    Uint8 volume;
    int freq, channels, bits;
} Mix_Chunk;
typedef struct Mix_Music { void *data; } Mix_Music;
int Mix_OpenAudio(int rate, Uint16 format, int channels, int buffer_size);
void Mix_CloseAudio(void);
const char *Mix_GetError(void);
Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *stream, int close_stream);
void Mix_FreeChunk(Mix_Chunk *chunk);
int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);
Mix_Music *Mix_LoadMUS_RW(SDL_RWops *stream, int close_stream);
Mix_Music *Mix_LoadMUS(const char *path);
void Mix_FreeMusic(Mix_Music *music);
int Mix_PlayMusic(Mix_Music *music, int loops);
int Mix_FadeInMusic(Mix_Music *music, int loops, int milliseconds);
int Mix_FadeOutMusic(int milliseconds);
int Mix_HaltMusic(void);
int Mix_PlayingMusic(void);
int Mix_VolumeMusic(int volume);
int Mix_Volume(int channel, int volume);
#ifdef __cplusplus
}
#include <vector>
Mix_Music *vvv_load_music(std::vector<Uint8> &&compressed);
#endif
#endif
