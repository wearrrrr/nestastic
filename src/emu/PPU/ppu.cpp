#include "ppu.h"
#include "../bus/bus.h"
#include <cstring>
#include <cstdio>
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

uint32_t PPU::GetColorFromPaletteRam(uint8_t palette, uint8_t pixel)
{
	uint8_t index = ppuRead(0x3F00 + (palette << 2) + pixel) & 0x3F;
	uint32_t color = nesPalette[index];
	uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
	uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
	uint8_t b = static_cast<uint8_t>(color & 0xFF);

	if (mask.emphasize_red || mask.emphasize_green || mask.emphasize_blue)
	{
		auto apply_emphasis = [](uint8_t channel, bool emphasize) -> uint8_t
		{
			float value = static_cast<float>(channel);
			const float boost = emphasize ? 1.15f : 0.92f;
			value *= boost;
			value = std::clamp(value, 0.0f, 255.0f);
			return static_cast<uint8_t>(value);
		};

		r = apply_emphasis(r, mask.emphasize_red);
		g = apply_emphasis(g, mask.emphasize_green);
		b = apply_emphasis(b, mask.emphasize_blue);
	}

	return (static_cast<uint32_t>(r) << 16) |
	       (static_cast<uint32_t>(g) << 8)  |
	        static_cast<uint32_t>(b);
}


