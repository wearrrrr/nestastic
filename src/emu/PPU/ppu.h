#pragma once

#include <cstdint>
#include <cstdio>

typedef struct Bus Bus;

struct ObjectAttributeEntry {
    uint8_t y;
    uint8_t id;
    uint8_t attribute;
    uint8_t x;
};

static_assert(sizeof(ObjectAttributeEntry) == 4, "OAM entry must remain 4 bytes");

class PPU {
public:
    PPU();
    explicit PPU(Bus *bus);
    void connect(Bus *bus);
private:

    struct {
        // $2000 – PPUCTRL
        union {
            uint8_t reg;
            struct {
                uint8_t nametable_x : 1;
    			uint8_t nametable_y : 1;
    			uint8_t increment : 1;
    			uint8_t pattern_sprite : 1;
    			uint8_t pattern_background : 1;
    			uint8_t sprite_size : 1;
    			uint8_t master_slave : 1; // unused
    			uint8_t enable_nmi : 1;
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

   	union v_reg
	{
		struct
		{

			uint16_t coarse_x : 5;
			uint16_t coarse_y : 5;
			uint16_t nametable_x : 1;
			uint16_t nametable_y : 1;
			uint16_t fine_y : 3;
			uint16_t unused : 1;
		};

		uint16_t reg = 0x0000;
	};

	v_reg vram_addr;
	v_reg tram_addr;

    uint8_t     nametable[2][1024];
    uint8_t     pattern_table[2][4096];
    uint8_t		palette_table[32];
    uint8_t		oam_addr = 0x00;
    ObjectAttributeEntry OAM[64];
    ObjectAttributeEntry spriteScanline[8];
    uint8_t sprite_count = 0;
    bool sprite_zero_hit_possible = false;
    bool sprite_zero_being_rendered = false;
    uint8_t sprite_shifter_pattern_lo[8];
    uint8_t sprite_shifter_pattern_hi[8];

private:
   	uint8_t fine_x = 0x00;

	// Internal communications
	uint8_t address_latch = 0x00;
	uint8_t ppu_data_buffer = 0x00;

	// Pixel "dot" position information
	int16_t scanline = 0;
	int16_t cycle = 0;

	// Background rendering
	uint8_t bg_next_tile_id     = 0x00;
	uint8_t bg_next_tile_attrib = 0x00;
	uint8_t bg_next_tile_lsb    = 0x00;
	uint8_t bg_next_tile_msb    = 0x00;
	uint16_t bg_shifter_pattern_lo = 0x0000;
	uint16_t bg_shifter_pattern_hi = 0x0000;
	uint16_t bg_shifter_attrib_lo  = 0x0000;
	uint16_t bg_shifter_attrib_hi  = 0x0000;

    uint32_t nesPalette[0x40] = {
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
    };

    uint32_t GetColorFromPaletteRam(uint8_t palette, uint8_t pixel);

public:
    Bus *bus;

    uint8_t cpuRead(uint16_t addr, bool rdonly = false);
	void    cpuWrite(uint16_t addr, uint8_t  data);
    uint8_t oamRead(uint8_t addr) const;
    void    oamWrite(uint8_t addr, uint8_t data);
    void    dmaWrite(uint8_t data);

	uint8_t ppuRead(uint16_t addr, bool rdonly = false);
	void    ppuWrite(uint16_t addr, uint8_t data);


    uint32_t framebuffer[256 * 240];

	void clock();
	void reset();
	bool nmi = false;
	bool frame_complete = false;
};
