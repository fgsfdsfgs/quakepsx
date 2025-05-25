<div align="center">
  <img src="https://github.com/fgsfdsfgs/quakepsx/blob/master/doc/img/screenshot0.png?raw=true" width="30%"/>
  <img src="https://github.com/fgsfdsfgs/quakepsx/blob/master/doc/img/screenshot1.png?raw=true" width="30%"/>
  <img src="https://github.com/fgsfdsfgs/quakepsx/blob/master/doc/img/screenshot2.png?raw=true" width="30%"/>
</div>

## QuakePSX

This is an experimental port of the original Quake to the Sony PlayStation.

It is based on the original Quake codebase, with the main idea being to keep it as close to the original as possible, other than the resource formats.

### Status
* Runs on both emulators and real hardware at mindblowing framerates of ~15 FPS on average.
* DualShock controllers and the PS Mouse are supported for input.
* CD Audio is supported, if the game is built with CDDA tracks.
* Map conversion is far from perfect and has many texture alignment and lighting issues.
* The entirety of Episode 1 is playable, but many maps from Episode 2 and some from other episodes do not fit in memory.
* There is no multiplayer of any kind.
* There is no save system.
* The renderer is slow and lacks good triangle subdivision/clipping and dynamic lighting support.
* There are some bugs in the movement code that can cause you to get stuck or gain high speed for no reason.
* There might be many other bugs.

### Building

#### Prerequisites:
* Linux or WSL, though MSYS might work as well;
* [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK);
* [mkpsxiso](https://github.com/Lameguy64/mkpsxiso);
* `gcc` for your host platform;
* `mipsel-none-elf-gcc`, preferably versions 12.2 - 13.3;
* `cmake`, `git`, `make`;
* Quake (shareware or full version).

#### Instructions:
1. Install the prerequisites and ensure `mipsel-none-elf-gcc` and `mkpsxiso` are in your `PATH` and the PSn00bSDK environment variables are set:
   ```
   export PSN00BSDK_LIBS=/path/to/psn00bsdk/lib/libpsn00b
   export PATH=/path/to/psn00bsdk/bin:$PATH
   ```
2. Clone the repo:
   ```
   git clone https://github.com/fgsfdsfgs/quakepsx.git && cd quakepsx
   ```
4. (Optional) You can put the music tracks from Quake into `id1psx/music/`.  
   Tracks should be in WAV format and named `track02.wav ... track11.wav`.
5. Build the tools and cook the maps:
   ```
   ./tools/scripts/cook_id1.sh /path/to/Quake/id1
   ```
7. If everything completed with no errors, you can now configure the game:
   * If you don't have the music:
   ```
   cmake --preset default -DCMAKE_BUILD_TYPE=RelWithDebInfo -Bbuild
   ```
   * If you do have the music:
   ```
   cmake --preset default -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_CDDA=ON -Bbuild
   ```
8. Build:
   ```
   cmake --build build -j4
   ```
9. This should produce `quakepsx.bin` and `quakepsx.cue` in the `build` directory.

### Credits
* id Software for the [original Quake](https://github.com/id-Software/Quake);
* Lameguy64, spicyjpeg and contributors for [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK) and [mkpsxiso](https://github.com/Lameguy64/mkpsxiso);
* [PCSX-Redux authors](https://github.com/grumpycoders/pcsx-redux/blob/main/AUTHORS) for PCSX-Redux, which was used for testing and debugging;
* [DuckStation authors](https://github.com/stenzek/duckstation/blob/master/CONTRIBUTORS.md) for DuckStation, which was used for testing and debugging;
* Martin Korth for PSX-SPX and no$psx;
* [psxavenc authors](https://github.com/WonderfulToolchain/psxavenc) for libpsxav;
* Sean Barrett for [stb_image](https://github.com/nothings/stb);
* David Reid for [dr_wav](https://github.com/mackron/dr_libs/blob/master/dr_wav.h);
* PSX.Dev Discord server for help and advice;
* anyone else who helped with this project that I might be forgetting.
