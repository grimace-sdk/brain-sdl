#include "SDL_internal.h"

#include "SDL_brainvideo.h"
#include "SDL_brain.h"
#include "video/SDL_pixels_c.h"
#include "events/SDL_keyboard_c.h"
#include "events/SDL_mouse_c.h"
#include "events/SDL_windowevents_c.h"

#define BRAIN_DRIVER_NAME "brain"
#define BRAIN_WINDOW_CLASS L"SDL_app"
#define BRAIN_TASKBAR_FULLSCREEN (WM_USER + 60)

static void SetTaskbarHidden(HWND hwnd, bool hidden)
{
    HWND taskbar;

    if (!SDL_GetHintBoolean(SDL_HINT_BRAIN_HIDE_TASKBAR, true)) {
        return;
    }
    taskbar = FindWindowW(L"HHTaskBar", NULL);
    if (taskbar) {
        SendMessageW(taskbar, BRAIN_TASKBAR_FULLSCREEN, hidden ? TRUE : FALSE, (LPARAM)hwnd);
    }
}

static void ClampWindowSize(SDL_VideoData *video, int *w, int *h)
{
    *w = SDL_clamp(*w, 8, video->screen_width);
    *h = SDL_clamp(*h, 8, video->screen_height);
}

static void LoadHintsFile(void)
{
    FILE *file = fopen("sdl-hints.txt", "r");
    char line[256];

    if (!file) {
        return;
    }
    while (fgets(line, sizeof(line), file)) {
        char *value = SDL_strchr(line, '=');
        char *end;
        if (!value || line[0] == '#') {
            continue;
        }
        *value++ = '\0';
        end = value + SDL_strlen(value);
        while (end > value && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ')) {
            *--end = '\0';
        }
        SDL_SetHintWithPriority(line, value, SDL_HINT_OVERRIDE);
        SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "brain: hint %s=%s", line, value);
    }
    fclose(file);
}

static bool BRAIN_VideoInit(SDL_VideoDevice *_this)
{
    SDL_VideoData *video = _this->internal;
    SDL_DisplayMode mode;
    WNDCLASSW window_class;

    LoadHintsFile();
    video->instance = GetModuleHandleW(NULL);
    video->start_ticks = GetTickCount();
    video->quit_after = (DWORD)SDL_atoi(SDL_GetHint(SDL_HINT_BRAIN_QUIT_AFTER_MS) ? SDL_GetHint(SDL_HINT_BRAIN_QUIT_AFTER_MS) : "0");
    video->log_fps = SDL_GetHintBoolean(SDL_HINT_BRAIN_LOG_FPS, false);
    video->fps_start = video->start_ticks;
    video->screen_width = GetSystemMetrics(SM_CXSCREEN);
    video->screen_height = GetSystemMetrics(SM_CYSCREEN);

    SDL_zero(window_class);
    window_class.lpfnWndProc = BRAIN_WindowProc;
    window_class.hInstance = video->instance;
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszClassName = BRAIN_WINDOW_CLASS;
    if (!RegisterClassW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return WINCE_SetError("RegisterClassW()");
    }

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = video->screen_width;
    mode.h = video->screen_height;
    mode.refresh_rate = 60.0f;
    if (SDL_AddBasicVideoDisplay(&mode) == 0) {
        return false;
    }

    video->user_activity = CreateEventW(NULL, FALSE, FALSE, L"PowerManager/ActivityTimer/EdUserActivity");
    {
        MEMORYSTATUS memory;
        memory.dwLength = sizeof(memory);
        GlobalMemoryStatus(&memory);
        SDL_Log("brain: %dx%d, memory %lu KiB free of %lu KiB, virtual %lu KiB free", video->screen_width, video->screen_height,
                (unsigned long)(memory.dwAvailPhys / 1024), (unsigned long)(memory.dwTotalPhys / 1024),
                (unsigned long)(memory.dwAvailVirtual / 1024));
    }
    return true;
}

static void BRAIN_VideoQuit(SDL_VideoDevice *_this)
{
    SDL_VideoData *video = _this->internal;

    if (video->user_activity) {
        CloseHandle(video->user_activity);
        video->user_activity = NULL;
    }
    UnregisterClassW(BRAIN_WINDOW_CLASS, video->instance);
}

static bool BRAIN_CreateWindow(SDL_VideoDevice *_this, SDL_Window *window, SDL_PropertiesID create_props)
{
    SDL_VideoData *video = _this->internal;
    SDL_WindowData *data;

    data = (SDL_WindowData *)SDL_calloc(1, sizeof(*data));
    if (!data) {
        return false;
    }
    data->window = window;

    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        window->w = video->screen_width;
        window->h = video->screen_height;
    } else {
        ClampWindowSize(video, &window->w, &window->h);
    }
    window->x = (video->screen_width - window->w) / 2;
    window->y = (video->screen_height - window->h) / 2;

    data->hwnd = CreateWindowW(BRAIN_WINDOW_CLASS, L"SDL", WS_POPUP, 0, 0, video->screen_width, video->screen_height,
                               NULL, NULL, video->instance, NULL);
    if (!data->hwnd) {
        SDL_free(data);
        return WINCE_SetError("CreateWindowW()");
    }
    data->dc = GetDC(NULL);
    window->internal = data;
    SetWindowLongW(data->hwnd, GWL_USERDATA, (LONG)window);

    return true;
}

static void BRAIN_SetWindowTitle(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *data = window->internal;
    WCHAR *title = WINCE_UTF8ToWide(window->title ? window->title : "");

    if (title) {
        SetWindowTextW(data->hwnd, title);
        SDL_free(title);
    }
}

