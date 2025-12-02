#pragma once

#include <cstdint>

struct PPURegisters {
    // $2000 – PPUCTRL
    union {
        uint8_t reg;
        struct {
            uint8_t nametable       : 2;
            uint8_t increment       : 1;
            uint8_t sprite_table    : 1;
            uint8_t bg_table        : 1;
            uint8_t sprite_size     : 1;
            uint8_t master_slave    : 1;
            uint8_t nmi_enable      : 1;
        };
    } ctrl;

    // $2001 – PPUMASK
    union {
        uint8_t reg;
        struct {
            uint8_t grayscale       : 1;
            uint8_t show_bg_left    : 1;
            uint8_t show_spr_left   : 1;
            uint8_t show_bg         : 1;
            uint8_t show_spr        : 1;
            uint8_t emphasize_red   : 1;
            uint8_t emphasize_green : 1;
            uint8_t emphasize_blue  : 1;
        };
    } mask;

    // $2002 – PPUSTATUS
    union {
        uint8_t reg;
        struct {
            uint8_t unused          : 5;
            uint8_t sprite_overflow : 1;
            uint8_t sprite_zero_hit : 1;
            uint8_t vblank          : 1;
        };
    } status;
};

struct PPUInternal {
    uint8_t oamaddr = 0;
    uint8_t oamdata[256];

    uint16_t vram_addr = 0;
    uint16_t tram_addr = 0;

    uint8_t fine_x = 0;
    bool write_latch = false;
    uint8_t buffered_read = 0;
};

struct PPU {
    PPURegisters reg;   // CPU-visible registers
    PPUInternal  st;    // Internal registers/latches

    uint8_t vram[0x4000]; // 16KB VRAM/mirroring
    uint8_t palette[32];  // palette RAM

    // TODO: CHR ROM will be mapped from cartridge
};
