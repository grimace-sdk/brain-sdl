#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

int main(int argc, char *argv[])
{
    Mix_Music *music;
    SDL_Surface *image;

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }
    SDL_Log("Mix_Init flags 0x%x", Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3 | MIX_INIT_FLAC | MIX_INIT_MID));
    SDL_Log("IMG_Init flags 0x%x", IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG));
    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2048) != 0) {
        SDL_Log("Mix_OpenAudio: %s", Mix_GetError());
    }
    music = Mix_LoadMUS(argc > 1 ? argv[1] : "test.ogg");
    SDL_Log("Mix_LoadMUS %s", music ? "ok" : Mix_GetError());
    if (music) {
        Mix_PlayMusic(music, 0);
        SDL_Delay(5000);
        Mix_FreeMusic(music);
    }
    image = IMG_Load(argc > 2 ? argv[2] : "test.png");
    SDL_Log("IMG_Load %s", image ? "ok" : IMG_GetError());
    if (image) {
        SDL_Log("image %dx%d", image->w, image->h);
        SDL_FreeSurface(image);
    }
    Mix_CloseAudio();
    Mix_Quit();
    IMG_Quit();
    SDL_Log("clean exit");
    SDL_Quit();
    return 0;
}
