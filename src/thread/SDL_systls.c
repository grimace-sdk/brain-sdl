#include "SDL_internal.h"

#include "../core/SDL_wince.h"
#include "thread/SDL_thread_c.h"

static DWORD thread_local_storage = TLS_OUT_OF_INDEXES;
static bool generic_local_storage = false;

void SDL_SYS_InitTLSData(void)
{
    if (thread_local_storage == TLS_OUT_OF_INDEXES && !generic_local_storage) {
        thread_local_storage = TlsAlloc();
        if (thread_local_storage == TLS_OUT_OF_INDEXES) {
            SDL_Generic_InitTLSData();
            generic_local_storage = true;
        }
    }
}

SDL_TLSData *SDL_SYS_GetTLSData(void)
{
    if (generic_local_storage) {
        return SDL_Generic_GetTLSData();
    }
    if (thread_local_storage != TLS_OUT_OF_INDEXES) {
        return (SDL_TLSData *)TlsGetValue(thread_local_storage);
    }
    return NULL;
}

bool SDL_SYS_SetTLSData(SDL_TLSData *data)
{
    if (generic_local_storage) {
        return SDL_Generic_SetTLSData(data);
    }
    if (!TlsSetValue(thread_local_storage, data)) {
        return WINCE_SetError("TlsSetValue()");
    }
    return true;
}

void SDL_SYS_QuitTLSData(void)
{
    if (generic_local_storage) {
        SDL_Generic_QuitTLSData();
        generic_local_storage = false;
    } else if (thread_local_storage != TLS_OUT_OF_INDEXES) {
        TlsFree(thread_local_storage);
        thread_local_storage = TLS_OUT_OF_INDEXES;
    }
}
