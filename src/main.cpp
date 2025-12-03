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

    Bus bus("./test_code/balloonfight.nes");
    bus.cpu.reset();

    auto handle_key = [&bus](SDL_Keycode key, bool pressed) {
        switch (key) {
            case SDLK_x:
                bus.set_controller_button(0, Bus::BUTTON_A, pressed);
                break;
            case SDLK_z:
                bus.set_controller_button(0, Bus::BUTTON_B, pressed);
                break;
            case SDLK_RSHIFT:
            case SDLK_LSHIFT:
                bus.set_controller_button(0, Bus::BUTTON_SELECT, pressed);
                break;
            case SDLK_RETURN:
                bus.set_controller_button(0, Bus::BUTTON_START, pressed);
                break;
            case SDLK_UP:
                bus.set_controller_button(0, Bus::BUTTON_UP, pressed);
                break;
            case SDLK_DOWN:
                bus.set_controller_button(0, Bus::BUTTON_DOWN, pressed);
                break;
            case SDLK_LEFT:
                bus.set_controller_button(0, Bus::BUTTON_LEFT, pressed);
                break;
            case SDLK_RIGHT:
                bus.set_controller_button(0, Bus::BUTTON_RIGHT, pressed);
                break;
            default:
                break;
        }
    };

    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                handle_key(event.key.keysym.sym, true);
            } else if (event.type == SDL_KEYUP) {
                handle_key(event.key.keysym.sym, false);
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
