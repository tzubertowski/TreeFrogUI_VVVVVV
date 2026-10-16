#include "../desktop_version/src/Screen.h"
#include "../desktop_version/src/GraphicsUtil.h"

#include <stdint.h>

extern "C" void vvv_submit_frame(const void *pixels, unsigned width, unsigned height, size_t pitch);

Screen::Screen() : isWindowed(true), isFiltered(false), badSignalEffect(false), glScreen(false),
    stretchMode(0), m_window(nullptr), m_renderer(nullptr), m_screenTexture(nullptr) {
    filterSubrect = {0, 0, 320, 240};
    m_screen = SDL_CreateRGBSurface(0, 320, 240, 16,
                                    0xf800, 0x07e0, 0x001f, 0);
    SDL_SetSurfaceBlendMode(m_screen, SDL_BLENDMODE_NONE);
}

Screen::~Screen() { SDL_FreeSurface(m_screen); }

void Screen::ResizeScreen(int, int) {}
void Screen::GetWindowSize(int *width, int *height) { *width = 320; *height = 240; }
const SDL_PixelFormat *Screen::GetFormat() { return m_screen->format; }
void Screen::toggleFullScreen() {}
void Screen::toggleStretchMode() {}
void Screen::toggleLinearFilter() {}
void Screen::ClearScreen(int colour) { SDL_FillRect(m_screen, nullptr, colour); }

void Screen::UpdateScreen(SDL_Surface *buffer, SDL_Rect *rect) {
    if (!buffer || !m_screen) return;
    SDL_Surface *source = badSignalEffect ? ApplyFilter(buffer) : buffer;
    SDL_FillRect(m_screen, nullptr, 0);
    BlitSurfaceStandard(source, nullptr, m_screen, rect);
    if (source != buffer) SDL_FreeSurface(source);
}

void Screen::FlipScreen() {
    vvv_submit_frame(m_screen->pixels, 320, 240, m_screen->pitch);
    SDL_FillRect(m_screen, nullptr, 0);
}
