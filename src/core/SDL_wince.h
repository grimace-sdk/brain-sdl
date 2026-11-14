#ifndef SDL_wince_h_
#define SDL_wince_h_

#include "SDL_internal.h"

#include <windows.h>
#include <stdio.h>

extern WCHAR *WINCE_UTF8ToWide(const char *text);
extern char *WINCE_WideToUTF8(const WCHAR *text);
extern WCHAR *WINCE_ResolvePath(const char *path);
extern const WCHAR *WINCE_GetProgramDirectory(void);
extern bool WINCE_SetError(const char *prefix);

#endif
