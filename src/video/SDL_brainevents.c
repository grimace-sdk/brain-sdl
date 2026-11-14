#include "SDL_internal.h"

#include "SDL_brain.h"
#include "SDL_brainvideo.h"
#include "events/SDL_events_c.h"
#include "events/SDL_keyboard_c.h"
#include "events/SDL_mouse_c.h"
#include "events/SDL_windowevents_c.h"

static WCHAR pending_high_surrogate;

static SDL_Scancode ScancodeFromVirtualKey(WPARAM key)
{
    if (key >= 'A' && key <= 'Z') {
        return (SDL_Scancode)(SDL_SCANCODE_A + (key - 'A'));
    }
    if (key >= '1' && key <= '9') {
        return (SDL_Scancode)(SDL_SCANCODE_1 + (key - '1'));
    }
    if (key >= VK_F1 && key <= VK_F12) {
        return (SDL_Scancode)(SDL_SCANCODE_F1 + (key - VK_F1));
    }
    if (key >= VK_F13 && key <= VK_F24) {
        return (SDL_Scancode)(SDL_SCANCODE_F13 + (key - VK_F13));
    }
    if (key >= VK_NUMPAD1 && key <= VK_NUMPAD9) {
        return (SDL_Scancode)(SDL_SCANCODE_KP_1 + (key - VK_NUMPAD1));
    }

    switch (key) {
    case '0':
        return SDL_SCANCODE_0;
    case VK_NUMPAD0:
        return SDL_SCANCODE_KP_0;
    case VK_BACK:
        return SDL_SCANCODE_BACKSPACE;
    case VK_TAB:
        return SDL_SCANCODE_TAB;
    case VK_RETURN:
        return SDL_SCANCODE_RETURN;
    case VK_SHIFT:
    case VK_LSHIFT:
        return SDL_SCANCODE_LSHIFT;
    case VK_RSHIFT:
        return SDL_SCANCODE_RSHIFT;
    case VK_CONTROL:
    case VK_LCONTROL:
        return SDL_SCANCODE_LCTRL;
    case VK_RCONTROL:
        return SDL_SCANCODE_RCTRL;
    case VK_MENU:
    case VK_LMENU:
        return SDL_SCANCODE_LALT;
    case VK_RMENU:
        return SDL_SCANCODE_RALT;
    case VK_PAUSE:
        return SDL_SCANCODE_PAUSE;
    case VK_CAPITAL:
        return SDL_SCANCODE_LSHIFT;
    case VK_ESCAPE:
        return SDL_SCANCODE_ESCAPE;
    case VK_SPACE:
        return SDL_SCANCODE_SPACE;
    case VK_PRIOR:
        return SDL_SCANCODE_PAGEUP;
    case VK_NEXT:
        return SDL_SCANCODE_PAGEDOWN;
    case VK_END:
        return SDL_SCANCODE_END;
    case VK_HOME:
        return SDL_SCANCODE_HOME;
    case VK_LEFT:
        return SDL_SCANCODE_LEFT;
    case VK_UP:
        return SDL_SCANCODE_UP;
    case VK_RIGHT:
        return SDL_SCANCODE_RIGHT;
    case VK_DOWN:
        return SDL_SCANCODE_DOWN;
    case VK_SNAPSHOT:
        return SDL_SCANCODE_PRINTSCREEN;
    case VK_INSERT:
        return SDL_SCANCODE_INSERT;
    case VK_DELETE:
        return SDL_SCANCODE_DELETE;
    case VK_LWIN:
        return SDL_SCANCODE_SPACE;
    case VK_RWIN:
        return SDL_SCANCODE_RGUI;
    case VK_APPS:
        return SDL_SCANCODE_APPLICATION;
    case VK_MULTIPLY:
        return SDL_SCANCODE_KP_MULTIPLY;
    case VK_ADD:
        return SDL_SCANCODE_KP_PLUS;
    case VK_SUBTRACT:
        return SDL_SCANCODE_KP_MINUS;
    case VK_DECIMAL:
        return SDL_SCANCODE_KP_PERIOD;
    case VK_DIVIDE:
        return SDL_SCANCODE_KP_DIVIDE;
    case VK_NUMLOCK:
        return SDL_SCANCODE_NUMLOCKCLEAR;
    case VK_SCROLL:
        return SDL_SCANCODE_SCROLLLOCK;
    case 0xBA:
        return SDL_SCANCODE_SEMICOLON;
    case 0xBB:
        return SDL_SCANCODE_EQUALS;
    case 0xBC:
        return SDL_SCANCODE_COMMA;
    case 0xBD:
        return SDL_SCANCODE_MINUS;
    case 0xBE:
        return SDL_SCANCODE_PERIOD;
    case 0xBF:
        return SDL_SCANCODE_SLASH;
    case 0xC0:
        return SDL_SCANCODE_GRAVE;
    case 0xDB:
        return SDL_SCANCODE_LEFTBRACKET;
    case 0xDC:
        return SDL_SCANCODE_BACKSLASH;
    case 0xDD:
        return SDL_SCANCODE_RIGHTBRACKET;
    case 0xDE:
        return SDL_SCANCODE_APOSTROPHE;
    default:
        return SDL_SCANCODE_UNKNOWN;
    }
}

