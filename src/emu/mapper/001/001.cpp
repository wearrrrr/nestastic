#include "001.h"

bool Mapper_001::prgRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
		mapped_addr = 0xFFFFFFFF;
		data = vram[addr & 0x1FFF];
		return true;
	}

	if (addr >= 0x8000) {
		// PRG mode is stored in bits 2-3 of ctrl_reg. Use the full two-bit value
		// to determine whether we're in 32KB mode (0/1) or 16KB modes (2/3).
		uint8_t nPRGMode = (ctrl_reg >> 2) & 0x03;

		if (nPRGMode == 0 || nPRGMode == 1) {
			// 32KB PRG mode
			mapped_addr = prg.bank32 * 0x8000 + (addr & 0x7FFF);
			return true;
		} else {
			// 16KB PRG modes - map the two 16KB slots
			if (addr >= 0x8000 && addr <= 0xBFFF) {
				mapped_addr = prg.bank16Lo * 0x4000 + (addr & 0x3FFF);
				return true;
			}

			if (addr >= 0xC000 && addr <= 0xFFFF) {
				mapped_addr = prg.bank16Hi * 0x4000 + (addr & 0x3FFF);
				return true;
			}
		}
	}

	return false;
}

bool Mapper_001::prgWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
	if (addr >= 0x6000 && addr <= 0x7FFF) {
		// Write is to static ram on cartridge
		mapped_addr = 0xFFFFFFFF;

		// Write data to RAM
		vram[addr & 0x1FFF] = data;

		// Signal mapper has handled request
		return true;
	}

	if (addr >= 0x8000) {
		// reset serial loading
		if (data & 0x80) {
			load_register = 0x00;
			load_register_cnt = 0;
			ctrl_reg = ctrl_reg | 0x0C;
		} else {
			load_register >>= 1;
			load_register |= (data & 0x01) << 4;
			load_register_cnt++;

			if (load_register_cnt == 5) {
				// bits 13 and 14 have the mapper target register
				uint8_t nTargetRegister = (addr >> 13) & 0x03;

				// 0x8000 - 0x9FFF
				if (nTargetRegister == 0) {
					// Set Control Register
					ctrl_reg = load_register & 0x1F;

					// Decide which nametable mirroring mode to use.
					// For one-screen modes we record which bank (low=0, high=1)
					// in the member `onescreen_bank`, otherwise leave it unset (-1).
					switch (ctrl_reg & 0x03) {
    					case 0:	this->onescreen_bank = 0; break; // one-screen lower
    					case 1: this->onescreen_bank = 1; break; // one-screen higher
    					case 2: this->onescreen_bank = -1; break; // vertical
    					case 3:	this->onescreen_bank = -1; break; // horizontal
					}

					if (cart) {
						// Propagate the high-level mirroring choice to the cartridge.
						// Cartridge stores SINGLE_SCREEN (when appropriate) or the
						// standard vertical/horizontal mirroring enum.
						if ((ctrl_reg & 0x03) == 2) {
							cart->mirroring_type = Mirroring::VERTICAL;
						} else if ((ctrl_reg & 0x03) == 3) {
							cart->mirroring_type = Mirroring::HORIZONTAL;
						} else {
							cart->mirroring_type = Mirroring::SINGLE_SCREEN;
						}
					}
				} else if (nTargetRegister == 1) {
					// Set CHR Bank Lo
					if (ctrl_reg & 0b10000)
					{
						// 4K CHR Bank at PPU 0x0000
						chr.bank4Lo = load_register & 0x1F;
					}
					else
					{
						// 8K CHR Bank at PPU 0x0000
						chr.bank8 = load_register & 0x1E;
					}
				} else if (nTargetRegister == 2) {
					// Set CHR Bank Hi
					if (ctrl_reg & 0b10000)
					{
						// 4K CHR Bank at PPU 0x1000
						chr.bank4Hi = load_register & 0x1F;
					}
				} else if (nTargetRegister == 3) {
					// Configure PRG Banks
					uint8_t nPRGMode = (ctrl_reg >> 2) & 0x03;

					if (nPRGMode == 0 || nPRGMode == 1) {
						// Set 32K PRG Bank at CPU 0x8000
						prg.bank32 = (load_register & 0x0E) >> 1;
					} else if (nPRGMode == 2) {
						// Fix 16KB PRG Bank at CPU 0x8000 to First Bank
						prg.bank16Lo = 0;
						// Set 16KB PRG Bank at CPU 0xC000
						prg.bank16Hi = load_register & 0x0F;
					} else if (nPRGMode == 3) {
						// Set 16KB PRG Bank at CPU 0x8000
						prg.bank16Lo = load_register & 0x0F;
						// Fix 16KB PRG Bank at CPU 0xC000 to Last Bank
						prg.bank16Hi = nPRGBanks - 1;
					}
				}

				// 5 bits were written, and decoded, so
				// reset load register
				load_register = 0x00;
				load_register_cnt = 0;
			}
		}
	}

	// Mapper has handled write, but do not update ROMs
	return false;
}

bool Mapper_001::chrRead(uint16_t addr, uint32_t &mapped_addr) {
	if (addr < 0x2000) {
		if (nCHRBanks == 0) {
			mapped_addr = addr;
			return true;
		} else {
			if (ctrl_reg & 0b10000) {
				// 4K CHR Bank Mode
				if (addr >= 0x0000 && addr <= 0x0FFF) {
					mapped_addr = chr.bank4Lo * 0x1000 + (addr & 0x0FFF);
					return true;
				}

				if (addr >= 0x1000 && addr <= 0x1FFF) {
					mapped_addr = chr.bank4Hi * 0x1000 + (addr & 0x0FFF);
					return true;
				}
			} else {
				// 8K CHR Bank Mode
				mapped_addr = chr.bank8 * 0x2000 + (addr & 0x1FFF);
				return true;
			}
		}
	}

	return false;
}

bool Mapper_001::chrWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data)
{
	if (addr < 0x2000) {
		// If there are no CHR banks the cartridge uses CHR RAM (writable).
		if (nCHRBanks == 0) {
			mapped_addr = addr;
			cart->chr[mapped_addr] = data;
			return true;
		}

		// CHR ROM is not writable.
		return false;
	}
	else
		return false;
}

int Mapper_001::get_onescreen_bank() {
	return onescreen_bank;
}

void Mapper_001::reset() {
	ctrl_reg = 0x1C;
	load_register = 0x00;
	load_register_cnt = 0x00;

	// Initialize 8KB of cartridge battery-backed/static RAM used at 0x6000-0x7FFF.
	// Mapper code expects `vram` to be sized before reads/writes.
	vram.assign(0x2000, 0x00);

	/* Do not set mapper-local mirroring state at reset.
	   onescreen_bank remains -1 (unset) and the cartridge mirroring
	   will be updated when the control register is written via prgWrite. */

	chr.bank4Lo = 0;
	chr.bank4Hi = 0;
	chr.bank8 = 0;

	prg.bank32 = 0;
	prg.bank16Lo = 0;
	prg.bank16Hi = nPRGBanks - 1;
}
