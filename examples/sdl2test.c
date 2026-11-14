#include "SDL.h"

static Uint16 pixels[320 * 200];

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    Uint32 start;
    Uint32 last_report;
    int frames = 0;
    int tick = 0;
    int running = 1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_version linked;
    SDL_GetVersion(&linked);
    SDL_Log("SDL2 API %d.%d.%d, video %s, audio %s", linked.major, linked.minor, linked.patch, SDL_GetCurrentVideoDriver(),
            SDL_GetCurrentAudioDriver());

    window = SDL_CreateWindow("sdl2test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 400, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!window || !renderer) {
        SDL_Log("window/renderer failed: %s", SDL_GetError());
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, 320, 200);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 320, 200);

    start = SDL_GetTicks();
    last_report = start;
    while (running) {
        SDL_Event event;
        int x;
        int y;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE)) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Log("key %s", SDL_GetKeyName(event.key.keysym.sym));
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                SDL_Log("tap %d,%d", event.button.x, event.button.y);
            }
        }

        for (y = 0; y < 200; y++) {
            for (x = 0; x < 320; x++) {
                pixels[y * 320 + x] = (Uint16)((((x + tick) >> 2) & 0x1F) << 11 | (((y + tick) >> 1) & 0x3F) << 5);
            }
        }
        tick++;
        SDL_UpdateTexture(texture, NULL, pixels, 320 * 2);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        frames++;
        if (SDL_GetTicks() - last_report >= 2000) {
            SDL_Log("%.1f fps", frames * 1000.0 / (SDL_GetTicks() - last_report));
            frames = 0;
            last_report = SDL_GetTicks();
        }
        if (argc > 1 && SDL_strcmp(argv[1], "--bench") == 0 && SDL_GetTicks() - start >= 15000) {
            running = 0;
        }
    }

    SDL_Log("clean exit");
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
