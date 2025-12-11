#include "SDL.h"
#include "SDL_video.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "imgui_memory_editor.h"
#include "emu/bus/bus.h"
#include <cstdio>
#include <filesystem>
#include <string>

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    SDL_Texture* texture = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    // NES resolution x 4
    if (SDL_CreateWindowAndRenderer(1024, 896, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    SDL_SetWindowTitle(window, "nestastic");

    const char *rom_arg = (argc > 1) ? argv[1] : "";
    std::string rom_label = "(none)";
    if (rom_arg && rom_arg[0] != '\0') {
        rom_label = std::filesystem::path(rom_arg).filename().string();
    }

    Bus bus(rom_arg);
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

    const double target_frame_time = 1.0 / 60.0;
    const double perf_freq = static_cast<double>(SDL_GetPerformanceFrequency());
    Uint64 last_counter = SDL_GetPerformanceCounter();
    double accumulator = 0.0;
    double fps_time_accum = 0.0;
    int fps_frames = 0;
    char title_buffer[64];

    bool running = true;

    bool debug_mode = false;

    SaveState save_state = {};
    bool has_save_state = false;

    while (running) {

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        Uint64 current_counter = SDL_GetPerformanceCounter();
        double delta_seconds = (current_counter - last_counter) / perf_freq;
        last_counter = current_counter;

        if (delta_seconds > 0.25) {
            delta_seconds = 0.25;
        }

        accumulator += delta_seconds;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                handle_key(event.key.keysym.sym, true);
            } else if (event.type == SDL_KEYUP) {
                handle_key(event.key.keysym.sym, false);
            }
        }
        if (!running) {
            break;
        }

        bool frame_rendered = false;
        while (running && accumulator >= target_frame_time) {
            while (!bus.ppu.frame_complete) {
                bus.clock();
            }
            bus.ppu.frame_complete = false;

            SDL_UpdateTexture(texture, nullptr, bus.ppu.framebuffer, 256 * sizeof(uint32_t));

            accumulator -= target_frame_time;
            fps_time_accum += target_frame_time;
            fps_frames++;
            frame_rendered = true;

            if (fps_time_accum >= 0.5) {
                double fps = static_cast<double>(fps_frames) / fps_time_accum;
                std::snprintf(title_buffer, sizeof(title_buffer), "nestastic - %.1f FPS", fps);
                SDL_SetWindowTitle(window, title_buffer);
                fps_time_accum = 0.0;
                fps_frames = 0;
            }
        }

        if (!frame_rendered) {
            SDL_Delay(1);
        }

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Emulation")) {
                if (ImGui::MenuItem("Reset")) {
                    bus.cpu.reset();
                    bus.ppu.reset();
                }
                ImGui::MenuItem("Debug", nullptr, &debug_mode);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Savestate")) {
                if (ImGui::MenuItem("Save")) {
                    save_state = bus.save_state();
                    has_save_state = true;
                }
                ImGui::BeginDisabled(!has_save_state);
                if (ImGui::MenuItem("Load") && has_save_state) {
                    bus.load_state(save_state);
                }
                ImGui::EndDisabled();
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (debug_mode) {
            CPURegisters cpu_state = bus.cpu.get_regs();
            char flag_buf[8];
            flag_buf[0] = (cpu_state.status & 0x80) ? 'N' : '.';
            flag_buf[1] = (cpu_state.status & 0x40) ? 'V' : '.';
            flag_buf[2] = (cpu_state.status & 0x10) ? 'B' : '.';
            flag_buf[3] = (cpu_state.status & 0x08) ? 'D' : '.';
            flag_buf[4] = (cpu_state.status & 0x04) ? 'I' : '.';
            flag_buf[5] = (cpu_state.status & 0x02) ? 'Z' : '.';
            flag_buf[6] = (cpu_state.status & 0x01) ? 'C' : '.';
            flag_buf[7] = '\0';

            ImGui::Begin("NES Debug");
            ImGui::Text("ROM: %s", rom_label.c_str());
            ImGui::Text("FPS (ImGui): %.1f", io.Framerate);
            ImGui::Separator();
            ImGui::Text("CPU PC: $%04X", cpu_state.pc);
            ImGui::Text("A:$%02X  X:$%02X  Y:$%02X  SP:$%02X", cpu_state.a, cpu_state.x, cpu_state.y, cpu_state.sp);
            ImGui::Text("Flags: %s", flag_buf);
            ImGui::Text("Cycles (CPU): %d", bus.cpu.getCycleCount());
            ImGui::Text("Pending NMI: %s", bus.cpu.pendingNMI ? "yes" : "no");
            ImGui::Separator();
            if (bus.cart) {
                ImGui::Text("Mapper ID: %u", bus.cart->mapperID);
            } else {
                ImGui::Text("Mapper: (unknown)");
            }

            ImGui::SeparatorText("Memory Editor");
            static MemoryEditor mem_edit_1;
            mem_edit_1.DrawContents(bus.ram, sizeof(bus.ram));
            ImGui::End();
        }

        ImGui::Render();
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
