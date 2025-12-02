#pragma once

#include <string>
#include <vector>
#include <cstdint>

typedef class Cartridge Cartridge;

Cartridge* load_cartridge(std::string path);

class Cartridge {
public:
    std::vector<uint8_t> prg;   // PRG ROM (16k or 32k)
    std::vector<uint8_t> chr;   // CHR ROM/RAM
    int mapper;
    bool mirroring_vertical;

    bool cpuRead(uint16_t addr, uint8_t &data) {
        // PRG ROM is at $8000-$FFFF
        if (addr >= 0x8000) {
            uint32_t offset = addr - 0x8000;
            data = prg[offset % prg.size()];
            return true;
        }
        return false;
    }

    bool cpuWrite(uint16_t addr, uint8_t value) {
        return false; // ROM cannot be written
    }
};