static void BRAIN_ShowWindow(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *data = window->internal;

    ShowWindow(data->hwnd, SW_SHOW);
    SetForegroundWindow(data->hwnd);
    SetFocus(data->hwnd);
    SetTaskbarHidden(data->hwnd, true);
    BRAIN_SetActive(window, true);
}

static void BRAIN_HideWindow(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *data = window->internal;

    BRAIN_SuspendPresent(window);
    SetTaskbarHidden(data->hwnd, false);
    ShowWindow(data->hwnd, SW_HIDE);
}

static void BRAIN_RaiseWindow(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *data = window->internal;

    SetForegroundWindow(data->hwnd);
    SetFocus(data->hwnd);
}

static void ResizeWindow(SDL_VideoDevice *_this, SDL_Window *window, int w, int h)
{
    SDL_VideoData *video = _this->internal;

    ClampWindowSize(video, &w, &h);
    SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_MOVED, (video->screen_width - w) / 2, (video->screen_height - h) / 2);
    SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_RESIZED, w, h);
}

static void BRAIN_SetWindowSize(SDL_VideoDevice *_this, SDL_Window *window)
{
    ResizeWindow(_this, window, window->pending.w, window->pending.h);
}

static SDL_FullscreenResult BRAIN_SetWindowFullscreen(SDL_VideoDevice *_this, SDL_Window *window, SDL_VideoDisplay *display, SDL_FullscreenOp fullscreen)
{
    SDL_VideoData *video = _this->internal;

    if (fullscreen) {
        ResizeWindow(_this, window, video->screen_width, video->screen_height);
    } else {
        ResizeWindow(_this, window, window->windowed.w, window->windowed.h);
    }
    return SDL_FULLSCREEN_SUCCEEDED;
}

static void BRAIN_DestroyWindow(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *data = window->internal;

    if (!data) {
        return;
    }
    BRAIN_DestroyWindowFramebuffer(_this, window);
    SetTaskbarHidden(data->hwnd, false);
    SetWindowLongW(data->hwnd, GWL_USERDATA, 0);
    DestroyWindow(data->hwnd);
    ReleaseDC(NULL, data->dc);
    InvalidateRect(GetDesktopWindow(), NULL, TRUE);
    SDL_free(data);
    window->internal = NULL;
}

bool BRAIN_SuspendScreenSaver(SDL_VideoDevice *_this)
{
    SDL_VideoData *video = _this->internal;

    video->last_activity = GetTickCount() - 1000;
    return true;
}

static bool BRAIN_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonID)
{
    SDL_VideoDevice *device = SDL_GetVideoDevice();
    SDL_Window *presenter = (device && device->internal) ? ((SDL_VideoData *)device->internal)->presenter : NULL;
    WCHAR *title = WINCE_UTF8ToWide(messageboxdata->title ? messageboxdata->title : "");
    WCHAR *message = WINCE_UTF8ToWide(messageboxdata->message ? messageboxdata->message : "");
    UINT style = MB_OK | MB_SETFOREGROUND | MB_TOPMOST;
    int answer;

    if (messageboxdata->flags & SDL_MESSAGEBOX_ERROR) {
        style |= MB_ICONERROR;
    } else if (messageboxdata->flags & SDL_MESSAGEBOX_WARNING) {
        style |= MB_ICONWARNING;
    } else {
        style |= MB_ICONINFORMATION;
    }
    if (messageboxdata->numbuttons >= 2) {
        style = (style & ~MB_OK) | MB_OKCANCEL;
    }

    if (presenter) {
        BRAIN_SuspendPresent(presenter);
    }
    answer = MessageBoxW(NULL, message ? message : L"", title ? title : L"", style);
    if (presenter) {
        BRAIN_ResumePresent(presenter);
    }

    SDL_free(title);
    SDL_free(message);

    if (buttonID) {
        *buttonID = -1;
        if (messageboxdata->numbuttons > 0) {
            int index = (answer == IDCANCEL && messageboxdata->numbuttons >= 2) ? 1 : 0;
            *buttonID = messageboxdata->buttons[index].buttonID;
        }
    }
    return answer != 0;
}

static void BRAIN_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device->internal);
    SDL_free(device);
}

static SDL_VideoDevice *BRAIN_CreateDevice(void)
{
    SDL_VideoDevice *device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        return NULL;
    }
    device->internal = (SDL_VideoData *)SDL_calloc(1, sizeof(SDL_VideoData));
    if (!device->internal) {
        SDL_free(device);
        return NULL;
    }

    device->VideoInit = BRAIN_VideoInit;
    device->VideoQuit = BRAIN_VideoQuit;
    device->CreateSDLWindow = BRAIN_CreateWindow;
    device->SetWindowTitle = BRAIN_SetWindowTitle;
    device->SetWindowSize = BRAIN_SetWindowSize;
    device->ShowWindow = BRAIN_ShowWindow;
    device->HideWindow = BRAIN_HideWindow;
    device->RaiseWindow = BRAIN_RaiseWindow;
    device->SetWindowFullscreen = BRAIN_SetWindowFullscreen;
    device->DestroyWindow = BRAIN_DestroyWindow;
    device->CreateWindowFramebuffer = BRAIN_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = BRAIN_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = BRAIN_DestroyWindowFramebuffer;
    device->SetWindowFramebufferVSync = BRAIN_SetWindowFramebufferVSync;
    device->GetWindowFramebufferVSync = BRAIN_GetWindowFramebufferVSync;
    device->PumpEvents = BRAIN_PumpEvents;
    device->SuspendScreenSaver = BRAIN_SuspendScreenSaver;
    device->free = BRAIN_DeleteDevice;
    return device;
}

VideoBootStrap PRIVATE_bootstrap = {
    BRAIN_DRIVER_NAME, "SDL Sharp Brain present-mode video driver",
    BRAIN_CreateDevice,
    BRAIN_ShowMessageBox,
    false
};
