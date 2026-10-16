#include "SDL.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

static Uint32 ticks;

extern "C" void vvv_advance_ticks(Uint32 milliseconds) { ticks += milliseconds; }
extern "C" Uint32 SDL_GetTicks(void) { return ticks; }
extern "C" void SDL_Delay(Uint32) {}
extern "C" void SDL_Quit(void) {}
extern "C" const char *SDL_GetError(void) { return "software surface error"; }

static int shift(Uint32 mask) {
    if (!mask) return 0;
    int n = 0;
    while (!(mask & 1)) { mask >>= 1; ++n; }
    return n;
}

static Uint8 component(Uint32 pixel, Uint32 mask, Uint8 component_shift) {
    if (!mask) return 255;
    Uint32 value = (pixel & mask) >> component_shift;
    Uint32 maximum = mask >> component_shift;
    if (maximum == 255) return static_cast<Uint8>(value);
    return static_cast<Uint8>((value * 255 + maximum / 2) / maximum);
}

static Uint32 pack(Uint8 r, Uint8 g, Uint8 b, Uint8 a, const SDL_PixelFormat *f) {
    Uint32 out = 0;
    const Uint8 values[] = {r, g, b, a};
    const Uint32 masks[] = {f->Rmask, f->Gmask, f->Bmask, f->Amask};
    const Uint8 shifts[] = {f->Rshift, f->Gshift, f->Bshift, f->Ashift};
    for (int i = 0; i < 4; ++i) if (masks[i]) {
        Uint32 maximum = masks[i] >> shifts[i];
        Uint32 value = maximum == 255 ? values[i] : (values[i] * maximum + 127) / 255;
        out |= value << shifts[i];
    }
    return out;
}

static Uint32 read_pixel(const SDL_Surface *surface, int x, int y) {
    const Uint8 *p = static_cast<const Uint8 *>(surface->pixels) + y * surface->pitch + x * surface->format->BytesPerPixel;
    Uint32 value = 0;
    std::memcpy(&value, p, surface->format->BytesPerPixel);
    return value;
}

static void write_pixel(SDL_Surface *surface, int x, int y, Uint32 value) {
    Uint8 *p = static_cast<Uint8 *>(surface->pixels) + y * surface->pitch + x * surface->format->BytesPerPixel;
    std::memcpy(p, &value, surface->format->BytesPerPixel);
}

static SDL_Surface *new_surface(void *pixels, bool owned, int width, int height, int depth, int pitch,
                                Uint32 red, Uint32 green, Uint32 blue, Uint32 alpha) {
    if (!pixels || width <= 0 || height <= 0 || (depth != 16 && depth != 24 && depth != 32)) return nullptr;
    SDL_Surface *surface = static_cast<SDL_Surface *>(std::calloc(1, sizeof(SDL_Surface)));
    SDL_PixelFormat *format = static_cast<SDL_PixelFormat *>(std::calloc(1, sizeof(SDL_PixelFormat)));
    if (!surface || !format) { std::free(surface); std::free(format); if (owned) std::free(pixels); return nullptr; }
    format->BitsPerPixel = depth;
    format->BytesPerPixel = depth / 8;
    format->Rmask = red; format->Gmask = green; format->Bmask = blue; format->Amask = alpha;
    format->Rshift = shift(red); format->Gshift = shift(green);
    format->Bshift = shift(blue); format->Ashift = shift(alpha);
    surface->format = format; surface->pixels = pixels; surface->w = width;
    surface->h = height; surface->pitch = pitch;
    surface->flags = owned ? 0 : SDL_SURFACE_EXTERNAL_PIXELS;
    surface->blend_mode = alpha ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE;
    return surface;
}

extern "C" SDL_Surface *SDL_CreateRGBSurface(Uint32, int w, int h, int depth,
                                               Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096 || (depth != 16 && depth != 24 && depth != 32)) return nullptr;
    int pitch = w * depth / 8;
    return new_surface(std::calloc(h, pitch), true, w, h, depth, pitch, r, g, b, a);
}

extern "C" SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int w, int h, int depth, int pitch,
                                                   Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    return new_surface(pixels, false, w, h, depth, pitch, r, g, b, a);
}

extern "C" void SDL_FreeSurface(SDL_Surface *surface) {
    if (!surface) return;
    if (!(surface->flags & SDL_SURFACE_EXTERNAL_PIXELS)) std::free(surface->pixels);
    std::free(surface->format);
    std::free(surface);
}

