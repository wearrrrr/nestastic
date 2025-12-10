#include "ppu.h"
#include "../bus/bus.h" // IWYU pragma: keep
#include "../mapper/001/001.h"
#include <cstring>
#include <algorithm>

PPU::PPU() : bus(nullptr) {
	reset();
}

PPU::PPU(Bus *bus) : bus(bus) {
	reset();
}

void PPU::connect(Bus *bus) {
	this->bus = bus;
}

PPUSaveState PPU::save_state() const {
	PPUSaveState state{};
	state.ctrl = ctrl;
	state.mask = mask;
	state.status = status;
	state.vram_addr = vram_addr.reg;
	state.tram_addr = tram_addr.reg;
	state.fine_x = fine_x;
	state.address_latch = address_latch;
	state.ppu_data_buffer = ppu_data_buffer;
	state.scanline = scanline;
	state.cycle = cycle;
	state.odd_frame = odd_frame;
	state.oam_addr = oam_addr;
	state.sprite_count = sprite_count;
	state.sprite_zero_hit_possible = sprite_zero_hit_possible;
	state.sprite_zero_being_rendered = sprite_zero_being_rendered;
	state.sprite_zero_scanline = sprite_zero_scanline;
	state.bg_next_tile_id = bg_next_tile_id;
	state.bg_next_tile_attrib = bg_next_tile_attrib;
	state.bg_next_tile_lsb = bg_next_tile_lsb;
	state.bg_next_tile_msb = bg_next_tile_msb;
	state.bg_shifter_pattern_lo = bg_shifter_pattern_lo;
	state.bg_shifter_pattern_hi = bg_shifter_pattern_hi;
	state.bg_shifter_attrib_lo = bg_shifter_attrib_lo;
	state.bg_shifter_attrib_hi = bg_shifter_attrib_hi;
	state.nmi_line = nmi_line;
	state.nmi = nmi;
	state.frame_complete = frame_complete;
	std::memcpy(state.nametable, nametable, sizeof(nametable));
	std::memcpy(state.pattern_table, pattern_table, sizeof(pattern_table));
	std::memcpy(state.palette_table, palette_table, sizeof(palette_table));
	std::memcpy(state.OAM, OAM, sizeof(OAM));
	std::memcpy(state.spriteScanline, spriteScanline, sizeof(spriteScanline));
	std::memcpy(state.sprite_shifter_pattern_lo, sprite_shifter_pattern_lo, sizeof(sprite_shifter_pattern_lo));
	std::memcpy(state.sprite_shifter_pattern_hi, sprite_shifter_pattern_hi, sizeof(sprite_shifter_pattern_hi));
	return state;
}

void PPU::load_state(const PPUSaveState &state) {
	ctrl = state.ctrl;
	mask = state.mask;
	status = state.status;
	vram_addr.reg = state.vram_addr;
	tram_addr.reg = state.tram_addr;
	fine_x = state.fine_x;
	address_latch = state.address_latch;
	ppu_data_buffer = state.ppu_data_buffer;
	scanline = state.scanline;
	cycle = state.cycle;
	odd_frame = state.odd_frame;
	oam_addr = state.oam_addr;
	sprite_count = state.sprite_count;
	sprite_zero_hit_possible = state.sprite_zero_hit_possible;
	sprite_zero_being_rendered = state.sprite_zero_being_rendered;
	sprite_zero_scanline = state.sprite_zero_scanline;
	bg_next_tile_id = state.bg_next_tile_id;
	bg_next_tile_attrib = state.bg_next_tile_attrib;
	bg_next_tile_lsb = state.bg_next_tile_lsb;
	bg_next_tile_msb = state.bg_next_tile_msb;
	bg_shifter_pattern_lo = state.bg_shifter_pattern_lo;
	bg_shifter_pattern_hi = state.bg_shifter_pattern_hi;
	bg_shifter_attrib_lo = state.bg_shifter_attrib_lo;
	bg_shifter_attrib_hi = state.bg_shifter_attrib_hi;
	nmi_line = state.nmi_line;
	nmi = state.nmi;
	frame_complete = state.frame_complete;
	std::memcpy(nametable, state.nametable, sizeof(nametable));
	std::memcpy(pattern_table, state.pattern_table, sizeof(pattern_table));
	std::memcpy(palette_table, state.palette_table, sizeof(palette_table));
	std::memcpy(OAM, state.OAM, sizeof(OAM));
	std::memcpy(spriteScanline, state.spriteScanline, sizeof(spriteScanline));
	std::memcpy(sprite_shifter_pattern_lo, state.sprite_shifter_pattern_lo, sizeof(sprite_shifter_pattern_lo));
	std::memcpy(sprite_shifter_pattern_hi, state.sprite_shifter_pattern_hi, sizeof(sprite_shifter_pattern_hi));
}

