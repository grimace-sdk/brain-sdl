#include "SDL_internal.h"

#include "../core/SDL_wince.h"

struct SDL_Semaphore
{
    HANDLE id;
    LONG count;
};

SDL_Semaphore *SDL_CreateSemaphore(Uint32 initial_value)
{
    SDL_Semaphore *sem = (SDL_Semaphore *)SDL_malloc(sizeof(*sem));
    if (sem) {
        sem->id = CreateSemaphoreW(NULL, (LONG)initial_value, 32 * 1024, NULL);
        sem->count = (LONG)initial_value;
        if (!sem->id) {
            SDL_SetError("Couldn't create semaphore");
            SDL_free(sem);
            sem = NULL;
        }
    }
    return sem;
}

void SDL_DestroySemaphore(SDL_Semaphore *sem)
{
    if (sem) {
        CloseHandle(sem->id);
        SDL_free(sem);
    }
}

bool SDL_WaitSemaphoreTimeoutNS(SDL_Semaphore *sem, Sint64 timeoutNS)
{
    DWORD milliseconds;

    if (!sem) {
        return true;
    }
    if (timeoutNS < 0) {
        milliseconds = INFINITE;
    } else {
        milliseconds = (DWORD)SDL_NS_TO_MS(timeoutNS);
    }
    if (WaitForSingleObject(sem->id, milliseconds) == WAIT_OBJECT_0) {
        InterlockedDecrement(&sem->count);
        return true;
    }
    return false;
}

Uint32 SDL_GetSemaphoreValue(SDL_Semaphore *sem)
{
    if (!sem) {
        return 0;
    }
    return (Uint32)sem->count;
}

void SDL_SignalSemaphore(SDL_Semaphore *sem)
{
    if (!sem) {
        return;
    }
    InterlockedIncrement(&sem->count);
    if (!ReleaseSemaphore(sem->id, 1, NULL)) {
        InterlockedDecrement(&sem->count);
    }
}
