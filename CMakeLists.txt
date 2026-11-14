cmake_minimum_required(VERSION 3.20)
project(brain_sdl LANGUAGES C)

include(Grimace)

set(SDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sources/SDL")
if(NOT EXISTS "${SDL_DIR}/include/SDL3/SDL.h")
    message(FATAL_ERROR "SDL sources missing: run tools/fetch-sources.sh")
endif()

set(BRAIN_SDL_CPU_FLAGS -march=armv5te -mtune=arm926ej-s)

file(GLOB SDL_UPSTREAM_SOURCES
    "${SDL_DIR}/src/*.c"
    "${SDL_DIR}/src/atomic/*.c"
    "${SDL_DIR}/src/audio/*.c"
    "${SDL_DIR}/src/audio/dummy/*.c"
    "${SDL_DIR}/src/camera/dummy/*.c"
    "${SDL_DIR}/src/haptic/dummy/*.c"
    "${SDL_DIR}/src/joystick/dummy/*.c"
    "${SDL_DIR}/src/joystick/virtual/*.c"
    "${SDL_DIR}/src/sensor/dummy/*.c"
    "${SDL_DIR}/src/camera/*.c"
    "${SDL_DIR}/src/core/*.c"
    "${SDL_DIR}/src/cpuinfo/*.c"
    "${SDL_DIR}/src/dynapi/*.c"
    "${SDL_DIR}/src/events/*.c"
    "${SDL_DIR}/src/filesystem/*.c"
    "${SDL_DIR}/src/gpu/*.c"
    "${SDL_DIR}/src/haptic/*.c"
    "${SDL_DIR}/src/hidapi/*.c"
    "${SDL_DIR}/src/io/*.c"
    "${SDL_DIR}/src/io/generic/*.c"
    "${SDL_DIR}/src/joystick/*.c"
    "${SDL_DIR}/src/libm/*.c"
    "${SDL_DIR}/src/locale/*.c"
    "${SDL_DIR}/src/locale/dummy/*.c"
    "${SDL_DIR}/src/main/*.c"
    "${SDL_DIR}/src/main/generic/*.c"
    "${SDL_DIR}/src/misc/*.c"
    "${SDL_DIR}/src/misc/dummy/*.c"
    "${SDL_DIR}/src/power/*.c"
    "${SDL_DIR}/src/process/*.c"
    "${SDL_DIR}/src/process/dummy/*.c"
    "${SDL_DIR}/src/render/*.c"
    "${SDL_DIR}/src/render/*/*.c"
    "${SDL_DIR}/src/sensor/*.c"
    "${SDL_DIR}/src/stdlib/*.c"
    "${SDL_DIR}/src/storage/*.c"
    "${SDL_DIR}/src/storage/generic/*.c"
    "${SDL_DIR}/src/thread/*.c"
    "${SDL_DIR}/src/time/*.c"
    "${SDL_DIR}/src/timer/*.c"
    "${SDL_DIR}/src/tray/*.c"
    "${SDL_DIR}/src/tray/dummy/*.c"
    "${SDL_DIR}/src/video/*.c"
    "${SDL_DIR}/src/video/dummy/*.c"
    "${SDL_DIR}/src/video/yuv2rgb/*.c"
)
list(APPEND SDL_UPSTREAM_SOURCES
    "${SDL_DIR}/src/dialog/SDL_dialog.c"
    "${SDL_DIR}/src/dialog/SDL_dialog_utils.c"
    "${SDL_DIR}/src/dialog/dummy/SDL_dummydialog.c"
    "${SDL_DIR}/src/thread/generic/SDL_syscond.c"
)

set(BRAIN_SDL_SOURCES
    src/core/SDL_wince.c
    src/thread/SDL_systhread.c
    src/thread/SDL_systls.c
    src/thread/SDL_sysmutex.c
    src/thread/SDL_syssem.c
    src/thread/SDL_sysrwlock.c
    src/timer/SDL_systimer.c
    src/time/SDL_systime.c
    src/loadso/SDL_sysloadso.c
    src/filesystem/SDL_sysfilesystem.c
    src/filesystem/SDL_sysfsops.c
    src/main/SDL_wince_main.c
    src/video/SDL_brainvideo.c
    src/video/SDL_brainevents.c
    src/video/SDL_brainframebuffer.c
    src/audio/SDL_waveout.c
)

add_library(SDL3 STATIC ${SDL_UPSTREAM_SOURCES} ${BRAIN_SDL_SOURCES})
add_library(SDL3::SDL3 ALIAS SDL3)
target_include_directories(SDL3
    PUBLIC
        "${SDL_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/config"
        "${SDL_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/thread"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(SDL3 PUBLIC grimace::headers PRIVATE grimace::posix)
target_compile_definitions(SDL3 PRIVATE UNICODE _UNICODE)
target_compile_options(SDL3 PRIVATE ${BRAIN_SDL_CPU_FLAGS} -O2 -fno-strict-aliasing -Wno-unused-parameter -Wno-format -Wno-atomic-alignment)

grimace_add_library(SDL3_dll OUTPUT SDL3.dll EXPORTS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config/SDL3.exports"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/SDL_wince_dll.c")
target_link_libraries(SDL3_dll PRIVATE SDL3)
target_link_options(SDL3_dll PRIVATE --defsym=GRIMACE_IMPORT_RESERVE=0x400)

set(SDL2_COMPAT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sources/sdl2-compat")
add_library(SDL2 STATIC "${SDL2_COMPAT_DIR}/src/sdl2_compat.c" "${SDL2_COMPAT_DIR}/src/dynapi/SDL_dynapi.c")
add_library(SDL2::SDL2 ALIAS SDL2)
target_include_directories(SDL2
    PUBLIC
        "${SDL2_COMPAT_DIR}/include/SDL2"
        "${SDL2_COMPAT_DIR}/include"
    PRIVATE
        "${SDL2_COMPAT_DIR}/src"
        "${SDL_DIR}/include"
)
target_compile_definitions(SDL2 PUBLIC SDL_MAIN_HANDLED PRIVATE "SDL2COMPAT_REVISION=\"brain-sdl\"")
target_compile_options(SDL2 PRIVATE ${BRAIN_SDL_CPU_FLAGS} -O2 -Wno-unused-parameter -Wno-format)
target_link_libraries(SDL2 PUBLIC grimace::headers)

add_library(SDL2main STATIC src/sdl2main/SDL2main_wince.c)
add_library(SDL2::SDL2main ALIAS SDL2main)
target_link_libraries(SDL2main PUBLIC SDL2)

set(SDL2_MIXER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sources/SDL2_mixer")
file(GLOB SDL2_MIXER_SOURCES
    "${SDL2_MIXER_DIR}/src/*.c"
    "${SDL2_MIXER_DIR}/src/codecs/*.c"
    "${SDL2_MIXER_DIR}/src/codecs/timidity/*.c"
)
add_library(SDL2_mixer STATIC ${SDL2_MIXER_SOURCES})
add_library(SDL2_mixer::SDL2_mixer ALIAS SDL2_mixer)
configure_file("${SDL2_MIXER_DIR}/include/SDL_mixer.h" "${CMAKE_CURRENT_BINARY_DIR}/include/SDL2/SDL_mixer.h" COPYONLY)
target_include_directories(SDL2_mixer
    PUBLIC "${SDL2_MIXER_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
    PRIVATE "${SDL2_MIXER_DIR}/src" "${SDL2_MIXER_DIR}/src/codecs"
)
target_compile_definitions(SDL2_mixer PRIVATE
    MUSIC_WAV MUSIC_OGG OGG_USE_STB MUSIC_FLAC_DRFLAC MUSIC_MP3_MINIMP3 MUSIC_MID_TIMIDITY
)
target_compile_options(SDL2_mixer PRIVATE ${BRAIN_SDL_CPU_FLAGS} -O2 -w)
target_link_libraries(SDL2_mixer PUBLIC SDL2)

set(SDL2_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sources/SDL2_image")
file(GLOB SDL2_IMAGE_SOURCES "${SDL2_IMAGE_DIR}/src/*.c")
list(FILTER SDL2_IMAGE_SOURCES EXCLUDE REGEX "/IMG_(WIC)\\.c$")
add_library(SDL2_image STATIC ${SDL2_IMAGE_SOURCES})
add_library(SDL2_image::SDL2_image ALIAS SDL2_image)
configure_file("${SDL2_IMAGE_DIR}/include/SDL_image.h" "${CMAKE_CURRENT_BINARY_DIR}/include/SDL2/SDL_image.h" COPYONLY)
target_include_directories(SDL2_image PUBLIC "${SDL2_IMAGE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" PRIVATE "${SDL2_IMAGE_DIR}/src")
target_compile_definitions(SDL2_image PRIVATE
    USE_STBIMAGE LOAD_BMP LOAD_GIF LOAD_JPG LOAD_LBM LOAD_PCX LOAD_PNG LOAD_PNM LOAD_QOI LOAD_TGA LOAD_XCF LOAD_XPM LOAD_XV
    SDL_IMAGE_SAVE_PNG=1 SDL_IMAGE_SAVE_JPG=0
)
target_compile_options(SDL2_image PRIVATE ${BRAIN_SDL_CPU_FLAGS} -O2 -w)
target_link_libraries(SDL2_image PUBLIC SDL2)

option(BRAIN_SDL_EXAMPLES "Build the example programs" ON)
if(BRAIN_SDL_EXAMPLES)
    add_subdirectory(examples)
endif()
