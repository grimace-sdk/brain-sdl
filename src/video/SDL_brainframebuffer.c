#include "SDL_internal.h"

#include "SDL_brainvideo.h"
#include "SDL_brain.h"

typedef struct ShadowBitmapInfo
{
    BITMAPINFOHEADER header;
    DWORD masks[3];
} ShadowBitmapInfo;

static SDL_VideoData *VideoData(void)
{
    return SDL_GetVideoDevice()->internal;
}

static Uint16 *PresentPixels(SDL_WindowData *data, DWORD buffer)
{
    return data->present.pBuffers[buffer];
}

static void CopyRows(Uint16 *destination, int destination_pitch, const Uint16 *source, int source_pitch, int x, int y, int w, int h)
{
    const Uint8 *from = (const Uint8 *)source + (size_t)y * source_pitch + (size_t)x * 2;
    Uint8 *to = (Uint8 *)destination + (size_t)y * destination_pitch + (size_t)x * 2;

    while (h-- > 0) {
        SDL_memcpy(to, from, (size_t)w * 2);
        from += source_pitch;
        to += destination_pitch;
    }
}

static void ClearPresentBuffers(SDL_WindowData *data)
{
    DWORD buffer;

    for (buffer = 0; buffer < BRAIN_PRESENT_MAX_BUFFERS; buffer++) {
        if (data->present.pBuffers[buffer]) {
            SDL_memset(data->present.pBuffers[buffer], 0, (size_t)data->present.Stride * data->present.Height);
        }
    }
}

static void ClosePresent(SDL_WindowData *data)
{
    SDL_VideoData *video = VideoData();

    if (data->presenting) {
        ExtEscape(data->dc, BRAIN_PRESENT_CLOSE, 0, NULL, 0, NULL);
        data->presenting = false;
        SDL_zero(data->present);
    }
    if (video->presenter == data->window) {
        video->presenter = NULL;
    }
}

static bool OpenPresent(SDL_WindowData *data)
{
    SDL_VideoData *video = VideoData();
    BrainPresentOpen request;
    int result;

    if (video->presenter && video->presenter != data->window) {
        return false;
    }

    SDL_zero(request);
    request.Width = (DWORD)data->frame_width;
    request.Height = (DWORD)data->frame_height;
    request.Scale = 1;
    request.Buffers = 2;
    request.Background = 0;
    request.Flags = SDL_GetHintBoolean(SDL_HINT_BRAIN_FAST_CPU, true) ? BRAIN_PRESENT_FAST_CPU : 0;

    SDL_zero(data->present);
    result = ExtEscape(data->dc, BRAIN_PRESENT_OPEN, (int)sizeof(request), (LPCSTR)&request, (int)sizeof(data->present), (LPSTR)&data->present);
    if (result <= 0) {
        DWORD error = GetLastError();
        DWORD escape = BRAIN_PRESENT_OPEN;
        int supported = ExtEscape(data->dc, QUERYESCSUPPORT, (int)sizeof(escape), (LPCSTR)&escape, 0, NULL);
        SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "brain: present open %dx%d failed: result %d error %lu supported %d",
                     data->frame_width, data->frame_height, result, (unsigned long)error, supported);
        SDL_zero(data->present);
        return false;
    }
    if (!data->present.pBuffers[0] || !data->present.pBuffers[1] || data->present.Stride < (DWORD)data->frame_width * 2) {
        ExtEscape(data->dc, BRAIN_PRESENT_CLOSE, 0, NULL, 0, NULL);
        SDL_zero(data->present);
        return false;
    }

    data->presenting = true;
    data->buffer = 0;
    video->presenter = data->window;
    ClearPresentBuffers(data);
    SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "brain: present %dx%d frame %dx%d stride %lu", data->window->w, data->window->h,
                 data->frame_width, data->frame_height, (unsigned long)data->present.Stride);
    return true;
}

static void SetSurfacePixels(SDL_Window *window, void *pixels, int pitch)
{
    if (window->surface) {
        window->surface->pixels = pixels;
        window->surface->pitch = pitch;
    }
}

void BRAIN_RepaintFallback(SDL_Window *window)
{
    SDL_WindowData *data = window->internal;
    ShadowBitmapInfo info;
    HDC dc;

    if (!data->has_framebuffer || data->presenting || !data->active || !data->shadow) {
        return;
    }

    SDL_zero(info);
    info.header.biSize = sizeof(BITMAPINFOHEADER);
    info.header.biWidth = window->w;
    info.header.biHeight = -window->h;
    info.header.biPlanes = 1;
    info.header.biBitCount = 16;
    info.header.biCompression = BI_BITFIELDS;
    info.header.biSizeImage = (DWORD)data->shadow_pitch * (DWORD)window->h;
    info.masks[0] = 0xF800;
    info.masks[1] = 0x07E0;
    info.masks[2] = 0x001F;

    dc = GetDC(data->hwnd);
    SetDIBitsToDevice(dc, data->frame_x, data->frame_y, (DWORD)window->w, (DWORD)window->h, 0, 0, 0, (UINT)window->h,
                      data->shadow, (const BITMAPINFO *)&info, DIB_RGB_COLORS);
    ReleaseDC(data->hwnd, dc);
}