void PPU::update_nmi_line() {
	bool line = ctrl.enable_nmi && status.vblank;
	if (line && !nmi_line)
		nmi = true;
	nmi_line = line;
}

uint32_t PPU::get_color(uint8_t palette, uint8_t pixel) {
	uint8_t index = ppuRead(0x3F00 + (palette << 2) + pixel) & 0x3F;
	uint32_t color = nesPalette[index];
	uint8_t r = (color >> 16) & 0xFF;
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t b = color & 0xFF;

	auto apply_emphasis = [](uint8_t channel, bool emphasize) {
		float value = channel;
		const float boost = emphasize ? 1.15f : 0.92f;
		value = std::clamp(value * boost, 0.0f, 255.0f);
		return value;
	};

   	if (mask.emphasize_red) r = apply_emphasis(r, mask.emphasize_red);
   	if (mask.emphasize_green) g = apply_emphasis(g, mask.emphasize_green);
   	if (mask.emphasize_blue) b = apply_emphasis(b, mask.emphasize_blue);

	return ((r) << 16) | ((g) << 8) | (b);
}


uint8_t PPU::cpuRead(uint16_t addr, bool read_only) {
	uint8_t data = 0x00;

	if (read_only) {
		if (addr == 0x0000) {
		    data = ctrl.reg;
		} else if (addr == 0x0001) {
		    data = mask.reg;
		} else if (addr == 0x0002) {
		    data = status.reg;
		}
	} else {
		if (addr == 0x0002) {
			data = (status.reg & 0xE0) | (ppu_data_buffer & 0x1F);
			status.vblank = 0;
			address_latch = 0;
			update_nmi_line();
		} else if (addr == 0x0004) {
			data = oamRead(oam_addr);
		} else if (addr == 0x0007) {
			data = ppu_data_buffer;
			uint16_t current_addr = vram_addr.reg;
			ppu_data_buffer = ppuRead(current_addr);
			if (current_addr >= 0x3F00)
			{
				data = ppu_data_buffer;
			}
			vram_addr.reg += (ctrl.increment ? 32 : 1);
		}
	}

	return data;
}

void PPU::cpuWrite(uint16_t addr, uint8_t data) {
	switch (addr) {
    	case 0x0000: // Control
    		ctrl.reg = data;
    		tram_addr.nametable_x = ctrl.nametable_x;
    		tram_addr.nametable_y = ctrl.nametable_y;
    		update_nmi_line();
    		break;
    	case 0x0001: // Mask
    		mask.reg = data;
    		break;
    	case 0x0002: // Status
    		break;
    	case 0x0003:
    		oam_addr = data;
    		break;
    	case 0x0004: // OAM Data
    		dmaWrite(data);
    		break;
    	case 0x0005: // Scroll
    		if (address_latch == 0) {
    			tram_addr.coarse_x = data >> 3;
    			fine_x = data & 0x07;
    			address_latch = 1;
    		} else {
    			tram_addr.fine_y = data & 0x07;
    			tram_addr.coarse_y = data >> 3;
    			address_latch = 0;
    		}
    		break;
    	case 0x0006: // PPU Address
    		if (address_latch == 0) {
    			tram_addr.reg = (uint16_t)((data & 0x3F) << 8) | (tram_addr.reg & 0x00FF);
    			address_latch = 1;
    		}
    		else {
    			tram_addr.reg = (tram_addr.reg & 0xFF00) | data;
    			vram_addr = tram_addr;
    			address_latch = 0;
    		}
    		break;
    	case 0x0007: // PPU Data
    		ppuWrite(vram_addr.reg, data);
    		vram_addr.reg += (ctrl.increment ? 32 : 1);
    		break;
	}
}

uint8_t PPU::oamRead(uint8_t addr) const {
	const uint8_t *bytes = reinterpret_cast<const uint8_t*>(OAM);
	return bytes[addr];
}

