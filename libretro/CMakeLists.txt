cmake_minimum_required(VERSION 3.16)
project(vvvvvv_libretro C CXX)

file(GLOB ENGINE CONFIGURE_DEPENDS ../desktop_version/src/*.cpp)
list(REMOVE_ITEM ENGINE
    ${CMAKE_CURRENT_SOURCE_DIR}/../desktop_version/src/FileSystemUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../desktop_version/src/Screen.cpp)

add_library(vvvvvv_libretro SHARED
    ${ENGINE}
    core.cpp screen.cpp filesystem.cpp network.c
    compat/sdl.cpp compat/mixer.cpp third_party/stb_vorbis.c
    ../desktop_version/tinyxml/tinystr.cpp
    ../desktop_version/tinyxml/tinyxml.cpp
    ../desktop_version/tinyxml/tinyxmlerror.cpp
    ../desktop_version/tinyxml/tinyxmlparser.cpp
    ../desktop_version/lodepng/lodepng.c
    ../desktop_version/physfs/physfs.c
    ../desktop_version/physfs/physfs_archiver_dir.c
    ../desktop_version/physfs/physfs_archiver_unpacked.c
    ../desktop_version/physfs/physfs_archiver_zip.c
    ../desktop_version/physfs/physfs_byteorder.c
    ../desktop_version/physfs/physfs_unicode.c
    ../desktop_version/physfs/physfs_platform_posix.c
    ../desktop_version/physfs/physfs_platform_unix.c
    ../desktop_version/physfs/physfs_platform_windows.c)

target_include_directories(vvvvvv_libretro PRIVATE compat ../desktop_version/src
    ../desktop_version/tinyxml ../desktop_version/physfs ../desktop_version/lodepng)
target_compile_definitions(vvvvvv_libretro PRIVATE LIBRETRO PHYSFS_SUPPORTS_DEFAULT=0 PHYSFS_SUPPORTS_ZIP=1)
target_compile_options(vvvvvv_libretro PRIVATE -O2 -fPIC)
target_link_libraries(vvvvvv_libretro PRIVATE m)
target_link_options(vvvvvv_libretro PRIVATE -Wl,--no-undefined
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map)
set_target_properties(vvvvvv_libretro PROPERTIES PREFIX "" OUTPUT_NAME "vvvvvv_libretro")

add_executable(vvvvvv_smoke tests/smoke.cpp)
target_link_libraries(vvvvvv_smoke PRIVATE vvvvvv_libretro)
target_compile_options(vvvvvv_smoke PRIVATE -UNDEBUG)
add_executable(vvvvvv_compat_smoke tests/compat_smoke.cpp ../desktop_version/src/GraphicsUtil.cpp compat/sdl.cpp
    compat/mixer.cpp third_party/stb_vorbis.c)
target_include_directories(vvvvvv_compat_smoke PRIVATE compat ../desktop_version/src)
target_compile_options(vvvvvv_compat_smoke PRIVATE -UNDEBUG)
if(NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_test(NAME vvvvvv_smoke COMMAND vvvvvv_smoke)
    add_test(NAME vvvvvv_compat_smoke COMMAND vvvvvv_compat_smoke)
endif()