uint8_t PPU::cpuRead(uint16_t addr, bool rdonly)
{
	uint8_t data = 0x00;

	if (rdonly)
	{
		if (addr == 0x0000) {
		    data = ctrl.reg;
		} else if (addr == 0x0001) {
		    data = mask.reg;
		} else if (addr == 0x0002) {
		    data = status.reg;
		}
	}
	else
	{
		if (addr == 0x0002) {
			data = (status.reg & 0xE0) | (ppu_data_buffer & 0x1F);
			status.vblank = 0;
			address_latch = 0;
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

void PPU::cpuWrite(uint16_t addr, uint8_t data)
{
	switch (addr)
	{
	case 0x0000: // Control
		ctrl.reg = data;
		tram_addr.nametable_x = ctrl.nametable_x;
		tram_addr.nametable_y = ctrl.nametable_y;
		static int ctrl_logs = 0;
		if (ctrl_logs < 10)
		{
			std::printf("PPUCTRL write: %02X\n", data);
			ctrl_logs++;
		}
		break;
	case 0x0001: // Mask
		mask.reg = data;
		static int mask_logs = 0;
		if (mask_logs < 10)
		{
			std::printf("PPUMASK write: %02X\n", data);
			mask_logs++;
		}
		break;
	case 0x0002: // Status
		break;
	case 0x0003: // OAM Address
		oam_addr = data;
		break;
	case 0x0004: // OAM Data
		dmaWrite(data);
		break;
	case 0x0005: // Scroll
		if (address_latch == 0)
		{
			// First write to scroll register contains X offset in pixel space
			// which we split into coarse and fine x values
			fine_x = data & 0x07;
			tram_addr.coarse_x = data >> 3;
			address_latch = 1;
		}
		else
		{
			// First write to scroll register contains Y offset in pixel space
			// which we split into coarse and fine Y values
			tram_addr.fine_y = data & 0x07;
			tram_addr.coarse_y = data >> 3;
			address_latch = 0;
		}
		break;
	case 0x0006: // PPU Address
		if (address_latch == 0)
		{
			// PPU address bus can be accessed by CPU via the ADDR and DATA
			// registers. The fisrt write to this register latches the high byte
			// of the address, the second is the low byte. Note the writes
			// are stored in the tram register...
			tram_addr.reg = (uint16_t)((data & 0x3F) << 8) | (tram_addr.reg & 0x00FF);
			address_latch = 1;
		}
		else
		{
			// ...when a whole address has been written, the internal vram address
			// buffer is updated. Writing to the PPU is unwise during rendering
			// as the PPU will maintam the vram address automatically whilst
			// rendering the scanline position.
			tram_addr.reg = (tram_addr.reg & 0xFF00) | data;
			vram_addr = tram_addr;
			address_latch = 0;
		}
		break;
	case 0x0007: // PPU Data
		ppuWrite(vram_addr.reg, data);
		// All writes from PPU data automatically increment the nametable
		// address depending upon the mode set in the control register.
		// If set to vertical mode, the increment is 32, so it skips
		// one whole nametable row; in horizontal mode it just increments
		// by 1, moving to the next column
		vram_addr.reg += (ctrl.increment ? 32 : 1);
		break;
	}
}

uint8_t PPU::oamRead(uint8_t addr) const
{
	const uint8_t *bytes = reinterpret_cast<const uint8_t*>(OAM);
	return bytes[addr];
}

void PPU::oamWrite(uint8_t addr, uint8_t data)
{
	uint8_t *bytes = reinterpret_cast<uint8_t*>(OAM);
	bytes[addr] = data;
}

void PPU::dmaWrite(uint8_t data)
{
	oamWrite(oam_addr, data);
	oam_addr++;
}

uint8_t PPU::ppuRead(uint16_t addr, bool readonly)
{
	uint8_t data = 0x00;
	addr &= 0x3FFF;

	Cartridge *cart = bus ? bus->cart : nullptr;
	Cartridge::Mirroring mirroring = cart ? cart->mirroring_type : Cartridge::Mirroring::HORIZONTAL;

	if (cart && cart->ppuRead(addr, data))
	{

	}
	else if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		data = pattern_table[(addr & 0x1000) >> 12][addr & 0x0FFF];
	}
	else if (addr >= 0x2000 && addr <= 0x3EFF)
	{
		addr &= 0x0FFF;

		if (mirroring == Cartridge::Mirroring::VERTICAL)
		{
			if (addr <= 0x03FF)
				data = nametable[0][addr & 0x03FF];
			else if (addr <= 0x07FF)
				data = nametable[1][addr & 0x03FF];
			else if (addr <= 0x0BFF)
				data = nametable[0][addr & 0x03FF];
			else
				data = nametable[1][addr & 0x03FF];
		}
		else
		{
			if (addr <= 0x03FF)
				data = nametable[0][addr & 0x03FF];
			else if (addr <= 0x07FF)
				data = nametable[0][addr & 0x03FF];
			else if (addr <= 0x0BFF)
				data = nametable[1][addr & 0x03FF];
			else
				data = nametable[1][addr & 0x03FF];
		}
	}
	else if (addr >= 0x3F00 && addr <= 0x3FFF)
	{
		addr &= 0x001F;
		if (addr == 0x0010) addr = 0x0000;
		if (addr == 0x0014) addr = 0x0004;
		if (addr == 0x0018) addr = 0x0008;
		if (addr == 0x001C) addr = 0x000C;
		data = palette_table[addr] & (mask.grayscale ? 0x30 : 0x3F);
	}

	return data;
}

void PPU::ppuWrite(uint16_t addr, uint8_t data)
{
	addr &= 0x3FFF;

	Cartridge *cart = bus ? bus->cart : nullptr;
	Cartridge::Mirroring mirroring = cart ? cart->mirroring_type : Cartridge::Mirroring::HORIZONTAL;

	if (cart && cart->ppuWrite(addr, data))
	{

	}
	else if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		pattern_table[(addr & 0x1000) >> 12][addr & 0x0FFF] = data;
	}
	else if (addr >= 0x2000 && addr <= 0x3EFF)
	{
		addr &= 0x0FFF;
		if (mirroring == Cartridge::Mirroring::VERTICAL)
		{
			if (addr <= 0x03FF)
				nametable[0][addr & 0x03FF] = data;
			else if (addr <= 0x07FF)
				nametable[1][addr & 0x03FF] = data;
			else if (addr <= 0x0BFF)
				nametable[0][addr & 0x03FF] = data;
			else
				nametable[1][addr & 0x03FF] = data;
		}
		else
		{
			if (addr <= 0x03FF)
				nametable[0][addr & 0x03FF] = data;
			else if (addr <= 0x07FF)
				nametable[0][addr & 0x03FF] = data;
			else if (addr <= 0x0BFF)
				nametable[1][addr & 0x03FF] = data;
			else
				nametable[1][addr & 0x03FF] = data;
		}
	}
	else if (addr >= 0x3F00 && addr <= 0x3FFF)
	{
		addr &= 0x001F;
		if (addr == 0x0010) addr = 0x0000;
		if (addr == 0x0014) addr = 0x0004;
		if (addr == 0x0018) addr = 0x0008;
		if (addr == 0x001C) addr = 0x000C;
		palette_table[addr] = data;
	}
}

void PPU::reset()
{
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
}

void PPU::clock()
{
	// As we progress through scanlines and cycles, the PPU is effectively
	// a state machine going through the motions of fetching background
	// information and sprite information, compositing them into a pixel
	// to be output.

	// The lambda functions (functions inside functions) contain the various
	// actions to be performed depending upon the output of the state machine
	// for a given scanline/cycle combination

	// ==============================================================================
	// Increment the background tile "pointer" one tile/column horizontally
	auto IncrementScrollX = [&]()
	{
		// Note: pixel perfect scrolling horizontally is handled by the
		// data shifters. Here we are operating in the spatial domain of
		// tiles, 8x8 pixel blocks.

		// Ony if rendering is enabled
		if (mask.show_bg || mask.show_spr)
		{
			// A single name table is 32x30 tiles. As we increment horizontally
			// we may cross into a neighbouring nametable, or wrap around to
			// a neighbouring nametable
			if (vram_addr.coarse_x == 31)
			{
				// Leaving nametable so wrap address round
				vram_addr.coarse_x = 0;
				// Flip target nametable bit
				vram_addr.nametable_x = ~vram_addr.nametable_x;
			}
			else
			{
				// Staying in current nametable, so just increment
				vram_addr.coarse_x++;
			}
		}
	};

	// ==============================================================================
	// Increment the background tile "pointer" one scanline vertically
	auto IncrementScrollY = [&]()
	{
		// Incrementing vertically is more complicated. The visible nametable
		// is 32x30 tiles, but in memory there is enough room for 32x32 tiles.
		// The bottom two rows of tiles are in fact not tiles at all, they
		// contain the "attribute" information for the entire table. This is
		// information that describes which palettes are used for different
		// regions of the nametable.

		// In addition, the NES doesnt scroll vertically in chunks of 8 pixels
		// i.e. the height of a tile, it can perform fine scrolling by using
		// the fine_y component of the register. This means an increment in Y
		// first adjusts the fine offset, but may need to adjust the whole
		// row offset, since fine_y is a value 0 to 7, and a row is 8 pixels high

		// Ony if rendering is enabled
		if (mask.show_bg || mask.show_spr)
		{
			// If possible, just increment the fine y offset
			if (vram_addr.fine_y < 7)
			{
				vram_addr.fine_y++;
			}
			else
			{
				// If we have gone beyond the height of a row, we need to
				// increment the row, potentially wrapping into neighbouring
				// vertical nametables. Dont forget however, the bottom two rows
				// do not contain tile information. The coarse y offset is used
				// to identify which row of the nametable we want, and the fine
				// y offset is the specific "scanline"

				// Reset fine y offset
				vram_addr.fine_y = 0;

				// Check if we need to swap vertical nametable targets
				if (vram_addr.coarse_y == 29)
				{
					// We do, so reset coarse y offset
					vram_addr.coarse_y = 0;
					// And flip the target nametable bit
					vram_addr.nametable_y = ~vram_addr.nametable_y;
				}
				else if (vram_addr.coarse_y == 31)
				{
					// In case the pointer is in the attribute memory, we
					// just wrap around the current nametable
					vram_addr.coarse_y = 0;
				}
				else
				{
					// None of the above boundary/wrapping conditions apply
					// so just increment the coarse y offset
					vram_addr.coarse_y++;
				}
			}
		}
	};

	// ==============================================================================
	// Transfer the temporarily stored horizontal nametable access information
	// into the "pointer". Note that fine x scrolling is not part of the "pointer"
	// addressing mechanism
	auto TransferAddressX = [&]()
	{
		// Ony if rendering is enabled
		if (mask.show_bg || mask.show_spr)
		{
			vram_addr.nametable_x = tram_addr.nametable_x;
			vram_addr.coarse_x    = tram_addr.coarse_x;
		}
	};

	// ==============================================================================
	// Transfer the temporarily stored vertical nametable access information
	// into the "pointer". Note that fine y scrolling is part of the "pointer"
	// addressing mechanism
	auto TransferAddressY = [&]()
	{
		// Ony if rendering is enabled
		if (mask.show_bg || mask.show_spr)
		{
			vram_addr.fine_y      = tram_addr.fine_y;
			vram_addr.nametable_y = tram_addr.nametable_y;
			vram_addr.coarse_y    = tram_addr.coarse_y;
		}
	};


	// ==============================================================================
	// Prime the "in-effect" background tile shifters ready for outputting next
	// 8 pixels in scanline.
	auto LoadBackgroundShifters = [&]()
	{
		// Each PPU update we calculate one pixel. These shifters shift 1 bit along
		// feeding the pixel compositor with the binary information it needs. Its
		// 16 bits wide, because the top 8 bits are the current 8 pixels being drawn
		// and the bottom 8 bits are the next 8 pixels to be drawn. Naturally this means
		// the required bit is always the MSB of the shifter. However, "fine x" scrolling
		// plays a part in this too, whcih is seen later, so in fact we can choose
		// any one of the top 8 bits.
		bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
		bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;

		// Attribute bits do not change per pixel, rather they change every 8 pixels
		// but are synchronised with the pattern shifters for convenience, so here
		// we take the bottom 2 bits of the attribute word which represent which
		// palette is being used for the current 8 pixels and the next 8 pixels, and
		// "inflate" them to 8 bit words.
		bg_shifter_attrib_lo  = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
		bg_shifter_attrib_hi  = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
	};


	// ==============================================================================
	// Every cycle the shifters storing pattern and attribute information shift
	// their contents by 1 bit. This is because every cycle, the output progresses
	// by 1 pixel. This means relatively, the state of the shifter is in sync
	// with the pixels being drawn for that 8 pixel section of the scanline.
	auto UpdateShifters = [&]()
	{
		if (mask.show_bg)
		{
			// Shifting background tile pattern row
			bg_shifter_pattern_lo <<= 1;
			bg_shifter_pattern_hi <<= 1;

			// Shifting palette attributes by 1
			bg_shifter_attrib_lo <<= 1;
			bg_shifter_attrib_hi <<= 1;
		}

		if (mask.show_spr && cycle >= 1 && cycle < 258)
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
	if (scanline >= -1 && scanline < 240)
	{
		if (scanline == 0 && cycle == 0 && odd_frame && (mask.show_bg || mask.show_spr))
		{
			// "Odd Frame" cycle skip matches hardware behavior when rendering
			cycle = 1;
		}

		if (scanline == -1 && cycle == 1)
		{
			// Effectively start of new frame, so clear vertical blank flag
			status.vblank = 0;
			status.sprite_zero_hit = 0;
			status.sprite_overflow = 0;
		}


		if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338))
		{
			UpdateShifters();


			// In these cycles we are collecting and working with visible data
			// The "shifters" have been preloaded by the end of the previous
			// scanline with the data for the start of this scanline. Once we
			// leave the visible region, we go dormant until the shifters are
			// preloaded for the next scanline.

			// Fortunately, for background rendering, we go through a fairly
			// repeatable sequence of events, every 2 clock cycles.
			switch ((cycle - 1) % 8)
			{
			case 0:
				// Load the current background tile pattern and attributes into the "shifter"
				LoadBackgroundShifters();

				// Fetch the next background tile ID
				// "(vram_addr.reg & 0x0FFF)" : Mask to 12 bits that are relevant
				// "| 0x2000"                 : Offset into nametable space on PPU address bus
				bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));

				// Explanation:
				// The bottom 12 bits of the loopy register provide an index into
				// the 4 nametables, regardless of nametable mirroring configuration.
				// nametable_y(1) nametable_x(1) coarse_y(5) coarse_x(5)
				//
				// Consider a single nametable is a 32x32 array, and we have four of them
				//   0                1
				// 0 +----------------+----------------+
				//   |                |                |
				//   |                |                |
				//   |    (32x32)     |    (32x32)     |
				//   |                |                |
				//   |                |                |
				// 1 +----------------+----------------+
				//   |                |                |
				//   |                |                |
				//   |    (32x32)     |    (32x32)     |
				//   |                |                |
				//   |                |                |
				//   +----------------+----------------+
				//
				// This means there are 4096 potential locations in this array, which
				// just so happens to be 2^12!
				break;
			case 2:
				// Fetch the next background tile attribute. OK, so this one is a bit
				// more involved :P

				// Recall that each nametable has two rows of cells that are not tile
				// information, instead they represent the attribute information that
				// indicates which palettes are applied to which area on the screen.
				// Importantly (and frustratingly) there is not a 1 to 1 correspondance
				// between background tile and palette. Two rows of tile data holds
				// 64 attributes. Therfore we can assume that the attributes affect
				// 8x8 zones on the screen for that nametable. Given a working resolution
				// of 256x240, we can further assume that each zone is 32x32 pixels
				// in screen space, or 4x4 tiles. Four system palettes are allocated
				// to background rendering, so a palette can be specified using just
				// 2 bits. The attribute byte therefore can specify 4 distinct palettes.
				// Therefore we can even further assume that a single palette is
				// applied to a 2x2 tile combination of the 4x4 tile zone. The very fact
				// that background tiles "share" a palette locally is the reason why
				// in some games you see distortion in the colours at screen edges.

				// As before when choosing the tile ID, we can use the bottom 12 bits of
				// the loopy register, but we need to make the implementation "coarser"
				// because instead of a specific tile, we want the attribute byte for a
				// group of 4x4 tiles, or in other words, we divide our 32x32 address
				// by 4 to give us an equivalent 8x8 address, and we offset this address
				// into the attribute section of the target nametable.

				// Reconstruct the 12 bit loopy address into an offset into the
				// attribute memory

				// "(vram_addr.coarse_x >> 2)"        : integer divide coarse x by 4,
				//                                      from 5 bits to 3 bits
				// "((vram_addr.coarse_y >> 2) << 3)" : integer divide coarse y by 4,
				//                                      from 5 bits to 3 bits,
				//                                      shift to make room for coarse x

				// Result so far: YX00 00yy yxxx

				// All attribute memory begins at 0x03C0 within a nametable, so OR with
				// result to select target nametable, and attribute byte offset. Finally
				// OR with 0x2000 to offset into nametable address space on PPU bus.
				bg_next_tile_attrib = ppuRead(0x23C0 | (vram_addr.nametable_y << 11)
					                                 | (vram_addr.nametable_x << 10)
					                                 | ((vram_addr.coarse_y >> 2) << 3)
					                                 | (vram_addr.coarse_x >> 2));

				// Right we've read the correct attribute byte for a specified address,
				// but the byte itself is broken down further into the 2x2 tile groups
				// in the 4x4 attribute zone.

				// The attribute byte is assembled thus: BR(76) BL(54) TR(32) TL(10)
				//
				// +----+----+			    +----+----+
				// | TL | TR |			    | ID | ID |
				// +----+----+ where TL =   +----+----+
				// | BL | BR |			    | ID | ID |
				// +----+----+			    +----+----+
				//
				// Since we know we can access a tile directly from the 12 bit address, we
				// can analyse the bottom bits of the coarse coordinates to provide us with
				// the correct offset into the 8-bit word, to yield the 2 bits we are
				// actually interested in which specifies the palette for the 2x2 group of
				// tiles. We know if "coarse y % 4" < 2 we are in the top half else bottom half.
				// Likewise if "coarse x % 4" < 2 we are in the left half else right half.
				// Ultimately we want the bottom two bits of our attribute word to be the
				// palette selected. So shift as required...
				if (vram_addr.coarse_y & 0x02) bg_next_tile_attrib >>= 4;
				if (vram_addr.coarse_x & 0x02) bg_next_tile_attrib >>= 2;
				bg_next_tile_attrib &= 0x03;
				break;

				// Compared to the last two, the next two are the easy ones... :P

			case 4:
				// Fetch the next background tile LSB bit plane from the pattern memory
				// The Tile ID has been read from the nametable. We will use this id to
				// index into the pattern memory to find the correct sprite (assuming
				// the sprites lie on 8x8 pixel boundaries in that memory, which they do
				// even though 8x16 sprites exist, as background tiles are always 8x8).
				//
				// Since the sprites are effectively 1 bit deep, but 8 pixels wide, we
				// can represent a whole sprite row as a single byte, so offsetting
				// into the pattern memory is easy. In total there is 8KB so we need a
				// 13 bit address.

				// "(control.pattern_background << 12)"  : the pattern memory selector
				//                                         from control register, either 0K
				//                                         or 4K offset
				// "((uint16_t)bg_next_tile_id << 4)"    : the tile id multiplied by 16, as
				//                                         2 lots of 8 rows of 8 bit pixels
				// "(vram_addr.fine_y)"                  : Offset into which row based on
				//                                         vertical scroll offset
				// "+ 0"                                 : Mental clarity for plane offset
				// Note: No PPU address bus offset required as it starts at 0x0000
				bg_next_tile_lsb = ppuRead((ctrl.pattern_background << 12)
					                       + ((uint16_t)bg_next_tile_id << 4)
					                       + (vram_addr.fine_y) + 0);

				break;
			case 6:
				// Fetch the next background tile MSB bit plane from the pattern memory
				// This is the same as above, but has a +8 offset to select the next bit plane
				bg_next_tile_msb = ppuRead((ctrl.pattern_background << 12)
					                       + ((uint16_t)bg_next_tile_id << 4)
					                       + (vram_addr.fine_y) + 8);
				break;
			case 7:
				// Increment the background tile "pointer" to the next tile horizontally
				// in the nametable memory. Note this may cross nametable boundaries which
				// is a little complex, but essential to implement scrolling
				IncrementScrollX();
				break;
			}
		}

		// End of a visible scanline, so increment downwards...
		if (cycle == 256)
		{
			IncrementScrollY();
		}

		//...and reset the x position
		if (cycle == 257)
		{
			LoadBackgroundShifters();
			TransferAddressX();
		}

		if (cycle == 257 && scanline >= 0)
		{
			std::memset(spriteScanline, 0, sizeof(spriteScanline));
			std::memset(sprite_shifter_pattern_lo, 0, sizeof(sprite_shifter_pattern_lo));
			std::memset(sprite_shifter_pattern_hi, 0, sizeof(sprite_shifter_pattern_hi));
			sprite_count = 0;
			sprite_zero_hit_possible = false;

			uint8_t sprite_height = ctrl.sprite_size ? 16 : 8;

			for (uint8_t i = 0; i < 64; i++)
			{
				int16_t diff = static_cast<int16_t>(scanline) - static_cast<int16_t>(OAM[i].y);
				if (diff >= 0 && diff < sprite_height)
				{
					if (sprite_count < 8)
					{
						if (i == 0)
							sprite_zero_hit_possible = true;

						spriteScanline[sprite_count] = OAM[i];
						sprite_shifter_pattern_lo[sprite_count] = 0;
						sprite_shifter_pattern_hi[sprite_count] = 0;
						sprite_count++;
					}
					else
					{
						status.sprite_overflow = 1;
						break;
					}
				}
			}
		}

		// Superfluous reads of tile id at end of scanline
		if (cycle == 338 || cycle == 340)
		{
			bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
		}

		if (cycle == 340)
		{
			auto reverse_byte = [](uint8_t value) -> uint8_t
			{
				value = (value & 0xF0) >> 4 | (value & 0x0F) << 4;
				value = (value & 0xCC) >> 2 | (value & 0x33) << 2;
				value = (value & 0xAA) >> 1 | (value & 0x55) << 1;
				return value;
			};

			for (uint8_t i = 0; i < sprite_count; i++)
			{
				uint8_t sprite_height = ctrl.sprite_size ? 16 : 8;
				uint16_t sprite_row = static_cast<uint16_t>(scanline - spriteScanline[i].y);

				if (spriteScanline[i].attribute & 0x80)
				{
					sprite_row = sprite_height - 1 - sprite_row;
				}

				uint16_t addr = 0;

				if (!ctrl.sprite_size)
				{
					addr = (ctrl.pattern_sprite << 12)
						+ (static_cast<uint16_t>(spriteScanline[i].id) << 4)
						+ sprite_row;
				}
				else
				{
					uint16_t base_table = (spriteScanline[i].id & 0x01) << 12;
					uint16_t tile = (spriteScanline[i].id & 0xFE);
					if (sprite_row > 7)
					{
						sprite_row -= 8;
						tile++;
					}
					addr = base_table + (tile << 4) + sprite_row;
				}

				uint8_t lo = ppuRead(addr + 0);
				uint8_t hi = ppuRead(addr + 8);

				if (spriteScanline[i].attribute & 0x40)
				{
					lo = reverse_byte(lo);
					hi = reverse_byte(hi);
				}

				sprite_shifter_pattern_lo[i] = lo;
				sprite_shifter_pattern_hi[i] = hi;
			}
		}

		if (scanline == -1 && cycle >= 280 && cycle < 305)
		{
			// End of vertical blank period so reset the Y address ready for rendering
			TransferAddressY();
		}
	}

	if (scanline == 240)
	{
		// Post Render Scanline - Do Nothing!
	}

	if (scanline >= 241 && scanline < 261)
	{
		if (scanline == 241 && cycle == 1)
		{
			// Effectively end of frame, so set vertical blank flag
			status.vblank = 1;

			// If the control register tells us to emit a NMI when
			// entering vertical blanking period, do it! The CPU
			// will be informed that rendering is complete so it can
			// perform operations with the PPU knowing it wont
			// produce visible artefacts
			if (ctrl.enable_nmi)
				nmi = true;
		}
	}



	// Composition - Combine background and sprite pixels for this cycle
	uint8_t bg_pixel = 0x00;
	uint8_t bg_palette = 0x00;
	uint8_t sprite_pixel = 0x00;
	uint8_t sprite_palette = 0x00;
	uint8_t sprite_priority = 0x00;

	if (mask.show_bg)
	{
		uint16_t bit_mux = 0x8000 >> fine_x;
		uint8_t p0_pixel = (bg_shifter_pattern_lo & bit_mux) > 0;
		uint8_t p1_pixel = (bg_shifter_pattern_hi & bit_mux) > 0;
		bg_pixel = (p1_pixel << 1) | p0_pixel;
		uint8_t bg_pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
		uint8_t bg_pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
		bg_palette = (bg_pal1 << 1) | bg_pal0;
	}

	if (mask.show_spr)
	{
		sprite_zero_being_rendered = false;
		for (uint8_t i = 0; i < sprite_count; i++)
		{
			if (spriteScanline[i].x == 0)
			{
				uint8_t fg_pixel_lo = (sprite_shifter_pattern_lo[i] & 0x80) > 0;
				uint8_t fg_pixel_hi = (sprite_shifter_pattern_hi[i] & 0x80) > 0;
				sprite_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;

				if (sprite_pixel != 0)
				{
					sprite_palette = (spriteScanline[i].attribute & 0x03) + 0x04;
					sprite_priority = (spriteScanline[i].attribute & 0x20) == 0;
					sprite_zero_being_rendered = (i == 0) && sprite_zero_hit_possible;
					break;
				}
			}
		}
	}

	if (!mask.show_bg_left && cycle < 9)
	{
		bg_pixel = 0;
		bg_palette = 0;
	}

	if (!mask.show_spr_left && cycle < 9)
	{
		sprite_pixel = 0;
	}

	uint8_t pixel = 0x00;
	uint8_t palette = 0x00;

	if (bg_pixel == 0 && sprite_pixel == 0)
	{
		pixel = 0x00;
		palette = 0x00;
	}
	else if (bg_pixel == 0 && sprite_pixel > 0)
	{
		pixel = sprite_pixel;
		palette = sprite_palette;
	}
	else if (bg_pixel > 0 && sprite_pixel == 0)
	{
		pixel = bg_pixel;
		palette = bg_palette;
	}
	else
	{
		if (sprite_zero_being_rendered)
		{
			if (mask.show_bg && mask.show_spr)
			{
				if ((mask.show_bg_left || cycle >= 9) && (mask.show_spr_left || cycle >= 9))
				{
					if (!status.sprite_zero_hit)
					{
						status.sprite_zero_hit = 1;
						std::printf("sprite zero hit at scanline %d cycle %d\n", scanline, cycle);
					}
				}
			}
		}

		if (sprite_priority)
		{
			pixel = sprite_pixel;
			palette = sprite_palette;
		}
		else
		{
			pixel = bg_pixel;
			palette = bg_palette;
		}
	}

	if (scanline >= 0 && scanline < 240 && cycle >= 1 && cycle <= 256)
		framebuffer[(scanline * 256) + (cycle - 1)] = GetColorFromPaletteRam(palette, pixel);

	// Fake some noise for now
	//sprScreen.SetPixel(cycle - 1, scanline, palScreen[(rand() % 2) ? 0x3F : 0x30]);

	// Advance renderer - it never stops, it's relentless
	cycle++;
	if (cycle >= 341)
	{
		cycle = 0;
		scanline++;
		if (scanline >= 261)
		{
			scanline = -1;
			frame_complete = true;
			odd_frame = !odd_frame;
		}
	}
}
