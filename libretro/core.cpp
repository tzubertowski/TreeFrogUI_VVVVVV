#include "libretro.h"
#include "../desktop_version/src/FileSystemUtils.h"

#include <cstdio>
#include <cstring>
#include <string>

extern "C" bool vvv_filesystem_init(const char *content, const char *save_root);
extern "C" void vvv_game_start();
extern "C" void vvv_game_frame();
extern "C" void vvv_game_stop();
extern "C" void vvv_advance_ticks(unsigned milliseconds);
extern "C" void vvv_mix_audio(short *output, size_t frames);

static retro_environment_t environment;
static retro_video_refresh_t video;
static retro_audio_sample_t audio_sample;
static retro_audio_sample_batch_t audio_batch;
static retro_input_poll_t input_poll;
static retro_input_state_t input_state;
static bool loaded;
extern "C" { unsigned short vvv_input_bits = 0; }
static unsigned frame_counter;

extern "C" void vvv_submit_frame(const void *pixels, unsigned width, unsigned height, size_t pitch) {
    if (video) video(pixels, width, height, pitch);
}

extern "C" void retro_set_environment(retro_environment_t callback) { environment = callback; }
extern "C" void retro_set_video_refresh(retro_video_refresh_t callback) { video = callback; }
extern "C" void retro_set_audio_sample(retro_audio_sample_t callback) { audio_sample = callback; }
extern "C" void retro_set_audio_sample_batch(retro_audio_sample_batch_t callback) { audio_batch = callback; }
extern "C" void retro_set_input_poll(retro_input_poll_t callback) { input_poll = callback; }
extern "C" void retro_set_input_state(retro_input_state_t callback) { input_state = callback; }
extern "C" unsigned retro_api_version(void) { return RETRO_API_VERSION; }
extern "C" void retro_init(void) {}
extern "C" void retro_deinit(void) { if (loaded) retro_unload_game(); }

extern "C" void retro_get_system_info(struct retro_system_info *info) {
    *info = {};
    info->library_name = "VVVVVV";
    info->library_version = "2.0-libretro";
    info->valid_extensions = "zip";
    info->need_fullpath = true;
    info->block_extract = true;
}

extern "C" void retro_get_system_av_info(struct retro_system_av_info *info) {
    std::fprintf(stderr, "VVVVVV callback: get_av\n");
    *info = {};
    info->geometry.base_width = info->geometry.max_width = 320;
    info->geometry.base_height = info->geometry.max_height = 240;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
    info->timing.fps = 30.0;
    info->timing.sample_rate = 44100.0;
}

extern "C" bool retro_load_game(const struct retro_game_info *game) {
    if (loaded || !game || !game->path || !environment) return false;
    std::fprintf(stderr, "VVVVVV load: pixel format\n");
    enum retro_pixel_format format = RETRO_PIXEL_FORMAT_RGB565;
    if (!environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format)) return false;
    std::fprintf(stderr, "VVVVVV load: save directory\n");
    const char *save_dir = nullptr;
    if (!environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) || !save_dir || !*save_dir) {
        static std::string fallback;
        fallback = game->path;
        size_t slash = fallback.find_last_of("/\\");
        fallback.resize(slash == std::string::npos ? 0 : slash);
        save_dir = fallback.c_str();
    }
    std::fprintf(stderr, "VVVVVV load: content mount\n");
    if (!vvv_filesystem_init(game->path, save_dir)) return false;
    std::fprintf(stderr, "VVVVVV load: game start\n");
    vvv_game_start();
    std::fprintf(stderr, "VVVVVV load: ready\n");
    loaded = true;
    return true;
}

extern "C" void retro_unload_game(void) {
    if (!loaded) return;
    vvv_game_stop();
    FILESYSTEM_deinit();
    loaded = false;
}

extern "C" void retro_run(void) {
    if (!frame_counter) std::fprintf(stderr, "VVVVVV callback: first_run\n");
    if (!loaded) return;
    if (input_poll) input_poll();
    vvv_input_bits = 0;
    if (input_state) for (unsigned i = 0; i < 16; ++i)
        if (input_state(0, RETRO_DEVICE_JOYPAD, 0, i)) vvv_input_bits |= 1u << i;
    vvv_advance_ticks(33 + (++frame_counter % 3 == 0));
    vvv_game_frame();
    short samples[1470 * 2];
    vvv_mix_audio(samples, 1470);
    if (audio_batch) audio_batch(samples, 1470);
    else if (audio_sample) for (unsigned i = 0; i < 1470; ++i) audio_sample(samples[2 * i], samples[2 * i + 1]);
}

extern "C" void retro_reset(void) {
    if (loaded) { vvv_game_stop(); vvv_game_start(); }
}
extern "C" void retro_set_controller_port_device(unsigned, unsigned) {}
extern "C" size_t retro_serialize_size(void) { std::fprintf(stderr, "VVVVVV callback: serialize_size\n"); return 0; }
extern "C" bool retro_serialize(void *, size_t) { return false; }
extern "C" bool retro_unserialize(const void *, size_t) { return false; }
extern "C" void *retro_get_memory_data(unsigned) { std::fprintf(stderr, "VVVVVV callback: memory_data\n"); return nullptr; }
extern "C" size_t retro_get_memory_size(unsigned) { std::fprintf(stderr, "VVVVVV callback: memory_size\n"); return 0; }
extern "C" unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }
extern "C" bool retro_load_game_special(unsigned, const struct retro_game_info *, size_t) { return false; }
extern "C" void retro_cheat_reset(void) {}
extern "C" void retro_cheat_set(unsigned, bool, const char *) {}