extern "C" int SDL_SetSurfaceBlendMode(SDL_Surface *surface, SDL_BlendMode mode) {
    if (!surface) return -1;
    surface->blend_mode = mode;
    return 0;
}

extern "C" Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b) {
    return pack(r, g, b, 255, format);
}

extern "C" int SDL_FillRect(SDL_Surface *surface, const SDL_Rect *rect, Uint32 color) {
    if (!surface) return -1;
    int left = std::max(0, rect ? rect->x : 0), top = std::max(0, rect ? rect->y : 0);
    int right = std::min(surface->w, rect ? rect->x + rect->w : surface->w);
    int bottom = std::min(surface->h, rect ? rect->y + rect->h : surface->h);
    if (surface->format->BytesPerPixel == 4) {
        const int width = right - left;
        if (width <= 0 || bottom <= top) return 0;
        for (int y = top; y < bottom; ++y) {
            Uint32 *row = reinterpret_cast<Uint32 *>(static_cast<Uint8 *>(surface->pixels) + y * surface->pitch) + left;
            if (!color) std::memset(row, 0, width * sizeof(*row));
            else std::fill_n(row, width, color);
        }
        return 0;
    }
    if (surface->format->BytesPerPixel == 2) {
        const int width = right - left;
        if (width <= 0 || bottom <= top) return 0;
        for (int y = top; y < bottom; ++y) {
            Uint16 *row = reinterpret_cast<Uint16 *>(static_cast<Uint8 *>(surface->pixels) + y * surface->pitch) + left;
            if (!color) std::memset(row, 0, width * sizeof(*row));
            else std::fill_n(row, width, static_cast<Uint16>(color));
        }
        return 0;
    }
    for (int y = top; y < bottom; ++y) for (int x = left; x < right; ++x) write_pixel(surface, x, y, color);
    return 0;
}

