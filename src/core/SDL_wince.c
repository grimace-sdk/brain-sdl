#include "SDL_wince.h"

static WCHAR program_directory[MAX_PATH];

WCHAR *WINCE_UTF8ToWide(const char *text)
{
    return (WCHAR *)SDL_iconv_string("UTF-16LE", "UTF-8", text, SDL_strlen(text) + 1);
}

char *WINCE_WideToUTF8(const WCHAR *text)
{
    return SDL_iconv_string("UTF-8", "UTF-16LE", (const char *)text, (SDL_wcslen((const wchar_t *)text) + 1) * sizeof(WCHAR));
}

const WCHAR *WINCE_GetProgramDirectory(void)
{
    if (!program_directory[0]) {
        DWORD length = GetModuleFileNameW(NULL, program_directory, MAX_PATH);
        while (length > 0 && program_directory[length - 1] != L'\\') {
            length--;
        }
        program_directory[length] = 0;
    }
    return program_directory;
}

WCHAR *WINCE_ResolvePath(const char *path)
{
    WCHAR *wide = WINCE_UTF8ToWide(path);
    WCHAR *resolved;
    const WCHAR *base;
    const WCHAR *relative;
    size_t base_length;
    size_t relative_length;
    WCHAR *character;

    if (!wide) {
        return NULL;
    }
    for (character = wide; *character; character++) {
        if (*character == L'/') {
            *character = L'\\';
        }
    }
    if (wide[0] == L'\\') {
        return wide;
    }

    relative = wide;
    while (relative[0] == L'.' && relative[1] == L'\\') {
        relative += 2;
    }
    if (relative[0] == L'.' && relative[1] == 0) {
        relative++;
    }

    base = WINCE_GetProgramDirectory();
    base_length = SDL_wcslen((const wchar_t *)base);
    relative_length = SDL_wcslen((const wchar_t *)relative);
    resolved = (WCHAR *)SDL_malloc((base_length + relative_length + 1) * sizeof(WCHAR));
    if (resolved) {
        SDL_memcpy(resolved, base, base_length * sizeof(WCHAR));
        SDL_memcpy(resolved + base_length, relative, (relative_length + 1) * sizeof(WCHAR));
    }
    SDL_free(wide);
    return resolved;
}

bool WINCE_SetError(const char *prefix)
{
    WCHAR buffer[256];
    char *message;
    DWORD error = GetLastError();

    if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, 0, buffer, SDL_arraysize(buffer), NULL)) {
        return SDL_SetError("%s%serror %lu", prefix ? prefix : "", prefix ? ": " : "", (unsigned long)error);
    }
    message = WINCE_WideToUTF8(buffer);
    SDL_SetError("%s%s%s", prefix ? prefix : "", prefix ? ": " : "", message ? message : "");
    SDL_free(message);
    return false;
}
