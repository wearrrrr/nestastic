#include <cstring>
#include <fstream>

#include "cartridge.h"
#include "src/emu/mapper/000/000.h"

Cartridge* load_cartridge(std::string path) {
    std::ifstream file(path, std::ios::binary);
    Cartridge *cart = new Cartridge();

    uint8_t header[16];
    file.read((char*)header, 16);

    if (memcmp(header, "NES\x1A", 4) != 0) {
        throw std::runtime_error("Not an NES ROM");
    }

    int prg_size = header[4] * 16 * 1024;
    int chr_size = header[5] * 8 * 1024;

    cart->mapperID = ((header[7] >> 4) << 4) | (header[6] >> 4);
    switch (cart->mapperID) {
        case 0:
            cart->mapper = new Mapper_000(header[4], header[5]);
            break;
        default:
            throw std::runtime_error("Unsupported mapper: " + std::to_string(cart->mapperID));
    }
    cart->mirroring_type =
        (header[6] & 0x01) ? Cartridge::Mirroring::VERTICAL
                           : Cartridge::Mirroring::HORIZONTAL;

    // Ignore trainer if present
    if (header[6] & 0x04)
        file.seekg(512, std::ios::cur);

    cart->prg.resize(prg_size);
    file.read((char*)cart->prg.data(), prg_size);

    if (chr_size == 0) {
        // CHR RAM (8KB)
        cart->chr.resize(8 * 1024);
    } else {
        cart->chr.resize(chr_size);
        file.read((char*)cart->chr.data(), chr_size);
    }

    return cart;
}

bool Cartridge::cpuRead(uint16_t addr, uint8_t &data) {
    uint32_t mapped_addr = 0;

    if (mapper->cpuMapRead(addr, mapped_addr)) {
        data = prg[mapped_addr];
        return true;
    }

    return false;
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;

    if (mapper->cpuMapWrite(addr, mapped_addr, data)) {
        prg[mapped_addr] = data; // Only if CHR RAM/PRG RAM
        return true;
    }

    return false;
}

bool Cartridge::ppuRead(uint16_t addr, uint8_t &data) {
    uint32_t mapped_addr = 0;

    if (mapper->ppuMapRead(addr, mapped_addr)) {
        data = chr[mapped_addr];
        return true;
    }

    return false;
}

bool Cartridge::ppuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;

    if (mapper->ppuMapWrite(addr, mapped_addr)) {
        chr[mapped_addr] = data;
        return true;
    }

    return false;
}
