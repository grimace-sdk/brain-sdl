#ifndef SDL_main_impl_private_h_
#define SDL_main_impl_private_h_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

extern SDL_DECLSPEC int SDLCALL SDL_WinCE_RunApp(LPWSTR command_line, SDL_main_func main_function);

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR command_line, int show)
{
    (void)instance;
    (void)previous;
    (void)show;
    return SDL_WinCE_RunApp(command_line, SDL_main);
}

#ifdef __cplusplus
}
#endif

#endif
