# brain-sdl

[SDL 3](https://libsdl.org), with sdl2-compat, SDL2_mixer and SDL2_image, for Windows CE 6.0 on the Sharp Brain (PW-SH2 and other i.MX28 models), built with [grimace](https://github.com/grimace-sdk/grimace).

Video goes through brain-display's present mode: SDL's window surface is the present buffer, so the software renderer draws straight into memory the driver sends to the panel on its tearing-effect pulse.

## Status

Runs on the PW-SH2:

- SDL3: `sdltest` renders at the panel rate, 59.3 Hz, with vsync. Keys, touch and waveOut audio work.
- SDL2 through sdl2-compat and `SDL3.dll`, with SDL2_mixer and SDL2_image: SDLPoP, Chocolate Doom, NXEngine, Rocks'n'Diamonds, ScummVM, OpenTyrian, TyrQuake, DevilutionX and Basilisk II run on the device.

## Layout

- `sources.json`: SDL 3.4.18, sdl2-compat 2.32.74, SDL2_mixer 2.8.2 and SDL2_image 2.8.12, pinned by checksum.
- `tools/`: `fetch-sources.sh [names]` fetches the sources into `sources/` and applies `patches/<name>`. `regen-patch.sh <name>` regenerates a source's patch from its edited tree. `gen-exports.sh` writes `config/SDL3.exports`.
- `patches/`: changes to the upstream sources (below).
- `config/`: `SDL_build_config.h` (the build configuration) and `SDL3.exports` (the DLL's exports).
- `include/`: the private-platform main headers (SDL provides `WinMain`) and `SDL_brain.h` (hints).
- `src/`:
  - CE threads (critical-section mutexes and read-write locks, kernel semaphores, TLS), timers, time, filesystem and loadso;
  - the entry point;
  - the `brain` video driver and the `waveout` audio driver;
  - `sdl2main`: `WinMain` for SDL2 programs, which loads SDL3 and calls the program's `main` (C or C++).
- `examples/`: `sdltest` (SDL3: renderer, input, audio, timing, `--bench`), `sdl2test` (SDL2 through sdl2-compat) and `satellites` (SDL2_mixer and SDL2_image).

### Patches

SDL:

- `0001`: a private thread backend hook in `SDL_thread_c.h`.
- `0002`: on a 16-bit target SDL stored alpha textures as RGB565, losing transparency and blending through the generic blitter. They fall back to ARGB8888.
- `0003`: RGB565 fast paths: RGB24/BGR24 to RGB565 conversion, and an exact 2x nearest stretch for 16-bit pixels (320x200 to 640x400).
- `0004`: blits from opaque surfaces with `SDL_BLENDMODE_BLEND` (as `IMG_Load` sets) and no alpha mod use the copy and colour-key blitters instead of the per-pixel alpha path.
- `0005`: audio devices open at the requested rate and channel count. SDL otherwise raises every device to at least 44.1 kHz stereo, which forces a float resample for 22.05 kHz games.

Everything else uses SDL's `SDL_PLATFORM_PRIVATE` hooks.

sdl2-compat:

- Loads `SDL3.dll` from the program's folder with `LoadLibraryW`/`GetProcAddressA`.
- Treats CE as a non-Windows platform in the SDL2 headers, and adds an SDL2 config for CE. No environment or dynamic API.
- Keeps the SDL2 window surface's pixel pointer in step with the SDL3 surface after each update, since the present buffers swap.
- Converts U8/S8/S16 audio in `SDL_ConvertAudio` (used by `Mix_LoadWAV`) with 16.16 integer interpolation, and mixes S16/U8/S8 in `SDL_MixAudioFormat` with integer volume, instead of SDL3's float paths.

SDL2_mixer:

- The stereo positional effects (`Mix_SetPanning`, `Mix_SetPosition`) use fixed-point gains instead of a float multiply per sample.

## Build

```sh
tools/fetch-sources.sh
cmake -S . -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$GRIMACE/cmake/toolchain.cmake" \
    -DGRIMACE_ARCH=arm -DGRIMACE_CE_VERSION=6 -DGRIMACE_PLATFORM=brain
cmake --build build
python3 "$GRIMACE/tests/check-imports.py" --arch arm --version 6 --platform brain build
```

Tested with grimace `712288e` and its LLVM release `v5`; CI (`.github/workflows/build.yml`) builds against the same. SDL is built for ARMv5TE (ARM926EJ-S).

| Target | Output | Use |
| --- | --- | --- |
| `SDL3::SDL3` | `libSDL3.a` | SDL3 programs, linked statically |
| `SDL3_dll` | `SDL3.dll` | loaded at run time by sdl2-compat; ship it next to SDL2 programs |
| `SDL2::SDL2`, `SDL2::SDL2main` | static | SDL2 programs (`SDL_MAIN_HANDLED` is defined; `SDL2main` provides `WinMain`) |
| `SDL2_mixer::SDL2_mixer` | static | WAV, OGG (stb_vorbis), FLAC, MP3, Timidity MIDI; no MOD |
| `SDL2_image::SDL2_image` | static | stb_image backend: PNG, JPG, BMP, GIF, TGA, and others |

Headers are available as `SDL.h` and `SDL2/SDL.h`, `SDL_mixer.h` and `SDL2/SDL_mixer.h`, and so on.

To use it from another project, `add_subdirectory` this folder after `include(Grimace)`.

## Platform notes

Video:

- One display, the screen size, RGB565. Windows keep their size, up to the screen, and are shown centred. The PXP cannot scale RGB, so a 400x240 window costs a quarter of a full-screen one. Fullscreen resizes to the screen.
- Present mode needs brain-display. Without it, or while another app is in front, frames go through `SetDIBitsToDevice`.
- The window surface is double buffered. Without a renderer, each update copies its rectangles into the next buffer so the surface keeps its contents. With a renderer, or with `SDL_BRAIN_FRAMEBUFFER_DISCARD=1`, it does not.
- Renderer vsync waits for the previous frame to reach the panel before presenting, so every frame is shown and rendering overlaps the panel refresh.
- A full-screen RGB565 clear costs about 7 ms, so games that redraw everything can skip `SDL_RenderClear`.

Audio:

- waveOut, 16-bit, at 11025, 22050 or 44100 Hz as requested (otherwise 44100), mono or stereo, four buffers of at least 2048 frames. Other rates are resampled by SDL in float.
- The audio thread runs one step above normal priority, so CE's round-robin time slices don't starve it. If it falls behind, it drops to normal priority until it catches up, so an overloaded game stutters instead of starving the device.

Input:

- Keys arrive as Windows virtual keys. The Brain's Shift key sends `VK_CAPITAL`; SDL reports it as Left Shift.
- With brainshell running, its low-level hook remaps keys first (`\Storage Card\brainshell\keys.txt`: Search is Escape, HOME is Backspace, Decide is Space, Back is Tab), and the launch keys open other apps. brainshell sends the Space key as `VK_LWIN`; SDL reports it as Space.
- Touch arrives as the left mouse button, with absolute coordinates.
- Joysticks, haptics and sensors have dummy drivers, so initialising them succeeds with no devices.

System:

- CE has no current directory. Relative paths resolve against the program's folder, and so do the base, current and pref paths (`<program folder>\<org>\<app>\`).
- While the screen saver is suspended (SDL's default with video), SDL resets the user activity timer every second.
- Not supported: camera, GPU, OpenGL, dialogs, trays, processes, recording.

## Hints and logging

Programs started through SDL's `WinMain` (SDL3 or SDL2) send `stdout` and `stderr` to `<program>.log` next to the program. SDL's log goes there and to the debugger.

`sdl-hints.txt` next to the program sets hints, one `NAME=VALUE` per line, as overrides.

| Hint | Default | Effect |
| --- | --- | --- |
| `SDL_BRAIN_FAST_CPU` | 1 | hold the CPU at 454 MHz while presenting |
| `SDL_BRAIN_HIDE_TASKBAR` | 1 | hide the taskbar while a window is shown |
| `SDL_BRAIN_FRAMEBUFFER_DISCARD` | 0 | don't carry window surface contents between updates |
| `SDL_BRAIN_QUIT_AFTER_MS` | 0 | quit after this many milliseconds, for timed runs |
| `SDL_BRAIN_LOG_FPS` | 0 | log the frame rate and free memory every 5 s; waveOut also logs buffers played, underruns and fill time |
| `SDL_BRAIN_LOG_KEYS` | 0 | log each key press (virtual key, scancode, time) |

## Porting notes

The ARM926 has no FPU, so floating point is software-emulated and slow:

- Prefer integer audio: WAV at 22050 or 44100 Hz, and OPL emulation such as DOSBox's dbopl. OGG Vorbis through stb_vorbis decodes in float and cannot keep up with 44.1 kHz stereo.
- Avoid SDL's float audio resampling: open the device at one of the waveOut rates.
- Unaligned 16- and 32-bit loads and stores crash. Watch for pointers to members of packed structs and casts of byte buffers to wider types; clang does not warn about these.
- Large stack arrays are a risk; prefer static or heap buffers.

## Licence

zlib, as SDL. See `LICENSE.txt`. SDL, sdl2-compat, SDL2_mixer and SDL2_image are under the zlib licence; `sources/` holds their own licence files after fetching.
