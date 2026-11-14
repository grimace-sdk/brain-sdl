#include "SDL_internal.h"

#include "../core/SDL_wince.h"
#include "filesystem/SDL_sysfilesystem.h"

#include <shellapi.h>
#include <shlobj.h>

static char *DirectoryWithSeparator(const WCHAR *directory)
{
    char *utf8 = WINCE_WideToUTF8(directory);
    size_t length;
    char *result;

    if (!utf8) {
        return NULL;
    }
    length = SDL_strlen(utf8);
    if (length > 0 && utf8[length - 1] == '\\') {
        return utf8;
    }
    result = (char *)SDL_realloc(utf8, length + 2);
    if (!result) {
        SDL_free(utf8);
        return NULL;
    }
    result[length] = '\\';
    result[length + 1] = '\0';
    return result;
}

char *SDL_SYS_GetBasePath(void)
{
    return DirectoryWithSeparator(WINCE_GetProgramDirectory());
}

char *SDL_SYS_GetExeName(void)
{
    WCHAR path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    const WCHAR *name = path;
    DWORD index;

    if (length == 0) {
        WINCE_SetError("GetModuleFileNameW()");
        return NULL;
    }
    for (index = 0; index < length; index++) {
        if (path[index] == L'\\') {
            name = path + index + 1;
        }
    }
    return WINCE_WideToUTF8(name);
}

static bool CreateDirectoryIfMissing(const char *path)
{
    WCHAR *wide = WINCE_UTF8ToWide(path);
    DWORD attributes;
    bool result = true;

    if (!wide) {
        return false;
    }
    if (!CreateDirectoryW(wide, NULL)) {
        attributes = GetFileAttributesW(wide);
        if (attributes == 0xFFFFFFFF || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            result = WINCE_SetError("Couldn't create directory");
        }
    }
    SDL_free(wide);
    return result;
}

char *SDL_SYS_GetPrefPath(const char *org, const char *app)
{
    char *base = SDL_SYS_GetBasePath();
    char *path = NULL;

    if (!base) {
        return NULL;
    }
    if (org && *org) {
        if (SDL_asprintf(&path, "%s%s\\", base, org) < 0 || !CreateDirectoryIfMissing(path)) {
            SDL_free(path);
            SDL_free(base);
            return NULL;
        }
        SDL_free(base);
        base = path;
    }
    if (SDL_asprintf(&path, "%s%s\\", base, app) < 0) {
        path = NULL;
    } else if (!CreateDirectoryIfMissing(path)) {
        SDL_free(path);
        path = NULL;
    }
    SDL_free(base);
    return path;
}

char *SDL_SYS_GetUserFolder(SDL_Folder folder)
{
    WCHAR path[MAX_PATH];
    int csidl;

    switch (folder) {
    case SDL_FOLDER_HOME:
        return SDL_strdup("\\");
    case SDL_FOLDER_DESKTOP:
        csidl = CSIDL_DESKTOPDIRECTORY;
        break;
    case SDL_FOLDER_DOCUMENTS:
        csidl = CSIDL_PERSONAL;
        break;
    case SDL_FOLDER_TEMPLATES:
        csidl = CSIDL_TEMPLATES;
        break;
    default:
        SDL_SetError("Unsupported folder on Windows CE");
        return NULL;
    }

    if (!SHGetSpecialFolderPath(NULL, path, csidl, FALSE)) {
        SDL_SetError("Couldn't find folder");
        return NULL;
    }
    return DirectoryWithSeparator(path);
}

char *SDL_SYS_GetCurrentDirectory(void)
{
    return SDL_SYS_GetBasePath();
}