bool BRAIN_CreateWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window, SDL_PixelFormat *format, void **pixels, int *pitch)
{
    SDL_VideoData *video = _this->internal;
    SDL_WindowData *data = window->internal;
    int w = window->w;
    int h = window->h;

    BRAIN_DestroyWindowFramebuffer(_this, window);

    data->frame_width = SDL_min((w + 7) & ~7, video->screen_width & ~7);
    data->frame_height = SDL_min((h + 7) & ~7, video->screen_height & ~7);
    data->frame_x = (video->screen_width - data->frame_width) / 2;
    data->frame_y = (video->screen_height - data->frame_height) / 2;

    data->shadow_pitch = (w * 2 + 3) & ~3;
    data->shadow = (Uint16 *)SDL_calloc((size_t)h, (size_t)data->shadow_pitch);
    if (!data->shadow) {
        return false;
    }
    data->has_framebuffer = true;

    *format = SDL_PIXELFORMAT_RGB565;
    if (data->active && OpenPresent(data)) {
        *pixels = PresentPixels(data, data->buffer);
        *pitch = (int)data->present.Stride;
    } else {
        *pixels = data->shadow;
        *pitch = data->shadow_pitch;
        SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "brain: GDI %dx%d (%s)", w, h, data->active ? "present unavailable" : "inactive");
    }
    return true;
}

static bool KeepsContents(SDL_Window *window)
{
    if (SDL_GetHintBoolean(SDL_HINT_BRAIN_FRAMEBUFFER_DISCARD, false)) {
        return false;
    }
    return SDL_GetRenderer(window) == NULL;
}

static bool WaitBuffer(SDL_WindowData *data, DWORD buffer)
{
    return ExtEscape(data->dc, BRAIN_PRESENT_WAIT, (int)sizeof(buffer), (LPCSTR)&buffer, 0, NULL) > 0;
}

static void CountFrame(SDL_VideoData *video, bool presenting)
{
    DWORD now;

    if (!video->log_fps) {
        return;
    }
    video->fps_frames++;
    now = GetTickCount();
    if (now - video->fps_start >= 5000) {
        MEMORYSTATUS memory;
        memory.dwLength = sizeof(memory);
        GlobalMemoryStatus(&memory);
        SDL_Log("brain: %.1f fps (%s), %lu KiB free", (double)video->fps_frames * 1000.0 / (double)(now - video->fps_start), presenting ? "present" : "GDI",
                (unsigned long)(memory.dwAvailPhys / 1024));
        video->fps_frames = 0;
        video->fps_start = now;
    }
}

bool BRAIN_UpdateWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window, const SDL_Rect *rects, int numrects)
{
    SDL_WindowData *data = window->internal;

    CountFrame(_this->internal, data->presenting);
    DWORD current;
    DWORD next;
    int index;

    if (!data->presenting) {
        BRAIN_RepaintFallback(window);
        return true;
    }

    current = data->buffer;
    next = current ^ 1;
    if ((data->vsync && !WaitBuffer(data, next)) ||
        ExtEscape(data->dc, BRAIN_PRESENT, (int)sizeof(current), (LPCSTR)&current, 0, NULL) <= 0 ||
        !WaitBuffer(data, next)) {
        BRAIN_SuspendPresent(window);
        BRAIN_RepaintFallback(window);
        return true;
    }

    if (KeepsContents(window)) {
        for (index = 0; index < numrects; index++) {
            SDL_Rect rect;
            SDL_Rect bounds = { 0, 0, window->w, window->h };
            if (SDL_GetRectIntersection(&rects[index], &bounds, &rect)) {
                CopyRows(PresentPixels(data, next), (int)data->present.Stride, PresentPixels(data, current), (int)data->present.Stride,
                         rect.x, rect.y, rect.w, rect.h);
            }
        }
    }

    data->buffer = next;
    SetSurfacePixels(window, PresentPixels(data, next), (int)data->present.Stride);
    return true;
}

void BRAIN_DestroyWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *data = window->internal;

    ClosePresent(data);
    SDL_free(data->shadow);
    data->shadow = NULL;
    data->has_framebuffer = false;
}

bool BRAIN_SetWindowFramebufferVSync(SDL_VideoDevice *_this, SDL_Window *window, int vsync)
{
    SDL_WindowData *data = window->internal;

    if (vsync < 0) {
        return SDL_Unsupported();
    }
    data->vsync = vsync;
    return true;
}

bool BRAIN_GetWindowFramebufferVSync(SDL_VideoDevice *_this, SDL_Window *window, int *vsync)
{
    SDL_WindowData *data = window->internal;

    *vsync = data->vsync;
    return true;
}

void BRAIN_SuspendPresent(SDL_Window *window)
{
    SDL_WindowData *data = window->internal;

    if (!data->presenting) {
        return;
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "brain: present suspended");
    CopyRows(data->shadow, data->shadow_pitch, PresentPixels(data, data->buffer), (int)data->present.Stride, 0, 0, window->w, window->h);
    ClosePresent(data);
    SetSurfacePixels(window, data->shadow, data->shadow_pitch);
}

void BRAIN_ResumePresent(SDL_Window *window)
{
    SDL_WindowData *data = window->internal;

    if (!data->has_framebuffer || data->presenting || !OpenPresent(data)) {
        BRAIN_RepaintFallback(window);
        return;
    }
    CopyRows(PresentPixels(data, 0), (int)data->present.Stride, data->shadow, data->shadow_pitch, 0, 0, window->w, window->h);
    CopyRows(PresentPixels(data, 1), (int)data->present.Stride, data->shadow, data->shadow_pitch, 0, 0, window->w, window->h);
    ExtEscape(data->dc, BRAIN_PRESENT, (int)sizeof(data->buffer), (LPCSTR)&data->buffer, 0, NULL);
    data->buffer = 1;
    SetSurfacePixels(window, PresentPixels(data, 1), (int)data->present.Stride);
}
