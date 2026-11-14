#include "SDL_internal.h"

#include "../core/SDL_wince.h"

SDL_SharedObject *SDL_LoadObject(const char *sofile)
{
    WCHAR *wide;
    HMODULE handle;

    if (!sofile) {
        SDL_InvalidParamError("sofile");
        return NULL;
    }

    wide = WINCE_UTF8ToWide(sofile);
    handle = wide ? LoadLibraryW(wide) : NULL;
    SDL_free(wide);
    if (!handle) {
        char message[512];
        SDL_snprintf(message, sizeof(message), "Failed loading %s", sofile);
        WINCE_SetError(message);
    }
    return (SDL_SharedObject *)handle;
}

SDL_FunctionPointer SDL_LoadFunction(SDL_SharedObject *handle, const char *name)
{
    SDL_FunctionPointer symbol = (SDL_FunctionPointer)GetProcAddressA((HMODULE)handle, name);
    if (!symbol) {
        char message[512];
        SDL_snprintf(message, sizeof(message), "Failed loading %s", name);
        WINCE_SetError(message);
    }
    return symbol;
}

void SDL_UnloadObject(SDL_SharedObject *handle)
{
    if (handle) {
        FreeLibrary((HMODULE)handle);
    }
}