void PPU::oamWrite(uint8_t addr, uint8_t data) {
	uint8_t *bytes = (uint8_t*)(OAM);
	bytes[addr] = data;
}

void PPU::dmaWrite(uint8_t data) {
	oamWrite(oam_addr, data);
	oam_addr++;
}

uint8_t PPU::ppuRead(uint16_t addr, bool readonly) {
	uint8_t data = 0x00;
	addr &= 0x3FFF;

	Cartridge *cart = bus ? bus->cart : nullptr;
	Mirroring mirroring = cart ? cart->mirroring_type : Mirroring::HORIZONTAL;
	int onescreen_bank = 0;
	if (cart && mirroring == Mirroring::SINGLE_SCREEN && cart->mapper) {
		int b = cart->mapper->get_onescreen_bank();
		if (b >= 0) onescreen_bank = b;
	}

	if (cart && cart->ppuRead(addr, data)) {
		return data;
	} else if (addr >= 0x0000 && addr <= 0x1FFF) {
		data = pattern_table[(addr & 0x1000) >> 12][addr & 0x0FFF];
	} else if (addr >= 0x2000 && addr <= 0x3EFF) {
		addr &= 0x0FFF;

		if (mirroring == Mirroring::VERTICAL) {
			if (addr <= 0x03FF)
				data = nametable[0][addr & 0x03FF];
			else if (addr <= 0x07FF)
				data = nametable[1][addr & 0x03FF];
			else if (addr <= 0x0BFF)
				data = nametable[0][addr & 0x03FF];
			else
				data = nametable[1][addr & 0x03FF];
		} else if (mirroring == Mirroring::HORIZONTAL) {
			if (addr <= 0x03FF)
				data = nametable[0][addr & 0x03FF];
			else if (addr <= 0x07FF)
				data = nametable[0][addr & 0x03FF];
			else if (addr <= 0x0BFF)
				data = nametable[1][addr & 0x03FF];
			else
				data = nametable[1][addr & 0x03FF];
		} else { // SINGLE_SCREEN
			// All nametable accesses map to the selected onescreen bank
			data = nametable[onescreen_bank][addr & 0x03FF];
		}
	} else if (addr >= 0x3F00 && addr <= 0x3FFF) {
		addr &= 0x001F;
		if (addr == 0x0010) addr = 0x0000;
		if (addr == 0x0014) addr = 0x0004;
		if (addr == 0x0018) addr = 0x0008;
		if (addr == 0x001C) addr = 0x000C;
		data = palette_table[addr] & (mask.grayscale ? 0x30 : 0x3F);
	}

	return data;
}

void PPU::ppuWrite(uint16_t addr, uint8_t data) {
	addr &= 0x3FFF;

	Cartridge *cart = bus ? bus->cart : nullptr;
	Mirroring mirroring = cart ? cart->mirroring_type : Mirroring::HORIZONTAL;
	int onescreen_bank = 0;
	if (cart && mirroring == Mirroring::SINGLE_SCREEN && cart->mapper) {
		int b = cart->mapper->get_onescreen_bank();
		if (b >= 0) onescreen_bank = b;
	}

	if (!cart) {
	    printf("PPU::ppuWrite: No cartridge loaded!\n");
		return;
	}

	cart->ppuWrite(addr, data);

	if (addr >= 0x0000 && addr <= 0x1FFF) {
		pattern_table[(addr & 0x1000) >> 12][addr & 0x0FFF] = data;
	}
	else if (addr >= 0x2000 && addr <= 0x3EFF) {
		addr &= 0x0FFF;
		if (mirroring == Mirroring::VERTICAL) {
			if (addr <= 0x03FF)
				nametable[0][addr & 0x03FF] = data;
			else if (addr <= 0x07FF)
				nametable[1][addr & 0x03FF] = data;
			else if (addr <= 0x0BFF)
				nametable[0][addr & 0x03FF] = data;
			else
				nametable[1][addr & 0x03FF] = data;
		}
		else if (mirroring == Mirroring::HORIZONTAL) {
			if (addr <= 0x03FF)
				nametable[0][addr & 0x03FF] = data;
			else if (addr <= 0x07FF)
				nametable[0][addr & 0x03FF] = data;
			else if (addr <= 0x0BFF)
				nametable[1][addr & 0x03FF] = data;
			else
				nametable[1][addr & 0x03FF] = data;
		}
		else { // SINGLE_SCREEN
			nametable[onescreen_bank][addr & 0x03FF] = data;
		}
	}
	else if (addr >= 0x3F00 && addr <= 0x3FFF) {
		addr &= 0x001F;
		if (addr == 0x0010) addr = 0x0000;
		if (addr == 0x0014) addr = 0x0004;
		if (addr == 0x0018) addr = 0x0008;
		if (addr == 0x001C) addr = 0x000C;
		palette_table[addr] = data;
	}
}

