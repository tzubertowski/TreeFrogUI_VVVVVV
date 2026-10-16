# VVVVVV Libretro core

This branch adapts James O'Farrell's [OpenDingux VVVVVV port](https://github.com/jamesofarrell/VVVVVV)
to Libretro. The frontend owns display, input and audio. A local software
software surface feeds a 320×240 RGB565 frame to `video_refresh` at 30 FPS and
44.1 kHz stereo PCM to the Libretro audio callback; no SDL library or device is opened. The
game's ordinary save files go under the frontend save directory at
`VVVVVV/saves/`, and custom levels go under `VVVVVV/levels/`. Libretro save
states are unavailable; `retro_serialize_size()` returns zero.

## Build

From the repository root:

```sh
cmake -S libretro -B build/host -DCMAKE_BUILD_TYPE=Release
cmake --build build/host -j4
ctest --test-dir build/host --output-on-failure

cmake -S libretro -B build/sf3000 \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/libretro/sf3000-toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/sf3000 -j4
```

The MIPS toolchain file uses the local `sf3000toolchain` Buildroot SDK used by
TreeFrogUI. The target is little-endian MIPS32r2, o32, hard float, glibc. The
output is `vvvvvv_libretro.so` in the selected build directory. This is not
the RG350 uClibc toolchain from the original standalone port.

For an already installed TreeFrogUI card, run
`../sf3000_treefrogui/deploy_vvvvvv.sh r36hd` from this checkout. It builds the
core incrementally and uses the guarded card deployer to copy and verify only
the VVVVVV core, launcher, artwork, guide, and license. Existing `data.zip` and
saves stay on the card. Use the full TreeFrogUI release deploy for first setup.

## Data and launch

Provide your own `data.zip` from a VVVVVV copy or the separately distributed
Make and Play edition. In TreeFrogUI, put it at `roms/ports/vvvvvv/data.zip` and
launch **VVVVVV** from **Games → Ports**. The archive must contain
`graphics/{tiles,tiles2,tiles3,entcolours,sprites,flipsprites,font,fontmask,teleporter,levelcomplete,minimap,covered,elephant,gamecomplete,fliplevelcomplete,flipgamecomplete,site,site2,site3,ending,site4}.png`,
`sounds/jump.wav`, and `vvvvvvmusic.vvv`. The core rejects missing or incomplete
data. No game data is included in this repository or TreeFrogUI packages.
No game data is included in this repository or TreeFrogUI packages.

The original VVVVVV source and bundled PhysicsFS, TinyXML and LodePNG retain
their upstream licenses. The Libretro API header was taken from the local
FroggyClaw core, and the single-file `stb_vorbis.c` decoder from the local
FroggyTale source tree (public-domain/MIT). The software surface interface is
local to this core. This work stays on our fork branch;
the upstream OpenDingux PR template excludes homebrew console targets.

## Current limits

Only normal in-game saves are supported. The editor's text input is not mapped
to a Libretro keyboard, so creating custom levels on the device is limited.
Device frame rate and memory use require a real device test with user-supplied
`data.zip`.
