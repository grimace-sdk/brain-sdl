#include "SDL_internal.h"

#include "../core/SDL_wince.h"

Uint64 SDL_GetPerformanceCounter(void)
{
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return (Uint64)GetTickCount();
    }
    return (Uint64)counter.QuadPart;
}

Uint64 SDL_GetPerformanceFrequency(void)
{
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart == 0) {
        return 1000;
    }
    return (Uint64)frequency.QuadPart;
}

void SDL_SYS_DelayNS(Uint64 ns)
{
    Sleep((DWORD)SDL_NS_TO_MS(ns));
}
