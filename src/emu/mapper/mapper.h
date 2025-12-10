#pragma once
#include <cstdint>
#include "../cartridge/cartridge.h"

class Mapper {
public:
	Mapper(uint8_t prgBanks, uint8_t chrBanks);
	~Mapper();

public:

    virtual bool prgRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) = 0;
	virtual bool prgWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) = 0;

	virtual bool chrRead(uint16_t addr, uint32_t &mapped_addr)	 = 0;
	virtual bool chrWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0)	 = 0;

	virtual void reset() = 0;

	virtual int get_onescreen_bank() { return -1; };

protected:
	// These are stored locally as many of the mappers require this information
	uint8_t nPRGBanks = 0;
	uint8_t nCHRBanks = 0;
};
