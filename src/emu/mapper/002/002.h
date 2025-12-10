#include <cstdint>

#include "../mapper.h"
#include "../../cartridge/cartridge.h"

class Mapper_002 : public Mapper {
public:
    Mapper_002(Cartridge *cart, uint8_t prg_banks, uint8_t chr_banks) : Mapper(prg_banks, chr_banks), cart(cart) {
        reset();
    };
    ~Mapper_002() = default;

    virtual bool prgRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
    virtual bool prgWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) override;

    virtual bool chrRead(uint16_t addr, uint32_t &mapped_addr) override;
    virtual bool chrWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) override;

    void reset() override;

private:
    Cartridge *cart;
    uint8_t select_prg_lo = 0;
    uint8_t select_prg_hi = 0;
};
