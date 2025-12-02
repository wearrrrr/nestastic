#include "SDL.h"
#include "SDL_video.h"
#include "emu/bus/bus.h"
#include "src/emu/cartridge/cartridge.h"

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    // SDL_Surface *surface;
    SDL_Event event;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    // NES resolution x 4
    if (SDL_CreateWindowAndRenderer(1024, 896, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    SDL_SetWindowTitle(window, "nestastic");

    Bus bus;
    bus.cart = load_cartridge("./test_code/smb.nes");
    bus.cpu.reset();
    bus.cpu.bus = &bus;

    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t last = now;
    double ns_per_cycle = 559.0;
    const int CYCLES_PER_SLICE = 1000;

    while (1) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) break;

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        now = SDL_GetPerformanceCounter();
        double elapsed_ns = (now - last) * 1e9 / SDL_GetPerformanceFrequency();

        while (elapsed_ns >= ns_per_cycle) {
            for (int i = 0; i < CYCLES_PER_SLICE && elapsed_ns >= ns_per_cycle; ++i) {
                bus.cpu.clock();
                elapsed_ns -= ns_per_cycle;
                last += (uint64_t)(ns_per_cycle * SDL_GetPerformanceFrequency() / 1e9);
            }
            // Allow SDL to handle input
            SDL_PollEvent(&event);
            if (event.type == SDL_QUIT) break;
        }
    }


    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
