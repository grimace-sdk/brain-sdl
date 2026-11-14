set(GRIMACE_DEFAULT_STACK_SIZE 0x100000)

grimace_add_executable(sdltest sdltest.c)
target_link_libraries(sdltest PRIVATE SDL3::SDL3)
target_compile_options(sdltest PRIVATE -O2 -Wall -Wno-unused-parameter)

grimace_add_executable(sdl2test sdl2test.c)
target_link_libraries(sdl2test PRIVATE SDL2::SDL2main SDL2::SDL2)
target_compile_options(sdl2test PRIVATE -O2 -Wall -Wno-unused-parameter)

grimace_add_executable(satellites satellites.c)
target_link_libraries(satellites PRIVATE SDL2::SDL2main SDL2_mixer::SDL2_mixer SDL2_image::SDL2_image SDL2::SDL2)
target_compile_options(satellites PRIVATE -O2 -Wall -Wno-unused-parameter)
