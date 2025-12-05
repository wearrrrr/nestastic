#pragma once

#include "../PPU/ppu.h"
#include "../cartridge/cartridge.h"
#include "src/emu/CPU/CPU.h"

struct SaveState {
    CPURegisters cpu_regs;
    CPUFlags cpu_flags;
    PPUSaveState ppu_state;
    bool cpu_pending_nmi;
    uint64_t cycles;
    uint8_t dma_page;
    uint8_t dma_addr;
    uint8_t dma_data;
    bool dma_transfer;
    bool dma_dummy;
    uint8_t controller_state[2];
    uint8_t controller_shift[2];
    uint8_t controller_strobe;
    uint8_t ram[0x10000];
};

class Bus {
public:
    explicit Bus(const char *rom_path);
    ~Bus();

    enum ControllerButton : uint8_t {
        BUTTON_A      = 1 << 0,
        BUTTON_B      = 1 << 1,
        BUTTON_SELECT = 1 << 2,
        BUTTON_START  = 1 << 3,
        BUTTON_UP     = 1 << 4,
        BUTTON_DOWN   = 1 << 5,
        BUTTON_LEFT   = 1 << 6,
        BUTTON_RIGHT  = 1 << 7,
    };

    CPU cpu = CPU(*this);
    PPU ppu = PPU(this);
    Cartridge *cart = nullptr;

    uint8_t ram[0x10000] = {0};

    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
    void clock();

    SaveState save_state() const;
    void load_state(const SaveState &state);

    void set_controller_button(int index, ControllerButton button, bool pressed);

private:
    uint64_t cycles = 0;
    uint8_t dma_page = 0x00;
    uint8_t dma_addr = 0x00;
    uint8_t dma_data = 0x00;
    bool dma_transfer = false;
    bool dma_dummy = true;

    uint8_t controller_state[2] = {0};
    uint8_t controller_shift[2] = {0};
    uint8_t controller_strobe = 0;
};
