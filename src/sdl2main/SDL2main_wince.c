#include <windows.h>

#include "SDL.h"

extern void SDL2Compat_InitWinCE(void);
extern int main(int argc, char *argv[]) __attribute__((weak));
extern int cxx_main(int argc, char *argv[]) __asm__("_Z4mainiPPc") __attribute__((weak));

static void RedirectStdioToLog(void)
{
    WCHAR path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH - 4);
    DWORD dot = length;

    while (dot > 0 && path[dot - 1] != L'.' && path[dot - 1] != L'\\') {
        dot--;
    }
    if (dot == 0 || path[dot - 1] != L'.') {
        dot = length + 1;
    }
    path[dot - 1] = L'.';
    path[dot] = L'l';
    path[dot + 1] = L'o';
    path[dot + 2] = L'g';
    path[dot + 3] = 0;
    DeleteFileW(path);
    grimace_redirect_stdio(path);
}

static SDL_LogOutputFunction default_log_output;
static void *default_log_userdata;

static void SDLCALL LogOutput(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    WCHAR *wide = (WCHAR *)SDL_iconv_string("UTF-16LE", "UTF-8", message, SDL_strlen(message) + 1);

    if (default_log_output) {
        default_log_output(default_log_userdata, category, priority, message);
    }
    if (wide) {
        OutputDebugStringW(wide);
        OutputDebugStringW(L"\r\n");
        SDL_free(wide);
    }
}

static char *WideToUTF8(const WCHAR *text)
{
    return SDL_iconv_string("UTF-8", "UTF-16LE", (const char *)text, (SDL_wcslen((const wchar_t *)text) + 1) * sizeof(WCHAR));
}

static int SplitArguments(char *text, char **output)
{
    int count = 0;
    char *write = text;

    while (*text) {
        SDL_bool quoted = SDL_FALSE;

        while (*text == ' ' || *text == '\t') {
            text++;
        }
        if (!*text) {
            break;
        }
        if (output) {
            output[count] = write;
        }
        while (*text && (quoted || (*text != ' ' && *text != '\t'))) {
            if (*text == '"') {
                quoted = !quoted;
            } else if (output) {
                *write++ = *text;
            }
            text++;
        }
        if (output) {
            *write++ = '\0';
        }
        if (*text) {
            text++;
        }
        count++;
    }
    return count;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR command_line, int show)
{
    WCHAR module_path[MAX_PATH];
    char *program;
    char *arguments;
    char *scan;
    char **argv;
    int argc;
    int result;

    (void)instance;
    (void)previous;
    (void)show;

    RedirectStdioToLog();
    SDL2Compat_InitWinCE();
    SDL_LogGetOutputFunction(&default_log_output, &default_log_userdata);
    SDL_LogSetOutputFunction(LogOutput, NULL);

    if (!GetModuleFileNameW(NULL, module_path, MAX_PATH)) {
        module_path[0] = 0;
    }
    program = WideToUTF8(module_path);
    arguments = WideToUTF8(command_line ? command_line : L"");
    scan = arguments ? SDL_strdup(arguments) : NULL;
    if (!program || !arguments || !scan) {
        return -1;
    }

    argc = 1 + SplitArguments(scan, NULL);
    argv = (char **)SDL_calloc((size_t)argc + 1, sizeof(char *));
    if (!argv) {
        return -1;
    }
    argv[0] = program;
    SplitArguments(arguments, argv + 1);

    SDL_SetMainReady();
    result = main ? main(argc, argv) : cxx_main(argc, argv);

    SDL_free(argv);
    SDL_free(scan);
    SDL_free(arguments);
    SDL_free(program);
    return result;
}