static int blit_surface(SDL_Surface *src, const SDL_Rect *source, SDL_Surface *dst, SDL_Rect *target,
                        const Uint32 *tint) {
    if (!src || !dst) return -1;
    int sx = source ? source->x : 0, sy = source ? source->y : 0;
    int w = source ? source->w : src->w, h = source ? source->h : src->h;
    int dx = target ? target->x : 0, dy = target ? target->y : 0;
    const SDL_PixelFormat *sf = src->format, *df = dst->format;
    const bool rgb565 = df->BytesPerPixel == 2 && df->Rmask == 0xf800 && df->Gmask == 0x07e0 && df->Bmask == 0x001f && !df->Amask;
    if (!tint && src != dst && src->blend_mode == SDL_BLENDMODE_NONE && rgb565 &&
        sf->BitsPerPixel == 16 && sf->Rmask == df->Rmask && sf->Gmask == df->Gmask && sf->Bmask == df->Bmask) {
        int x0 = std::max(0, -sx), x1 = std::min(w, src->w - sx);
        x0 = std::max(x0, -dx); x1 = std::min(x1, dst->w - dx);
        int y0 = std::max(0, -sy), y1 = std::min(h, src->h - sy);
        y0 = std::max(y0, -dy); y1 = std::min(y1, dst->h - dy);
        if (x1 > x0 && y1 > y0) for (int y = y0; y < y1; ++y) {
            const Uint16 *from = reinterpret_cast<const Uint16 *>(static_cast<const Uint8 *>(src->pixels) + (sy + y) * src->pitch) + sx + x0;
            Uint16 *to = reinterpret_cast<Uint16 *>(static_cast<Uint8 *>(dst->pixels) + (dy + y) * dst->pitch) + dx + x0;
            std::memcpy(to, from, (x1 - x0) * sizeof(*to));
        }
        if (target) { target->w = w; target->h = h; }
        return 0;
    }
    if (rgb565 && sf->BytesPerPixel == 4 && sf->Rmask == 0x000000ff && sf->Gmask == 0x0000ff00 && sf->Bmask == 0x00ff0000 && sf->Amask == 0xff000000) {
        const Uint8 tr = tint ? component(*tint, df->Rmask, df->Rshift) : 0;
        const Uint8 tg = tint ? component(*tint, df->Gmask, df->Gshift) : 0;
        const Uint8 tb = tint ? component(*tint, df->Bmask, df->Bshift) : 0;
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            const int xx = sx + x, yy = sy + y, tx = dx + x, ty = dy + y;
            if (xx < 0 || yy < 0 || xx >= src->w || yy >= src->h || tx < 0 || ty < 0 || tx >= dst->w || ty >= dst->h) continue;
            const Uint32 pixel = reinterpret_cast<const Uint32 *>(static_cast<const Uint8 *>(src->pixels) + yy * src->pitch)[xx];
            Uint8 r = tint ? tr : pixel & 255, g = tint ? tg : (pixel >> 8) & 255, b = tint ? tb : (pixel >> 16) & 255;
            const Uint8 a = pixel >> 24;
            if (src->blend_mode == SDL_BLENDMODE_BLEND && a < 255) {
                if (!a) continue;
                const Uint16 old = reinterpret_cast<const Uint16 *>(static_cast<const Uint8 *>(dst->pixels) + ty * dst->pitch)[tx];
                const Uint8 inv = 255 - a;
                r = (r * a + component(old, df->Rmask, df->Rshift) * inv + 127) / 255;
                g = (g * a + component(old, df->Gmask, df->Gshift) * inv + 127) / 255;
                b = (b * a + component(old, df->Bmask, df->Bshift) * inv + 127) / 255;
            }
            reinterpret_cast<Uint16 *>(static_cast<Uint8 *>(dst->pixels) + ty * dst->pitch)[tx] = static_cast<Uint16>(pack(r, g, b, 255, df));
        }
        if (target) { target->w = w; target->h = h; }
        return 0;
    }
    const bool common32 = sf->BytesPerPixel == 4 && df->BytesPerPixel == 4 &&
        sf->Rmask == 0x000000ff && sf->Gmask == 0x0000ff00 && sf->Bmask == 0x00ff0000 && sf->Amask == 0xff000000 &&
        df->Amask == 0xff000000 && df->Rmask + df->Gmask + df->Bmask == 0x00ffffff;
    if (common32) {
        const Uint8 sr = sf->Rshift, sg = sf->Gshift, sb = sf->Bshift;
        const Uint8 dr = df->Rshift, dg = df->Gshift, db = df->Bshift;
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            int xx = sx + x, yy = sy + y, tx = dx + x, ty = dy + y;
            if (xx < 0 || yy < 0 || xx >= src->w || yy >= src->h || tx < 0 || ty < 0 || tx >= dst->w || ty >= dst->h) continue;
            const Uint32 pixel = reinterpret_cast<const Uint32 *>(static_cast<const Uint8 *>(src->pixels) + yy * src->pitch)[xx];
            const Uint32 color = tint ? *tint : pixel;
            const Uint8 r = (color >> (tint ? dr : sr)) & 255;
            const Uint8 g = (color >> (tint ? dg : sg)) & 255;
            const Uint8 b = (color >> (tint ? db : sb)) & 255;
            const Uint8 a = pixel >> 24;
            Uint32 out = (Uint32(r) << dr) | (Uint32(g) << dg) | (Uint32(b) << db) | (Uint32(a) << df->Ashift);
            Uint32 *dst_pixel = reinterpret_cast<Uint32 *>(static_cast<Uint8 *>(dst->pixels) + ty * dst->pitch) + tx;
            if (src->blend_mode == SDL_BLENDMODE_BLEND && a < 255) {
                if (!a) continue;
                const Uint32 old = *dst_pixel;
                const Uint8 old_a = old >> 24;
                if (old_a == 255) {
                    const Uint8 inverse = 255 - a;
                    const Uint8 or_ = (old >> dr) & 255, og = (old >> dg) & 255, ob = (old >> db) & 255;
                    const Uint8 rr = (r * a + or_ * inverse + 127) / 255;
                    const Uint8 rg = (g * a + og * inverse + 127) / 255;
                    const Uint8 rb = (b * a + ob * inverse + 127) / 255;
                    out = (Uint32(rr) << dr) | (Uint32(rg) << dg) | (Uint32(rb) << db) | 0xff000000;
                } else if (old_a != 0) {
                    Uint32 out_a = a + (old_a * (255 - a) + 127) / 255;
                    const Uint8 rr = (r * a * 255 + ((old >> dr) & 255) * old_a * (255 - a) + out_a * 127) / (out_a * 255);
                    const Uint8 rg = (g * a * 255 + ((old >> dg) & 255) * old_a * (255 - a) + out_a * 127) / (out_a * 255);
                    const Uint8 rb = (b * a * 255 + ((old >> db) & 255) * old_a * (255 - a) + out_a * 127) / (out_a * 255);
                    out = (Uint32(rr) << dr) | (Uint32(rg) << dg) | (Uint32(rb) << db) | (out_a << df->Ashift);
                }
            }
            *dst_pixel = out;
        }
        if (target) { target->w = w; target->h = h; }
        return 0;
    }
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        int xx = sx + x, yy = sy + y, tx = dx + x, ty = dy + y;
        if (xx < 0 || yy < 0 || xx >= src->w || yy >= src->h || tx < 0 || ty < 0 || tx >= dst->w || ty >= dst->h) continue;
        Uint32 pixel = read_pixel(src, xx, yy);
        const Uint32 color = tint ? *tint : pixel;
        Uint8 r = component(color, tint ? dst->format->Rmask : src->format->Rmask, tint ? dst->format->Rshift : src->format->Rshift);
        Uint8 g = component(color, tint ? dst->format->Gmask : src->format->Gmask, tint ? dst->format->Gshift : src->format->Gshift);
        Uint8 b = component(color, tint ? dst->format->Bmask : src->format->Bmask, tint ? dst->format->Bshift : src->format->Bshift);
        Uint8 a = component(pixel, src->format->Amask, src->format->Ashift);
        if (src->blend_mode == SDL_BLENDMODE_BLEND && a < 255) {
            if (!a) continue;
            Uint32 old = read_pixel(dst, tx, ty);
            Uint8 old_a = component(old, dst->format->Amask, dst->format->Ashift);
            Uint32 out_a = a + (old_a * (255 - a) + 127) / 255;
            if (out_a) {
                r = (r * a * 255 + component(old, dst->format->Rmask, dst->format->Rshift) * old_a * (255 - a) + out_a * 127) / (out_a * 255);
                g = (g * a * 255 + component(old, dst->format->Gmask, dst->format->Gshift) * old_a * (255 - a) + out_a * 127) / (out_a * 255);
                b = (b * a * 255 + component(old, dst->format->Bmask, dst->format->Bshift) * old_a * (255 - a) + out_a * 127) / (out_a * 255);
            }
            a = static_cast<Uint8>(out_a);
        }
        write_pixel(dst, tx, ty, pack(r, g, b, a, dst->format));
    }
    if (target) { target->w = w; target->h = h; }
    return 0;
}

