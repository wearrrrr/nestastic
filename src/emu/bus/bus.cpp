#include "bus.h"

uint8_t Bus::read(uint16_t addr)
{
    uint8_t data = 0x00;

    // Cartridge first (mappers override everything)
    if (cart && cart->cpuRead(addr, data))
        return data;

    // 2 KB internal RAM, mirrored every 0x800 bytes
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return ram[addr & 0x07FF];
    }

    // PPU registers, mirrored every 8 bytes
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        // return ppu.cpuRead(0x2000 + (addr & 7));
        return 0x00;
    }

    // APU + IO (not implemented yet)
    if (addr >= 0x4000 && addr <= 0x401F) {
        // stub for now
        return 0;
    }

    return data;
}

void Bus::write(uint16_t addr, uint8_t value)
{
    // Cartridge first (mappers sometimes catch writes)
    if (cart && cart->cpuWrite(addr, value))
        return;

    // CPU RAM
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        ram[addr & 0x07FF] = value;
        return;
    }

    // PPU registers
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        // ppu.cpuWrite(0x2000 + (addr & 7), value);
        return;
    }

    // APU/IO stubs
    if (addr >= 0x4000 && addr <= 0x401F) {
        // TODO: APU, OAMDMA
        return;
    }
}