static void SendCharacter(WCHAR character)
{
    char text[5];
    Uint32 codepoint;

    if (character >= 0xD800 && character <= 0xDBFF) {
        pending_high_surrogate = character;
        return;
    }
    if (character >= 0xDC00 && character <= 0xDFFF) {
        if (!pending_high_surrogate) {
            return;
        }
        codepoint = 0x10000 + (((Uint32)pending_high_surrogate - 0xD800) << 10) + ((Uint32)character - 0xDC00);
        pending_high_surrogate = 0;
    } else {
        codepoint = character;
        pending_high_surrogate = 0;
    }
    if (codepoint < 0x20 || codepoint == 0x7F) {
        return;
    }
    *SDL_UCS4ToUTF8(codepoint, text) = '\0';
    SDL_SendKeyboardText(text);
}

static void SendPointer(SDL_Window *window, LPARAM lparam)
{
    float x = (float)((short)LOWORD(lparam) - window->x);
    float y = (float)((short)HIWORD(lparam) - window->y);

    SDL_SendMouseMotion(0, window, SDL_DEFAULT_MOUSE_ID, false, x, y);
}

void BRAIN_SetActive(SDL_Window *window, bool active)
{
    SDL_WindowData *data = window->internal;

    if (data->active == active) {
        return;
    }
    data->active = active;
    if (active) {
        BRAIN_ResumePresent(window);
        SDL_SetKeyboardFocus(window);
        SDL_SetMouseFocus(window);
        SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_EXPOSED, 0, 0);
    } else {
        BRAIN_SuspendPresent(window);
        SDL_ResetKeyboard();
        SDL_SetKeyboardFocus(NULL);
        SDL_SetMouseFocus(NULL);
    }
}

LRESULT CALLBACK BRAIN_WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    SDL_Window *window = (SDL_Window *)GetWindowLongW(hwnd, GWL_USERDATA);

    if (!window || !window->internal) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    switch (message) {
    case WM_ACTIVATE:
        BRAIN_SetActive(window, LOWORD(wparam) != WA_INACTIVE);
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (SDL_GetHintBoolean(SDL_HINT_BRAIN_LOG_KEYS, false)) {
            SDL_Log("key: vk 0x%02x down, scancode %d, %u ms", (unsigned int)wparam, (int)ScancodeFromVirtualKey(wparam), (unsigned int)SDL_GetTicks());
        }
        SDL_SendKeyboardKey(0, SDL_DEFAULT_KEYBOARD_ID, (int)wparam, ScancodeFromVirtualKey(wparam), true);
        return 0;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        SDL_SendKeyboardKey(0, SDL_DEFAULT_KEYBOARD_ID, (int)wparam, ScancodeFromVirtualKey(wparam), false);
        return 0;

    case WM_CHAR:
        if (SDL_TextInputActive(window)) {
            SendCharacter((WCHAR)wparam);
        }
        return 0;

    case WM_MOUSEMOVE:
        SendPointer(window, lparam);
        return 0;

    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        SendPointer(window, lparam);
        SDL_SendMouseButton(0, window, SDL_DEFAULT_MOUSE_ID, SDL_BUTTON_LEFT, true);
        return 0;

    case WM_LBUTTONUP:
        SendPointer(window, lparam);
        SDL_SendMouseButton(0, window, SDL_DEFAULT_MOUSE_ID, SDL_BUTTON_LEFT, false);
        ReleaseCapture();
        return 0;

    case WM_RBUTTONDOWN:
        SendPointer(window, lparam);
        SDL_SendMouseButton(0, window, SDL_DEFAULT_MOUSE_ID, SDL_BUTTON_RIGHT, true);
        return 0;

    case WM_RBUTTONUP:
        SendPointer(window, lparam);
        SDL_SendMouseButton(0, window, SDL_DEFAULT_MOUSE_ID, SDL_BUTTON_RIGHT, false);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT paint;
        BeginPaint(hwnd, &paint);
        EndPaint(hwnd, &paint);
        BRAIN_RepaintFallback(window);
        return 0;
    }

    case WM_CLOSE:
        SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_CLOSE_REQUESTED, 0, 0);
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

void BRAIN_PumpEvents(SDL_VideoDevice *_this)
{
    SDL_VideoData *video = _this->internal;
    MSG message;

    while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            SDL_SendQuit();
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (video->quit_after && !video->quit_sent && GetTickCount() - video->start_ticks >= video->quit_after) {
        SDL_Log("brain: quitting after %lu ms", (unsigned long)video->quit_after);
        video->quit_sent = true;
        SDL_SendQuit();
    }

    if (_this->suspend_screensaver && video->user_activity && GetTickCount() - video->last_activity >= 1000) {
        SetEvent(video->user_activity);
        video->last_activity = GetTickCount();
    }
}
