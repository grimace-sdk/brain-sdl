#include "SDL_internal.h"

#include "../core/SDL_wince.h"

extern SDL_DECLSPEC int SDLCALL SDL_WinCE_RunApp(LPWSTR command_line, SDL_main_func main_function);

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

static void SDLCALL LogOutput(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    WCHAR *wide = WINCE_UTF8ToWide(message);

    (void)userdata;
    SDL_GetDefaultLogOutputFunction()(NULL, category, priority, message);
    if (wide) {
        OutputDebugStringW(wide);
        OutputDebugStringW(L"\r\n");
        SDL_free(wide);
    }
}

static int CountArguments(const char *text, char **output, char *storage)
{
    int count = 0;

    while (*text) {
        bool quoted = false;

        while (*text == ' ' || *text == '\t') {
            text++;
        }
        if (!*text) {
            break;
        }
        if (output) {
            output[count] = storage;
        }
        while (*text && (quoted || (*text != ' ' && *text != '\t'))) {
            if (*text == '"') {
                quoted = !quoted;
            } else if (storage) {
                *storage++ = *text;
            }
            text++;
        }
        if (storage) {
            *storage++ = '\0';
        }
        count++;
    }
    return count;
}

int SDL_WinCE_RunApp(LPWSTR command_line, SDL_main_func main_function)
{
    WCHAR module_path[MAX_PATH];
    char *program;
    char *arguments;
    char *storage;
    char **argv;
    int argc;
    int result;

    RedirectStdioToLog();
    SDL_SetLogOutputFunction(LogOutput, NULL);

    if (!GetModuleFileNameW(NULL, module_path, MAX_PATH)) {
        module_path[0] = 0;
    }
    program = WINCE_WideToUTF8(module_path);
    arguments = WINCE_WideToUTF8(command_line ? command_line : L"");
    if (!program || !arguments) {
        SDL_free(program);
        SDL_free(arguments);
        return -1;
    }

    argc = 1 + CountArguments(arguments, NULL, NULL);
    argv = (char **)SDL_calloc((size_t)argc + 1, sizeof(char *));
    storage = (char *)SDL_malloc(SDL_strlen(arguments) + 1);
    if (!argv || !storage) {
        SDL_free(argv);
        SDL_free(storage);
        SDL_free(program);
        SDL_free(arguments);
        return -1;
    }
    argv[0] = program;
    CountArguments(arguments, argv + 1, storage);

    result = SDL_RunApp(argc, argv, main_function, NULL);

    SDL_free(storage);
    SDL_free(argv);
    SDL_free(program);
    SDL_free(arguments);
    return result;
}
