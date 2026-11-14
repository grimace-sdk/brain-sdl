#include "SDL_internal.h"

#include "../core/SDL_wince.h"

struct SDL_Mutex
{
    CRITICAL_SECTION section;
};

SDL_Mutex *SDL_CreateMutex(void)
{
    SDL_Mutex *mutex = (SDL_Mutex *)SDL_malloc(sizeof(*mutex));
    if (mutex) {
        InitializeCriticalSection(&mutex->section);
    }
    return mutex;
}

void SDL_DestroyMutex(SDL_Mutex *mutex)
{
    if (mutex) {
        DeleteCriticalSection(&mutex->section);
        SDL_free(mutex);
    }
}

void SDL_LockMutex(SDL_Mutex *mutex) SDL_NO_THREAD_SAFETY_ANALYSIS
{
    if (mutex) {
        EnterCriticalSection(&mutex->section);
    }
}

bool SDL_TryLockMutex(SDL_Mutex *mutex)
{
    if (mutex) {
        return TryEnterCriticalSection(&mutex->section) != 0;
    }
    return true;
}

void SDL_UnlockMutex(SDL_Mutex *mutex) SDL_NO_THREAD_SAFETY_ANALYSIS
{
    if (mutex) {
        LeaveCriticalSection(&mutex->section);
    }
}
