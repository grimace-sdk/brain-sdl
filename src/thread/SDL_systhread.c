#include "SDL_internal.h"

#include "../core/SDL_wince.h"
#include "thread/SDL_thread_c.h"
#include "thread/SDL_systhread.h"

#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif

static DWORD WINAPI RunThread(LPVOID data)
{
    SDL_RunThread((SDL_Thread *)data);
    return 0;
}

bool SDL_SYS_CreateThread(SDL_Thread *thread, SDL_FunctionPointer pfnBeginThread, SDL_FunctionPointer pfnEndThread)
{
    DWORD flags = thread->stacksize ? STACK_SIZE_PARAM_IS_A_RESERVATION : 0;
    DWORD thread_id = 0;

    (void)pfnBeginThread;
    (void)pfnEndThread;

    thread->handle = CreateThread(NULL, (DWORD)thread->stacksize, RunThread, thread, flags, &thread_id);
    thread->threadid = (SDL_ThreadID)thread_id;
    if (!thread->handle) {
        return SDL_SetError("Not enough resources to create thread");
    }
    return true;
}

void SDL_SYS_SetupThread(const char *name)
{
    (void)name;
}

SDL_ThreadID SDL_GetCurrentThreadID(void)
{
    return (SDL_ThreadID)GetCurrentThreadId();
}

bool SDL_SYS_SetThreadPriority(SDL_ThreadPriority priority)
{
    int value;

    if (priority == SDL_THREAD_PRIORITY_LOW) {
        value = THREAD_PRIORITY_BELOW_NORMAL;
    } else if (priority == SDL_THREAD_PRIORITY_TIME_CRITICAL) {
        value = THREAD_PRIORITY_ABOVE_NORMAL;
    } else {
        value = THREAD_PRIORITY_NORMAL;
    }
    if (!SetThreadPriority(GetCurrentThread(), value)) {
        return WINCE_SetError("SetThreadPriority()");
    }
    return true;
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
}

void SDL_SYS_DetachThread(SDL_Thread *thread)
{
    CloseHandle(thread->handle);
}
