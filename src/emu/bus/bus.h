#pragma once

#include "../CPU/6502.h"
#include "../PPU/ppu.h"
#include "../cartridge/cartridge.h"

struct Bus {
    CPU6502 cpu;
    PPU ppu;
    Cartridge *cart;

    uint8_t ram[2048];   // CPU RAM

    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
};
