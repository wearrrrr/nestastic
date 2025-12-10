#include "002.h"

void Mapper_002::reset() {
	select_prg_lo = 0;
	select_prg_hi = nPRGBanks ? nPRGBanks - 1 : 0;
}

bool Mapper_002::prgRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
    if (addr >= 0x8000 && addr <= 0xBFFF)
	{
		mapped_addr = select_prg_lo * 0x4000 + (addr & 0x3FFF);
		return true;
	}

	if (addr >= 0xC000 && addr <= 0xFFFF)
	{
		mapped_addr = select_prg_hi * 0x4000 + (addr & 0x3FFF);
		return true;
	}

	return false;

}

bool Mapper_002::prgWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
	if (addr >= 0x8000 && addr <= 0xFFFF && nPRGBanks != 0) {
		select_prg_lo = data % nPRGBanks;
	}
    return false;
}

bool Mapper_002::chrRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr < 0x2000) {
		mapped_addr = addr;
		return true;
	} else
		return false;
}

bool Mapper_002::chrWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    if (addr < 0x2000) {
		if (nCHRBanks == 0) {
			mapped_addr = addr;
			cart->chr[mapped_addr] = data;
			return true;
		}
	}
	return false;
}
