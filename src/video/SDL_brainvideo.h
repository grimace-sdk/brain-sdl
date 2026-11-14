#ifndef SDL_brainvideo_h_
#define SDL_brainvideo_h_

#include "SDL_internal.h"

#include "../core/SDL_wince.h"
#include "video/SDL_sysvideo.h"
#include "brain_present.h"

typedef struct SDL_VideoData
{
    HINSTANCE instance;
    int screen_width;
    int screen_height;
    SDL_Window *presenter;
    HANDLE user_activity;
    DWORD last_activity;
    DWORD start_ticks;
    DWORD quit_after;
    bool quit_sent;
    bool log_fps;
    DWORD fps_start;
    int fps_frames;
} SDL_VideoData;

struct SDL_WindowData
{
    SDL_Window *window;
    HWND hwnd;
    HDC dc;
    bool active;
    bool has_framebuffer;
    bool presenting;
    BrainPresentInfo present;
    DWORD buffer;
    int frame_width;
    int frame_height;
    int frame_x;
    int frame_y;
    int vsync;
    Uint16 *shadow;
    int shadow_pitch;
};

extern LRESULT CALLBACK BRAIN_WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
extern void BRAIN_PumpEvents(SDL_VideoDevice *_this);
extern bool BRAIN_SuspendScreenSaver(SDL_VideoDevice *_this);

extern bool BRAIN_CreateWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window, SDL_PixelFormat *format, void **pixels, int *pitch);
extern bool BRAIN_UpdateWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window, const SDL_Rect *rects, int numrects);
extern void BRAIN_DestroyWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window);
extern bool BRAIN_SetWindowFramebufferVSync(SDL_VideoDevice *_this, SDL_Window *window, int vsync);
extern bool BRAIN_GetWindowFramebufferVSync(SDL_VideoDevice *_this, SDL_Window *window, int *vsync);
extern void BRAIN_SuspendPresent(SDL_Window *window);
extern void BRAIN_ResumePresent(SDL_Window *window);
extern void BRAIN_RepaintFallback(SDL_Window *window);
extern void BRAIN_SetActive(SDL_Window *window, bool active);

#endif
