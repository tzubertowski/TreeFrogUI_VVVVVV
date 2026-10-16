#include "../compat/SDL.h"
#include "../compat/SDL_mixer.h"

#include <cassert>

extern "C" void vvv_mix_audio(short *, size_t);
void OverlaySurfaceKeyed(SDL_Surface *, SDL_Surface *, Uint32);

int main() {
    SDL_Surface *source = SDL_CreateRGBSurface(0, 1, 1, 32,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    SDL_Surface *dest = SDL_CreateRGBSurface(0, 1, 1, 32,
        0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    *static_cast<unsigned *>(source->pixels) = 0x800000ff;
    SDL_Surface *copy = SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_ABGR8888, 0);
    assert((*static_cast<unsigned *>(copy->pixels) >> 24) == 0x80);
    SDL_BlitSurface(copy, nullptr, dest, nullptr);
    assert((*static_cast<unsigned *>(dest->pixels) >> 24) == 0x80);
    assert(((*static_cast<unsigned *>(dest->pixels) >> 16) & 0xff) >= 127);
    *static_cast<unsigned *>(copy->pixels) = 0x000000ff;
    *static_cast<unsigned *>(dest->pixels) = 0xff123456;
    SDL_BlitSurface(copy, nullptr, dest, nullptr);
    assert(*static_cast<unsigned *>(dest->pixels) == 0xff123456);
    *static_cast<unsigned *>(copy->pixels) = 0xff000000;
    SDL_BlitSurfaceTinted(copy, nullptr, dest, nullptr, 0x0000ff00);
    assert(*static_cast<unsigned *>(dest->pixels) == 0xff00ff00);
    *static_cast<unsigned *>(copy->pixels) = 0x800000ff;
    *static_cast<unsigned *>(dest->pixels) = 0xff0000ff;
    SDL_BlitSurface(copy, nullptr, dest, nullptr);
    unsigned blended = *static_cast<unsigned *>(dest->pixels);
    assert(((blended >> 16) & 0xff) >= 127 && ((blended >> 16) & 0xff) <= 129);
    assert((blended & 0xff) >= 126 && (blended & 0xff) <= 128);
    SDL_FillRect(dest, nullptr, 0);
    assert(*static_cast<unsigned *>(dest->pixels) == 0);
    SDL_FreeSurface(copy); SDL_FreeSurface(source); SDL_FreeSurface(dest);

    SDL_Surface *rgb565 = SDL_CreateRGBSurface(0, 1, 1, 16, 0xf800, 0x07e0, 0x001f, 0);
    SDL_Surface *rgba = SDL_CreateRGBSurface(0, 1, 1, 32,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    SDL_SetSurfaceBlendMode(rgba, SDL_BLENDMODE_BLEND);
    *static_cast<unsigned *>(rgba->pixels) = 0xff0000ff;
    SDL_BlitSurface(rgba, nullptr, rgb565, nullptr);
    assert(*static_cast<unsigned short *>(rgb565->pixels) == 0xf800);
    *static_cast<unsigned short *>(rgb565->pixels) = 0x001f;
    *static_cast<unsigned *>(rgba->pixels) = 0x800000ff;
    SDL_BlitSurface(rgba, nullptr, rgb565, nullptr);
    assert(*static_cast<unsigned short *>(rgb565->pixels) == 0x800f);
    *static_cast<unsigned *>(rgba->pixels) = 0xff000000;
    SDL_BlitSurfaceTinted(rgba, nullptr, rgb565, nullptr, 0xf800);
    assert(*static_cast<unsigned short *>(rgb565->pixels) == 0xf800);
    SDL_FillRect(rgb565, nullptr, 0x07e0);
    assert(*static_cast<unsigned short *>(rgb565->pixels) == 0x07e0);
    SDL_Surface *keyed = SDL_CreateRGBSurface(0, 1, 1, 16, 0xf800, 0x07e0, 0x001f, 0);
    *static_cast<unsigned short *>(keyed->pixels) = 0xbeef;
    *static_cast<unsigned short *>(rgb565->pixels) = 0x1234;
    OverlaySurfaceKeyed(keyed, rgb565, 0xdeadbeef);
    assert(*static_cast<unsigned short *>(rgb565->pixels) == 0x1234);
    *static_cast<unsigned short *>(keyed->pixels) = 0x001f;
    OverlaySurfaceKeyed(keyed, rgb565, 0xdeadbeef);
    assert(*static_cast<unsigned short *>(rgb565->pixels) == 0x001f);
    SDL_FreeSurface(keyed);
    SDL_FreeSurface(rgba); SDL_FreeSurface(rgb565);

    unsigned char wav[48] = {'R','I','F','F', 40,0,0,0, 'W','A','V','E',
        'f','m','t',' ', 16,0,0,0, 1,0, 2,0, 0x44,0xac,0,0,
        0x10,0xb1,2,0, 4,0, 16,0, 'd','a','t','a', 4,0,0,0,
        0xff,0x7f, 0xff,0x7f};
    SDL_RWops *stream = SDL_RWFromMem(wav, sizeof wav);
    Mix_Chunk *sound = Mix_LoadWAV_RW(stream, 1);
    assert(sound && Mix_PlayChannel(-1, sound, 0) >= 0);
    short samples[4] = {};
    vvv_mix_audio(samples, 2);
    assert(samples[0] > 0 && samples[1] > 0);
    Mix_FreeChunk(sound);
    Mix_CloseAudio();
}
