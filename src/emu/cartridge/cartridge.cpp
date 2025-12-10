#include <cstring>
#include <fstream>
#include <limits>

#include "cartridge.h"
#include "../mapper/000/000.h"
#include "../mapper/001/001.h"
#include "../mapper/002/002.h"

Cartridge* load_cartridge(std::string path) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open ROM file: " + (path.empty() ? "No path provided." : path));
    }

    Cartridge *cart = new Cartridge();

    uint8_t header[16];
    file.read((char*)header, 16);

    if (memcmp(header, "NES\x1A", 4) != 0) {
        throw std::runtime_error("Not a NES ROM");
    }

    int prg_size = header[4] * 16 * 1024;
    int chr_size = header[5] * 8 * 1024;

    bool nes20 = (header[7] & 0x0C) == 0x08;
    if (!nes20 && (header[12] || header[13] || header[14] || header[15])) {
        header[7] &= 0x0F;
    }
    cart->mirroring_type =
        (header[6] & 0x01) ? Mirroring::VERTICAL : Mirroring::HORIZONTAL;

    // We don't support trainers.
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

    cart->mapperID = ((header[7] & 0xF0) | (header[6] >> 4));
    switch (cart->mapperID) {
        case 0:
            cart->mapper = new Mapper_000(cart, header[4], header[5]);
            break;
        case 1:
            cart->mapper = new Mapper_001(cart, header[4], header[5]);
            break;
        case 2:
            cart->mapper = new Mapper_002(cart, header[4], header[5]);
            break;
        default:
            throw std::runtime_error("Unsupported mapper: " + std::to_string(cart->mapperID));
    }

    return cart;
}

bool Cartridge::cpuRead(uint16_t addr, uint8_t &data) {
    uint32_t mapped_addr = std::numeric_limits<uint32_t>::max();

    // Let mapper attempt to map the PRG read. Some mappers handle the read
    // themselves and will provide the data via the 'data' out parameter and
    // signal this by setting mapped_addr to the sentinel value (max uint32).
    if (mapper && mapper->prgRead(addr, mapped_addr, data)) {
        if (mapped_addr != std::numeric_limits<uint32_t>::max()) {
            // Mapper returned a PRG ROM/RAM index to read from.
            if (mapped_addr < prg.size()) {
                data = prg[mapped_addr];
                return true;
            } else {
                // Invalid mapping returned
                return false;
            }
        }
        // Mapper already provided 'data' directly
        return true;
    }

    return false;
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = std::numeric_limits<uint32_t>::max();

    if (mapper && mapper->prgWrite(addr, mapped_addr, data)) {
        if (mapped_addr != std::numeric_limits<uint32_t>::max()) {
            if (mapped_addr < prg.size()) {
                prg[mapped_addr] = data;
            }
        }
        return true;
    }

    return false;
}

bool Cartridge::ppuRead(uint16_t addr, uint8_t &data) {
    uint32_t mapped_addr = 0;

    if (mapper && mapper->chrRead(addr, mapped_addr)) {
        if (mapped_addr < chr.size()) {
            data = chr[mapped_addr];
            return true;
        }
        return false;
    }

    return false;
}

bool Cartridge::ppuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = std::numeric_limits<uint32_t>::max();

    if (mapper && mapper->chrWrite(addr, mapped_addr, data)) {
        if (mapped_addr != std::numeric_limits<uint32_t>::max()) {
            if (mapped_addr < chr.size()) {
                chr[mapped_addr] = data;
            }
        }
        return true;
    }

    return false;
}