extern "C" int SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *source, SDL_Surface *dst, SDL_Rect *target) {
    return blit_surface(src, source, dst, target, nullptr);
}

extern "C" int SDL_BlitSurfaceTinted(SDL_Surface *src, const SDL_Rect *source, SDL_Surface *dst,
                                       SDL_Rect *target, Uint32 color) {
    return blit_surface(src, source, dst, target, &color);
}

extern "C" SDL_Surface *SDL_ConvertSurfaceFormat(SDL_Surface *source, Uint32, Uint32) {
    if (!source) return nullptr;
    SDL_Surface *converted = SDL_CreateRGBSurface(0, source->w, source->h, 32,
                                                   0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (converted) for (int y = 0; y < source->h; ++y) for (int x = 0; x < source->w; ++x) {
        Uint32 pixel = read_pixel(source, x, y);
        write_pixel(converted, x, y,
                    pack(component(pixel, source->format->Rmask, source->format->Rshift),
                         component(pixel, source->format->Gmask, source->format->Gshift),
                         component(pixel, source->format->Bmask, source->format->Bshift),
                         component(pixel, source->format->Amask, source->format->Ashift), converted->format));
    }
    return converted;
}

extern "C" SDL_RWops *SDL_RWFromMem(void *memory, int size) {
    if (!memory || size < 0) return nullptr;
    SDL_RWops *stream = static_cast<SDL_RWops *>(std::calloc(1, sizeof(SDL_RWops)));
    if (stream) {
        stream->hidden.mem.base = static_cast<Uint8 *>(memory);
        stream->hidden.mem.here = static_cast<Uint8 *>(memory);
        stream->hidden.mem.stop = static_cast<Uint8 *>(memory) + size;
    }
    return stream;
}

extern "C" int SDL_RWclose(SDL_RWops *stream) { std::free(stream); return 0; }
