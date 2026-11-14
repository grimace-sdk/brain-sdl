#include "SDL_internal.h"

#include "../core/SDL_wince.h"
#include "filesystem/SDL_sysfilesystem.h"

bool SDL_SYS_EnumerateDirectory(const char *path, SDL_EnumerateDirectoryCallback cb, void *userdata)
{
    SDL_EnumerationResult result = SDL_ENUM_CONTINUE;
    WIN32_FIND_DATAW entry;
    WCHAR *directory;
    WCHAR *pattern;
    char *dirname;
    size_t length;
    HANDLE find;

    directory = WINCE_ResolvePath(*path ? path : "\\");
    if (!directory) {
        return false;
    }
    length = SDL_wcslen((const wchar_t *)directory);
    while (length > 0 && directory[length - 1] == L'\\') {
        directory[--length] = 0;
    }

    pattern = (WCHAR *)SDL_malloc((length + 3) * sizeof(WCHAR));
    if (!pattern) {
        SDL_free(directory);
        return false;
    }
    SDL_memcpy(pattern, directory, length * sizeof(WCHAR));
    pattern[length] = L'\\';
    pattern[length + 1] = 0;
    dirname = WINCE_WideToUTF8(pattern);
    pattern[length + 1] = L'*';
    pattern[length + 2] = 0;
    SDL_free(directory);
    if (!dirname) {
        SDL_free(pattern);
        return false;
    }

    find = FindFirstFileW(pattern, &entry);
    SDL_free(pattern);
    if (find == INVALID_HANDLE_VALUE) {
        SDL_free(dirname);
        if (GetLastError() == ERROR_NO_MORE_FILES || GetLastError() == ERROR_FILE_NOT_FOUND) {
            return true;
        }
        return WINCE_SetError("Failed to enumerate directory");
    }

    do {
        const WCHAR *name = entry.cFileName;
        char *utf8;

        if (name[0] == L'.' && (name[1] == 0 || (name[1] == L'.' && name[2] == 0))) {
            continue;
        }
        utf8 = WINCE_WideToUTF8(name);
        if (!utf8) {
            result = SDL_ENUM_FAILURE;
        } else {
            result = cb(userdata, dirname, utf8);
            SDL_free(utf8);
        }
    } while (result == SDL_ENUM_CONTINUE && FindNextFileW(find, &entry));

    FindClose(find);
    SDL_free(dirname);
    return result != SDL_ENUM_FAILURE;
}

bool SDL_SYS_RemovePath(const char *path)
{
    WCHAR *wide = WINCE_ResolvePath(path);
    DWORD attributes;
    BOOL removed;

    if (!wide) {
        return false;
    }
    attributes = GetFileAttributesW(wide);
    if (attributes == 0xFFFFFFFF) {
        SDL_free(wide);
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return true;
        }
        return WINCE_SetError("Couldn't get path's attributes");
    }
    removed = (attributes & FILE_ATTRIBUTE_DIRECTORY) ? RemoveDirectoryW(wide) : DeleteFileW(wide);
    SDL_free(wide);
    if (!removed) {
        return WINCE_SetError("Couldn't remove path");
    }
    return true;
}

bool SDL_SYS_RenamePath(const char *oldpath, const char *newpath)
{
    WCHAR *old_wide = WINCE_ResolvePath(oldpath);
    WCHAR *new_wide = WINCE_ResolvePath(newpath);
    bool result = false;

    if (old_wide && new_wide) {
        DWORD attributes = GetFileAttributesW(new_wide);
        if (attributes != 0xFFFFFFFF && !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            DeleteFileW(new_wide);
        }
        result = MoveFileW(old_wide, new_wide) ? true : WINCE_SetError("Couldn't rename path");
    }
    SDL_free(old_wide);
    SDL_free(new_wide);
    return result;
}

bool SDL_SYS_CopyFile(const char *oldpath, const char *newpath)
{
    WCHAR *old_wide = WINCE_ResolvePath(oldpath);
    WCHAR *new_wide = WINCE_ResolvePath(newpath);
    bool result = false;

    if (old_wide && new_wide) {
        result = CopyFileW(old_wide, new_wide, FALSE) ? true : WINCE_SetError("Couldn't copy path");
    }
    SDL_free(old_wide);
    SDL_free(new_wide);
    return result;
}

bool SDL_SYS_CreateDirectory(const char *path)
{
    WCHAR *wide = WINCE_ResolvePath(path);
    bool result = true;

    if (!wide) {
        return false;
    }
    if (!CreateDirectoryW(wide, NULL)) {
        DWORD attributes = GetFileAttributesW(wide);
        if (attributes == 0xFFFFFFFF || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            result = WINCE_SetError("Couldn't create directory");
        }
    }
    SDL_free(wide);
    return result;
}

bool SDL_SYS_GetPathInfo(const char *path, SDL_PathInfo *info)
{
    WCHAR *wide = WINCE_ResolvePath(path);
    WIN32_FILE_ATTRIBUTE_DATA data;
    BOOL found;

    if (!wide) {
        return false;
    }
    found = GetFileAttributesExW(wide, GetFileExInfoStandard, &data);
    SDL_free(wide);
    if (!found) {
        return WINCE_SetError("Can't stat");
    }

    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        info->type = SDL_PATHTYPE_DIRECTORY;
        info->size = 0;
    } else {
        info->type = SDL_PATHTYPE_FILE;
        info->size = (((Uint64)data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    }
    info->create_time = SDL_TimeFromWindows(data.ftCreationTime.dwLowDateTime, data.ftCreationTime.dwHighDateTime);
    info->modify_time = SDL_TimeFromWindows(data.ftLastWriteTime.dwLowDateTime, data.ftLastWriteTime.dwHighDateTime);
    info->access_time = SDL_TimeFromWindows(data.ftLastAccessTime.dwLowDateTime, data.ftLastAccessTime.dwHighDateTime);
    return true;
}
