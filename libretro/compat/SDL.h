#ifndef VVVVVV_LIBRETRO_SOFTWARE_SDL_H
#define VVVVVV_LIBRETRO_SOFTWARE_SDL_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef int16_t Sint16;
typedef int32_t Sint32;
typedef int SDL_Keycode;
typedef int SDL_JoystickID;
typedef struct SDL_GameController SDL_GameController;
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;
typedef struct SDL_PixelFormat {
    Uint8 BitsPerPixel, BytesPerPixel;
    Uint32 Rmask, Gmask, Bmask, Amask;
    Uint8 Rshift, Gshift, Bshift, Ashift;
} SDL_PixelFormat;
typedef enum SDL_BlendMode { SDL_BLENDMODE_NONE, SDL_BLENDMODE_BLEND } SDL_BlendMode;
typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat *format;
    int w, h, pitch;
    void *pixels;
    SDL_BlendMode blend_mode;
} SDL_Surface;
typedef struct SDL_RWops {
    union {
        struct { Uint8 *base, *here, *stop; } mem;
    } hidden;
} SDL_RWops;
typedef enum SDL_GameControllerButton {
    SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK, SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT
} SDL_GameControllerButton;

#define SDL_SWSURFACE 0
#define SDL_SURFACE_EXTERNAL_PIXELS 0x80000000u
#define SDL_PIXELFORMAT_ABGR8888 0x16762004u
#define AUDIO_S16SYS 0x8010
#define SDL_BIG_ENDIAN 4321
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SDL_BYTEORDER SDL_BIG_ENDIAN
#else
#define SDL_BYTEORDER 1234
#endif
#define SDL_assert(expr) assert(expr)

#define SDLK_RETURN '\r'
#define SDLK_SPACE ' '
#define SDLK_BACKSPACE '\b'
#define SDLK_COMMA ','
#define SDLK_PERIOD '.'
#define SDLK_LEFT 0x40000050
#define SDLK_RIGHT 0x4000004f
#define SDLK_UP 0x40000052
#define SDLK_DOWN 0x40000051
#define SDLK_KP_ENTER 0x40000058
#define SDLK_LCTRL 0x400000e0
#define SDLK_LSHIFT 0x400000e1
#define SDLK_LALT 0x400000e2
#define SDLK_RCTRL 0x400000e4
#define SDLK_RSHIFT 0x400000e5
#define SDLK_F1 0x4000003a
#define SDLK_F2 0x4000003b
#define SDLK_F3 0x4000003c
#define SDLK_F4 0x4000003d
#define SDLK_F5 0x4000003e
#define SDLK_F10 0x40000043
#define SDLK_0 '0'
#define SDLK_1 '1'
#define SDLK_2 '2'
#define SDLK_3 '3'
#define SDLK_4 '4'
#define SDLK_5 '5'
#define SDLK_6 '6'
#define SDLK_7 '7'
#define SDLK_8 '8'
#define SDLK_9 '9'
#define SDLK_a 'a'
#define SDLK_d 'd'
#define SDLK_e 'e'
#define SDLK_i 'i'
#define SDLK_l 'l'
#define SDLK_m 'm'
#define SDLK_o 'o'
#define SDLK_p 'p'
#define SDLK_r 'r'
#define SDLK_s 's'
#define SDLK_t 't'
#define SDLK_u 'u'
#define SDLK_v 'v'
#define SDLK_w 'w'
#define SDLK_x 'x'
#define SDLK_y 'y'
#define SDLK_z 'z'

Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32 milliseconds);
void SDL_Quit(void);
const char *SDL_GetError(void);
SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
                                  Uint32 red, Uint32 green, Uint32 blue, Uint32 alpha);
SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch,
                                      Uint32 red, Uint32 green, Uint32 blue, Uint32 alpha);
SDL_Surface *SDL_ConvertSurfaceFormat(SDL_Surface *source, Uint32 format, Uint32 flags);
void SDL_FreeSurface(SDL_Surface *surface);
int SDL_SetSurfaceBlendMode(SDL_Surface *surface, SDL_BlendMode mode);
Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 red, Uint8 green, Uint8 blue);
int SDL_FillRect(SDL_Surface *surface, const SDL_Rect *rect, Uint32 color);
int SDL_BlitSurface(SDL_Surface *source, const SDL_Rect *source_rect,
                    SDL_Surface *target, SDL_Rect *target_rect);
int SDL_BlitSurfaceTinted(SDL_Surface *source, const SDL_Rect *source_rect,
                          SDL_Surface *target, SDL_Rect *target_rect, Uint32 color);
SDL_RWops *SDL_RWFromMem(void *memory, int size);
int SDL_RWclose(SDL_RWops *stream);

#ifdef __cplusplus
}
#endif
#endif
