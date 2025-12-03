#pragma once

#include "../CPU/6502.h"
#include "../PPU/ppu.h"
#include "../cartridge/cartridge.h"

class Bus {
public:
    explicit Bus(const char *rom_path);
    ~Bus();

    CPU6502 cpu;
    PPU ppu;
    Cartridge *cart = nullptr;

    uint8_t ram[0x10000] = {0};

    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
    void clock();

private:
    uint64_t cycles = 0;
    uint8_t dma_page = 0x00;
    uint8_t dma_addr = 0x00;
    uint8_t dma_data = 0x00;
    bool dma_transfer = false;
    bool dma_dummy = true;
};
