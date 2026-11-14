#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define TONE_FREQUENCY 440
#define SAMPLE_RATE 44100

typedef struct Tone
{
    SDL_AudioStream *stream;
    Uint32 phase;
    bool enabled;
} Tone;

static void SDLCALL FeedTone(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    Tone *tone = (Tone *)userdata;
    Sint16 samples[256];

    while (additional_amount > 0) {
        int count = SDL_min(additional_amount / (int)sizeof(Sint16), (int)SDL_arraysize(samples));
        int index;
        if (count <= 0) {
            break;
        }
        for (index = 0; index < count; index++) {
            Uint32 position = tone->phase++ % (SAMPLE_RATE / TONE_FREQUENCY);
            samples[index] = (Sint16)((tone->enabled && position < SAMPLE_RATE / TONE_FREQUENCY / 2) ? 600 : -600);
            if (!tone->enabled) {
                samples[index] = 0;
            }
        }
        SDL_PutAudioStreamData(stream, samples, count * (int)sizeof(Sint16));
        additional_amount -= count * (int)sizeof(Sint16);
    }
}

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, SAMPLE_RATE };
    Tone tone = { NULL, 0, false };
    SDL_FRect box = { 0, 200, 80, 80 };
    float pointer_x = -1;
    float pointer_y = -1;
    float speed = 4;
    Uint64 frames = 0;
    Uint64 last_report;
    bool running = true;
    int vsync = 1;
    bool bench = argc > 1 && SDL_strcmp(argv[1], "--bench") == 0;
    bool text = true;
    Uint64 bench_start;
    Uint64 draw_ticks = 0;
    Uint64 present_ticks = 0;
    double ticks_per_ms = (double)SDL_GetPerformanceFrequency() / 1000.0;

    SDL_SetLogPriority(SDL_LOG_CATEGORY_VIDEO, SDL_LOG_PRIORITY_DEBUG);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Log("SDL %d.%d.%d on %s, video %s, audio %s", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION,
            SDL_GetPlatform(), SDL_GetCurrentVideoDriver(), SDL_GetCurrentAudioDriver());
    SDL_Log("base path %s", SDL_GetBasePath());

    if (!SDL_CreateWindowAndRenderer("sdltest", 800, 480, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, vsync);
    SDL_Log("renderer %s", SDL_GetRendererName(renderer));

    tone.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedTone, &tone);
    if (tone.stream) {
        SDL_ResumeAudioStreamDevice(tone.stream);
    } else {
        SDL_Log("SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
    }

    last_report = SDL_GetTicks();
    bench_start = last_report;
    if (bench) {
        SDL_Log("bench: 10 s vsync on, 10 s vsync off, 10 s vsync off without text");
    }
    while (running) {
        SDL_Event event;
        Uint64 now;
        Uint64 draw_start;
        Uint64 present_start;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                SDL_Log("key down %s (raw 0x%x)", SDL_GetScancodeName(event.key.scancode), event.key.raw);
                if (event.key.scancode == SDL_SCANCODE_BACKSPACE) {
                    running = false;
                } else if (event.key.scancode == SDL_SCANCODE_SPACE) {
                    tone.enabled = !tone.enabled;
                } else if (event.key.scancode == SDL_SCANCODE_V) {
                    vsync = !vsync;
                    SDL_SetRenderVSync(renderer, vsync);
                    SDL_Log("vsync %d", vsync);
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                pointer_x = event.motion.x;
                pointer_y = event.motion.y;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                SDL_Log("tap %.0f,%.0f", event.button.x, event.button.y);
                break;
            default:
                break;
            }
        }

        box.x += speed;
        if (box.x < 0 || box.x + box.w > 800) {
            speed = -speed;
            box.x += speed;
        }

        draw_start = SDL_GetPerformanceCounter();
        SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
        SDL_RenderFillRect(renderer, &box);
        if (pointer_x >= 0) {
            SDL_FRect cursor = { pointer_x - 10, pointer_y - 10, 20, 20 };
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &cursor);
        }
        if (text) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, 8, 8, "sdltest: HOME quits, Space tone, V vsync, tap draws");
        }
        present_start = SDL_GetPerformanceCounter();
        SDL_RenderPresent(renderer);
        draw_ticks += present_start - draw_start;
        present_ticks += SDL_GetPerformanceCounter() - present_start;

        frames++;
        now = SDL_GetTicks();
        if (now - last_report >= 2000) {
            SDL_Log("%.1f fps, vsync %d, draw %.2f ms, present %.2f ms", (double)frames * 1000.0 / (double)(now - last_report), vsync,
                    (double)draw_ticks / ticks_per_ms / (double)frames, (double)present_ticks / ticks_per_ms / (double)frames);
            frames = 0;
            draw_ticks = 0;
            present_ticks = 0;
            last_report = now;
        }
        if (bench) {
            if (vsync && now - bench_start >= 10000) {
                vsync = 0;
                SDL_SetRenderVSync(renderer, vsync);
                SDL_Log("vsync %d", vsync);
            } else if (text && now - bench_start >= 20000) {
                text = false;
                SDL_Log("text off");
            } else if (now - bench_start >= 30000) {
                running = false;
            }
        }
    }

    SDL_DestroyAudioStream(tone.stream);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Log("clean exit");
    SDL_Quit();
    return 0;
}
