#include "SDL_internal.h"

#include "../core/SDL_wince.h"

struct SDL_RWLock
{
    CRITICAL_SECTION section;
};

SDL_RWLock *SDL_CreateRWLock(void)
{
    SDL_RWLock *rwlock = (SDL_RWLock *)SDL_malloc(sizeof(*rwlock));
    if (rwlock) {
        InitializeCriticalSection(&rwlock->section);
    }
    return rwlock;
}

void SDL_DestroyRWLock(SDL_RWLock *rwlock)
{
    if (rwlock) {
        DeleteCriticalSection(&rwlock->section);
        SDL_free(rwlock);
    }
}

void SDL_LockRWLockForReading(SDL_RWLock *rwlock) SDL_NO_THREAD_SAFETY_ANALYSIS
{
    if (rwlock) {
        EnterCriticalSection(&rwlock->section);
    }
}

void SDL_LockRWLockForWriting(SDL_RWLock *rwlock) SDL_NO_THREAD_SAFETY_ANALYSIS
{
    if (rwlock) {
        EnterCriticalSection(&rwlock->section);
    }
}

bool SDL_TryLockRWLockForReading(SDL_RWLock *rwlock)
{
    if (rwlock) {
        return TryEnterCriticalSection(&rwlock->section) != 0;
    }
    return true;
}

bool SDL_TryLockRWLockForWriting(SDL_RWLock *rwlock)
{
    if (rwlock) {
        return TryEnterCriticalSection(&rwlock->section) != 0;
    }
    return true;
}

void SDL_UnlockRWLock(SDL_RWLock *rwlock) SDL_NO_THREAD_SAFETY_ANALYSIS
{
    if (rwlock) {
        LeaveCriticalSection(&rwlock->section);
    }
}