void PPU::reset() {
	fine_x = 0x00;
	address_latch = 0x00;
	ppu_data_buffer = 0x00;
	scanline = 0;
	cycle = 0;
	odd_frame = false;
	bg_next_tile_id = 0x00;
	bg_next_tile_attrib = 0x00;
	bg_next_tile_lsb = 0x00;
	bg_next_tile_msb = 0x00;
	bg_shifter_pattern_lo = 0x0000;
	bg_shifter_pattern_hi = 0x0000;
	bg_shifter_attrib_lo = 0x0000;
	bg_shifter_attrib_hi = 0x0000;
	status.reg = 0x00;
	mask.reg = 0x00;
	ctrl.reg = 0x00;
	nmi_line = false;
	nmi = false;
	vram_addr.reg = 0x0000;
	tram_addr.reg = 0x0000;
	oam_addr = 0x00;
	std::memset(OAM, 0, sizeof(OAM));
	std::memset(spriteScanline, 0, sizeof(spriteScanline));
	std::memset(sprite_shifter_pattern_lo, 0, sizeof(sprite_shifter_pattern_lo));
	std::memset(sprite_shifter_pattern_hi, 0, sizeof(sprite_shifter_pattern_hi));
	sprite_count = 0;
	sprite_zero_hit_possible = false;
	sprite_zero_being_rendered = false;
	sprite_zero_scanline = 0xFF;
}

