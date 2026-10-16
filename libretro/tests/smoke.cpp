#include "../libretro.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static unsigned video_frames;
static size_t audio_frames;
static const char *frame_dump;
static unsigned dump_frame_at;

static void video(const void *pixels, unsigned width, unsigned height, size_t pitch) {
    assert(pixels && width == 320 && height == 240 && pitch == 640);
    if (frame_dump && video_frames + 1 == dump_frame_at) {
        FILE *out = std::fopen(frame_dump, "wb");
        assert(out);
        std::fprintf(out, "P6\n%u %u\n255\n", width, height);
        const uint16_t *row = static_cast<const uint16_t *>(pixels);
        for (unsigned y = 0; y < height; ++y) {
            row = reinterpret_cast<const uint16_t *>(static_cast<const unsigned char *>(pixels) + y * pitch);
            for (unsigned x = 0; x < width; ++x) {
                uint16_t p = row[x];
                unsigned char rgb[] = {
                    static_cast<unsigned char>(((p >> 11) & 31) * 255 / 31),
                    static_cast<unsigned char>(((p >> 5) & 63) * 255 / 63),
                    static_cast<unsigned char>((p & 31) * 255 / 31)
                };
                std::fwrite(rgb, 1, sizeof rgb, out);
            }
        }
        std::fclose(out);
    }
    ++video_frames;
}
static size_t audio(const short *samples, size_t frames) {
    assert(samples && frames == 1470);
    audio_frames += frames;
    return frames;
}
static void input_poll() {}
static int16_t input_state(unsigned, unsigned, unsigned, unsigned) { return 0; }

static bool environment(unsigned command, void *data) {
    if (command == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)
        return *static_cast<retro_pixel_format *>(data) == RETRO_PIXEL_FORMAT_RGB565;
    if (command == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY) {
        *static_cast<const char **>(data) = "/tmp";
        return true;
    }
    return false;
}

int main(int argc, char **argv) {
    frame_dump = std::getenv("VVV_DUMP_FRAME");
    dump_frame_at = frame_dump ? std::strtoul(std::getenv("VVV_DUMP_AT") ? std::getenv("VVV_DUMP_AT") : "1", nullptr, 10) : 0;
    retro_set_environment(environment);
    retro_set_video_refresh(video);
    retro_set_audio_sample_batch(audio);
    retro_set_input_poll(input_poll);
    retro_set_input_state(input_state);
    retro_system_info system;
    retro_system_av_info av;
    retro_get_system_info(&system);
    retro_get_system_av_info(&av);
    assert(std::strcmp(system.valid_extensions, "zip") == 0 && system.need_fullpath);
    assert(av.geometry.base_width == 320 && av.geometry.base_height == 240);
    assert(av.timing.fps == 30.0 && av.timing.sample_rate == 44100.0);
    assert(retro_serialize_size() == 0 && !retro_serialize(nullptr, 0));
    retro_game_info missing = {};
    missing.path = "/tmp/does-not-exist-vvvvvv-data.zip";
    assert(!retro_load_game(&missing));

    if (argc > 1) {
        for (int launch = 0; launch < 2; ++launch) {
            retro_game_info game = {};
            game.path = argv[1];
            assert(retro_load_game(&game));
            assert(retro_get_memory_size(RETRO_MEMORY_SAVE_RAM) == 0);
            retro_get_system_av_info(&av);
            assert(av.geometry.base_width == 320 && av.timing.sample_rate == 44100.0);
            unsigned frames = frame_dump ? dump_frame_at : 3;
            for (unsigned i = 0; i < frames; ++i) retro_run();
            retro_unload_game();
        }
        if (!frame_dump) assert(video_frames == 6 && audio_frames == 6 * 1470);
    }
    retro_deinit();
}
