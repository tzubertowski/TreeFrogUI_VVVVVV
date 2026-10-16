#include "../desktop_version/src/FileSystemUtils.h"

#include <physfs.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits.h>

static char save_dir[PATH_MAX];
static char level_dir[PATH_MAX];

extern "C" bool vvv_filesystem_init(const char *content, const char *save_root) {
    std::fprintf(stderr, "VVVVVV fs: init\n");
    if (!content || !save_root || !PHYSFS_init(content)) return false;
    char base[PATH_MAX];
    if (snprintf(base, sizeof base, "%s/VVVVVV", save_root) >= int(sizeof base) ||
        snprintf(save_dir, sizeof save_dir, "%s/saves/", base) >= int(sizeof save_dir) ||
        snprintf(level_dir, sizeof level_dir, "%s/levels/", base) >= int(sizeof level_dir)) {
        PHYSFS_deinit(); return false;
    }
    mkdir(base, 0755);
    mkdir(save_dir, 0755);
    mkdir(level_dir, 0755);
    const char *images[] = {
        "tiles", "tiles2", "tiles3", "entcolours", "sprites", "flipsprites",
        "font", "fontmask", "teleporter", "levelcomplete", "minimap",
        "covered", "elephant", "gamecomplete", "fliplevelcomplete",
        "flipgamecomplete", "site", "site2", "site3", "ending", "site4"
    };
    bool valid = PHYSFS_mount(base, nullptr, 1) && PHYSFS_mount(content, nullptr, 1);
    std::fprintf(stderr, "VVVVVV fs: archive mounted=%d\n", valid);
    for (const char *image : images) {
        char path[96];
        snprintf(path, sizeof path, "graphics/%s.png", image);
        PHYSFS_File *file = valid ? PHYSFS_openRead(path) : nullptr;
        unsigned char signature[8];
        valid = file && PHYSFS_read(file, signature, 1, sizeof signature) == sizeof signature &&
                !memcmp(signature, "\x89PNG\r\n\x1a\n", sizeof signature);
        if (file) PHYSFS_close(file);
        if (!valid) break;
    }
    PHYSFS_File *music = valid ? PHYSFS_openRead("vvvvvvmusic.vvv") : nullptr;
    valid = music && PHYSFS_fileLength(music) >= 128 * 60 && PHYSFS_exists("sounds/jump.wav");
    std::fprintf(stderr, "VVVVVV fs: data checked=%d\n", valid);
    if (music) PHYSFS_close(music);
    if (!valid) {
        std::fprintf(stderr, "VVVVVV: data.zip is missing required game files\n");
        PHYSFS_deinit(); return false;
    }
    return true;
}

void FILESYSTEM_init(char *) {}
void FILESYSTEM_deinit() { PHYSFS_deinit(); }
char *FILESYSTEM_getUserSaveDirectory() { return save_dir; }
char *FILESYSTEM_getUserLevelDirectory() { return level_dir; }

void FILESYSTEM_loadFileToMemory(const char *name, unsigned char **memory, size_t *length) {
    *memory = nullptr;
    if (length) *length = 0;
    PHYSFS_File *file = PHYSFS_openRead(name);
    if (!file) {
        std::fprintf(stderr, "VVVVVV file open failed: %s: %s\n", name,
                     PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        return;
    }
    PHYSFS_sint64 size = PHYSFS_fileLength(file);
    if (size >= 0 && size <= 64 * 1024 * 1024) {
        unsigned char *data = static_cast<unsigned char *>(std::malloc(size ? size : 1));
        PHYSFS_sint64 got = data ? PHYSFS_readBytes(file, data, size) : -1;
        if (data && got == size) {
            *memory = data;
            if (length) *length = size;
        } else {
            std::fprintf(stderr, "VVVVVV file read failed: %s size=%lld got=%lld: %s\n",
                         name, (long long)size, (long long)got,
                         PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            std::free(data);
        }
    } else std::fprintf(stderr, "VVVVVV file length failed: %s size=%lld\n", name, (long long)size);
    PHYSFS_close(file);
}

void FILESYSTEM_freeMemory(unsigned char **memory) { std::free(*memory); *memory = nullptr; }

std::vector<std::string> FILESYSTEM_getLevelDirFileNames() {
    std::vector<std::string> names;
    char **list = PHYSFS_enumerateFiles("levels");
    if (!list) return names;
    for (char **item = list; *item; ++item) names.push_back(std::string("levels/") + *item);
    PHYSFS_freeList(list);
    return names;
}