void PPU::clock() {
	auto IncrementScrollX = [&]() {
		if (mask.show_bg || mask.show_sprite) {
			if (vram_addr.coarse_x == 31) {
				vram_addr.coarse_x = 0;
				vram_addr.nametable_x = ~vram_addr.nametable_x;
			} else {
				vram_addr.coarse_x++;
			}
		}
	};

	auto IncrementScrollY = [&]() {
		if (mask.show_bg || mask.show_sprite) {
			if (vram_addr.fine_y < 7) {
				vram_addr.fine_y++;
			} else {
				vram_addr.fine_y = 0;

				if (vram_addr.coarse_y == 29) {
					vram_addr.coarse_y = 0;
					vram_addr.nametable_y = ~vram_addr.nametable_y;
				}
				else if (vram_addr.coarse_y == 31) {
					vram_addr.coarse_y = 0;
				}
				else {
					vram_addr.coarse_y++;
				}
			}
		}
	};

	auto TransferAddressX = [&]() {
		if (mask.show_bg || mask.show_sprite) {
			vram_addr.nametable_x = tram_addr.nametable_x;
			vram_addr.coarse_x    = tram_addr.coarse_x;
		}
	};

	auto TransferAddressY = [&]()
	{
		if (mask.show_bg || mask.show_sprite) {
			vram_addr.fine_y      = tram_addr.fine_y;
			vram_addr.nametable_y = tram_addr.nametable_y;
			vram_addr.coarse_y    = tram_addr.coarse_y;
		}
	};

	auto LoadShifters = [&]() {
		bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
		bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;
		bg_shifter_attrib_lo  = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
		bg_shifter_attrib_hi  = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
	};

	auto UpdateShifters = [&]() {
		if (mask.show_bg) {
			bg_shifter_pattern_lo <<= 1;
			bg_shifter_pattern_hi <<= 1;

			bg_shifter_attrib_lo <<= 1;
			bg_shifter_attrib_hi <<= 1;
		}

		if (mask.show_sprite && cycle >= 1 && cycle < 258)
		{
			for (uint8_t i = 0; i < sprite_count; i++)
			{
				if (spriteScanline[i].x > 0)
				{
					spriteScanline[i].x--;
				}
				else
				{
					sprite_shifter_pattern_lo[i] <<= 1;
					sprite_shifter_pattern_hi[i] <<= 1;
				}
			}
		}
	};

	// All but 1 of the secanlines is visible to the user. The pre-render scanline
	// at -1, is used to configure the "shifters" for the first visible scanline, 0.
	if (scanline >= -1 && scanline < 240) {
		if (scanline == 0 && cycle == 0 && odd_frame && (mask.show_bg || mask.show_sprite)) {
			// "Odd Frame" cycle skip matches hardware behavior when rendering
			cycle = 1;
		}

		if (scanline == -1 && cycle == 1) {
			status.vblank = 0;
			status.sprite_zero_hit = 0;
			status.sprite_overflow = 0;
			update_nmi_line();
		}


		if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338)) {
			UpdateShifters();

			switch ((cycle - 1) % 8) {
			case 0:
				LoadShifters();
				bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
				break;
			case 2:
				bg_next_tile_attrib = ppuRead(0x23C0 | (vram_addr.nametable_y << 11)
					                                 | (vram_addr.nametable_x << 10)
					                                 | ((vram_addr.coarse_y >> 2) << 3)
					                                 | (vram_addr.coarse_x >> 2));
				if (vram_addr.coarse_y & 0x02) bg_next_tile_attrib >>= 4;
				if (vram_addr.coarse_x & 0x02) bg_next_tile_attrib >>= 2;
				bg_next_tile_attrib &= 0x03;
				break;

			case 4:
				bg_next_tile_lsb = ppuRead((ctrl.pattern_background << 12) + ((uint16_t)bg_next_tile_id << 4) + (vram_addr.fine_y) + 0);
				break;
			case 6:
				bg_next_tile_msb = ppuRead((ctrl.pattern_background << 12) + ((uint16_t)bg_next_tile_id << 4) + (vram_addr.fine_y) + 8);
				break;
			case 7:
				IncrementScrollX();
				break;
			}
		}

		if (cycle == 256) {
			IncrementScrollY();
		}

		if (cycle == 257) {
			LoadShifters();
			TransferAddressX();
		}

		if (cycle == 257 && scanline >= 0) {
			std::memset(spriteScanline, 0, sizeof(spriteScanline));
			std::memset(sprite_shifter_pattern_lo, 0, sizeof(sprite_shifter_pattern_lo));
			std::memset(sprite_shifter_pattern_hi, 0, sizeof(sprite_shifter_pattern_hi));
			sprite_count = 0;
			sprite_zero_hit_possible = false;
			sprite_zero_scanline = 0xFF;

			uint8_t sprite_height = ctrl.sprite_size ? 16 : 8;
			for (uint8_t i = 0; i < 64; i++) {
				int16_t diff = static_cast<int16_t>(scanline) - static_cast<int16_t>(OAM[i].y);
				if (diff >= 0 && diff < sprite_height) {
					if (sprite_count < 8) {
						spriteScanline[sprite_count] = OAM[i];
						sprite_shifter_pattern_lo[sprite_count] = 0;
						sprite_shifter_pattern_hi[sprite_count] = 0;
						if (i == 0) {
							sprite_zero_hit_possible = true;
							sprite_zero_scanline = sprite_count;
						}
						sprite_count++;
					} else {
						status.sprite_overflow = 1;
						break;
					}
				}
			}
		}

		if (cycle == 338 || cycle == 340) {
			bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
		}

		if (cycle == 340) {
    		static unsigned char rb_lookup[16] = {
                0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
                0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf
    		};

			auto reverse_byte = [](uint8_t value) -> uint8_t
			{
			    return (rb_lookup[value&0b1111] << 4) | rb_lookup[value >> 4];
			};

			for (uint8_t i = 0; i < sprite_count; i++) {
				uint8_t sprite_height = ctrl.sprite_size ? 16 : 8;
				uint16_t sprite_row = static_cast<uint16_t>(scanline - spriteScanline[i].y);

				if (spriteScanline[i].attribute & 0x80) {
					sprite_row = sprite_height - 1 - sprite_row;
				}

				uint16_t addr = 0;

				if (!ctrl.sprite_size) {
					addr = (ctrl.pattern_sprite << 12)
						+ (static_cast<uint16_t>(spriteScanline[i].id) << 4)
						+ sprite_row;
				} else {
					uint16_t base_table = (spriteScanline[i].id & 0x01) << 12;
					uint16_t tile = (spriteScanline[i].id & 0xFE);
					if (sprite_row > 7) {
						sprite_row -= 8;
						tile++;
					}
					addr = base_table + (tile << 4) + sprite_row;
				}

				uint8_t lo = ppuRead(addr + 0);
				uint8_t hi = ppuRead(addr + 8);

				if (spriteScanline[i].attribute & 0x40) {
					lo = reverse_byte(lo);
					hi = reverse_byte(hi);
				}

				sprite_shifter_pattern_lo[i] = lo;
				sprite_shifter_pattern_hi[i] = hi;
			}
		}

		if (scanline == -1 && cycle >= 280 && cycle < 305) {
			TransferAddressY();
		}
	}

	constexpr int VBLANK_START_SCANLINE = 241;
	if (scanline >= VBLANK_START_SCANLINE && scanline < 261)
	{
		if (scanline == VBLANK_START_SCANLINE && cycle == 1)
		{
			status.vblank = 1;
			update_nmi_line();
		}
	}

	uint8_t sprite_pixel = 0x00;
	uint8_t sprite_palette = 0x00;
	uint8_t sprite_priority = 0x00;

	uint8_t bg_pixel = 0x00;
	uint8_t bg_palette = 0x00;

	if (mask.show_bg) {
		uint16_t bit_mux = 0x8000 >> fine_x;
		uint8_t p0_pixel = (bg_shifter_pattern_lo & bit_mux) > 0;
		uint8_t p1_pixel = (bg_shifter_pattern_hi & bit_mux) > 0;
		bg_pixel = (p1_pixel << 1) | p0_pixel;
		uint8_t bg_pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
		uint8_t bg_pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
		bg_palette = (bg_pal1 << 1) | bg_pal0;
	}

	if (mask.show_sprite) {
		sprite_zero_being_rendered = false;
		for (uint8_t i = 0; i < sprite_count; i++) {
			if (spriteScanline[i].x == 0) {
				uint8_t fg_pixel_lo = (sprite_shifter_pattern_lo[i] & 0x80) > 0;
				uint8_t fg_pixel_hi = (sprite_shifter_pattern_hi[i] & 0x80) > 0;
				sprite_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;
				if (sprite_pixel != 0) {
					sprite_palette = (spriteScanline[i].attribute & 0x03) + 0x04;
					sprite_priority = (spriteScanline[i].attribute & 0x20) == 0;
					sprite_zero_being_rendered = (i == sprite_zero_scanline) && sprite_zero_hit_possible;
					break;
				}
			}
		}
	}

	if (!mask.show_bg_left && cycle < 9) {
		bg_pixel = 0;
		bg_palette = 0;
	}

	if (!mask.show_sprite_left && cycle < 9) {
		sprite_pixel = 0;
	}

	uint8_t pixel = 0x00;
	uint8_t palette = 0x00;

	if (bg_pixel == 0 && sprite_pixel == 0) {
		pixel = 0x00;
		palette = 0x00;
	} else if (bg_pixel == 0 && sprite_pixel > 0) {
		pixel = sprite_pixel;
		palette = sprite_palette;
	} else if (bg_pixel > 0 && sprite_pixel == 0) {
		pixel = bg_pixel;
		palette = bg_palette;
	} else {
		if (sprite_zero_being_rendered) {
			if (mask.show_bg && mask.show_sprite) {
				if ((mask.show_bg_left || cycle >= 9) && (mask.show_sprite_left || cycle >= 9)) {
					if (!status.sprite_zero_hit) {
						status.sprite_zero_hit = 1;
					}
				}
			}
		}

		if (sprite_priority) {
			pixel = sprite_pixel;
			palette = sprite_palette;
		} else {
			pixel = bg_pixel;
			palette = bg_palette;
		}
	}

	if (scanline >= 0 && scanline < 240 && cycle >= 1 && cycle <= 256) {
	    framebuffer[(scanline * 256) + (cycle - 1)] = get_color(palette, pixel);
	}

	cycle++;
	if (cycle >= 341) {
		cycle = 0;
		scanline++;
		if (scanline >= 261) {
			scanline = -1;
			frame_complete = true;
			odd_frame = !odd_frame;
		}
	}
}
