#include "SDL.h"
#include "SDL_video.h"
#include "emu/bus/bus.h"

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    // SDL_Surface *surface;
    SDL_Event event;
    SDL_Texture* texture = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    // NES resolution x 4
    if (SDL_CreateWindowAndRenderer(1024, 896, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    SDL_SetWindowTitle(window, "nestastic");

    Bus bus("./test_code/dk.nes");
    bus.cpu.reset();

    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
        }
        if (!running) break;

        while (!bus.ppu.frame_complete) {
            bus.clock();
        }
        bus.ppu.frame_complete = false;

        SDL_UpdateTexture(texture, nullptr, bus.ppu.framebuffer, 256 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }


    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
