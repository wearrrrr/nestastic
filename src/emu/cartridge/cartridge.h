#pragma once

#include "src/emu/mapper/mapper.h"
#include <string>
#include <vector>
#include <cstdint>

typedef class Cartridge Cartridge;

Cartridge* load_cartridge(std::string path);

class Cartridge {
public:
    std::vector<uint8_t> prg;   // PRG ROM (16k or 32k)
    std::vector<uint8_t> chr;   // CHR ROM/RAM

    enum class Mirroring {
        HORIZONTAL,
        VERTICAL,
        FOUR_SCREEN,
        SINGLE_SCREEN
    };

    Mirroring mirroring_type;

    uint8_t mapperID = 0;
    Mapper *mapper = nullptr;

    bool cpuRead(uint16_t addr, uint8_t &data);
    bool cpuWrite(uint16_t addr, uint8_t data);
    bool ppuRead(uint16_t addr, uint8_t &data);
    bool ppuWrite(uint16_t addr, uint8_t data);
};
