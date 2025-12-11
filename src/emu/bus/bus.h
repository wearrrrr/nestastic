#pragma once

#include "../PPU/ppu.h"
#include "../cartridge/cartridge.h"
#include "../APU/apu.h"
#include "src/emu/CPU/CPU.h"

// Forward declarations for APU and AudioPlayer so the Bus header doesn't need
// to directly include audio/APU implementation details.
class AudioPlayer;
class APU;

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
    // APU instance managed by the Bus (constructed at runtime)
    APU *apu = nullptr;
    // Audio player used by the APU; forward-declared above
    AudioPlayer *audio_player = nullptr;

    uint8_t ram[0x10000] = {0};

    // Read/write/clock entrypoints
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
    void clock();

    SaveState save_state() const;
    void load_state(const SaveState &state);

    // Controller input
    void set_controller_button(int index, ControllerButton button, bool pressed);

    // APU logging: when enabled the Bus will print APU register reads/writes.
    // This toggle is intentionally part of the Bus so the wiring code in the
    // .cpp can consult it and emit diagnostic output when APU registers are
    // accessed. Use setAPULogging(true) to enable.
    void setAPULogging(bool enable) { apu_logging = enable; }
    bool getAPULogging() const { return apu_logging; }

private:
    // Internal flag that controls emitting APU register access logs.
    bool apu_logging = false;

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
