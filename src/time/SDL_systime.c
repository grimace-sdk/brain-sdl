#include "SDL_internal.h"

#include "../core/SDL_wince.h"
#include "time/SDL_time_c.h"

void SDL_GetSystemTimeLocalePreferences(SDL_DateFormat *df, SDL_TimeFormat *tf)
{
    WCHAR text[80];

    if (df && GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, text, SDL_arraysize(text))) {
        LPWSTR character;
        for (character = text; *character; character++) {
            if (*character == L'y') {
                *df = SDL_DATE_FORMAT_YYYYMMDD;
                break;
            }
            if (*character == L'd') {
                *df = SDL_DATE_FORMAT_DDMMYYYY;
                break;
            }
            if (*character == L'M') {
                *df = SDL_DATE_FORMAT_MMDDYYYY;
                break;
            }
        }
    }

    if (tf && GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, text, SDL_arraysize(text))) {
        LPWSTR character;
        for (character = text; *character; character++) {
            if (*character == L'H') {
                *tf = SDL_TIME_FORMAT_24HR;
                break;
            }
            if (*character == L'h') {
                *tf = SDL_TIME_FORMAT_12HR;
                break;
            }
        }
    }
}

bool SDL_GetCurrentTime(SDL_Time *ticks)
{
    FILETIME file_time;

    if (!ticks) {
        return SDL_InvalidParamError("ticks");
    }
    GetSystemTimeAsFileTime(&file_time);
    *ticks = SDL_TimeFromWindows(file_time.dwLowDateTime, file_time.dwHighDateTime);
    return true;
}

bool SDL_TimeToDateTime(SDL_Time ticks, SDL_DateTime *dt, bool localTime)
{
    FILETIME file_time;
    FILETIME local_file_time;
    SYSTEMTIME system_time;
    Uint32 low;
    Uint32 high;
    int remainder_ns;

    if (!dt) {
        return SDL_InvalidParamError("dt");
    }

    remainder_ns = (int)(ticks % SDL_NS_PER_SECOND);
    if (remainder_ns < 0) {
        if (-remainder_ns < 100) {
            ticks -= 100;
        }
        remainder_ns += SDL_NS_PER_SECOND;
    }

    SDL_TimeToWindows(ticks, &low, &high);
    file_time.dwLowDateTime = (DWORD)low;
    file_time.dwHighDateTime = (DWORD)high;

    dt->utc_offset = 0;
    if (localTime) {
        if (!FileTimeToLocalFileTime(&file_time, &local_file_time)) {
            return WINCE_SetError("FileTimeToLocalFileTime()");
        }
        dt->utc_offset = (int)SDL_NS_TO_SECONDS(SDL_TimeFromWindows(local_file_time.dwLowDateTime, local_file_time.dwHighDateTime) - ticks);
        file_time = local_file_time;
    }

    if (!FileTimeToSystemTime(&file_time, &system_time)) {
        return WINCE_SetError("FileTimeToSystemTime()");
    }

    dt->year = system_time.wYear;
    dt->month = system_time.wMonth;
    dt->day = system_time.wDay;
    dt->hour = system_time.wHour;
    dt->minute = system_time.wMinute;
    dt->second = system_time.wSecond;
    dt->nanosecond = remainder_ns;
    dt->day_of_week = system_time.wDayOfWeek;
    return true;
}
