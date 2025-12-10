#include <cstdint>

#include "../mapper.h"
#include "../../cartridge/cartridge.h"

class Mapper_001 : public Mapper {
public:
    Mapper_001(Cartridge *cart, uint8_t prg_banks, uint8_t chr_banks) : Mapper(prg_banks, chr_banks), cart(cart) {
        this->cart = cart;
        reset();
    };
    ~Mapper_001() = default;

    virtual bool prgRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
	virtual bool prgWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;

	virtual bool chrRead(uint16_t addr, uint32_t &mapped_addr) override;
	virtual bool chrWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;

    // Returns the onescreen bank selected by the mapper when the cartridge
    // mirroring type is SINGLE_SCREEN. Return 0 for lower, 1 for higher, -1 if not applicable.
    virtual int get_onescreen_bank() override;

    void reset() override;

private:
    Cartridge *cart;


    struct {
        uint8_t bank4Lo;
        uint8_t bank4Hi;
        uint8_t bank8;
    } chr;

    struct {
        uint8_t bank16Lo;
        uint8_t bank16Hi;
        uint8_t bank32;
    } prg;

	uint8_t load_register = 0x00;
    uint8_t load_register_cnt = 0x00;
    uint8_t ctrl_reg = 0x00;
    // Selected onescreen bank when cartridge mirroring is SINGLE_SCREEN.
    // 0 = lower nametable, 1 = higher nametable, -1 = not set/applicable.
    int onescreen_bank = -1;
    std::vector<uint8_t> vram;
};
