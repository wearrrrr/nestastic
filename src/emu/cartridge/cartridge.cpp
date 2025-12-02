#include <cstring>
#include <fstream>

#include "cartridge.h"

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

    cart->mapper = (header[6] >> 4) | (header[7] & 0xF0);
    cart->mirroring_vertical = header[6] & 0x01;

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
