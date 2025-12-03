#include "bus.h"

Bus::Bus(const char *rom_path) {
  cart = load_cartridge(rom_path);
  cpu.Connect(this);
  ppu.connect(this);
}

Bus::~Bus() { delete cart; }

uint8_t Bus::read(uint16_t addr) {
  uint8_t data = 0x00;

  if (cart && cart->cpuRead(addr, data))
    return data;

  if (addr <= 0x1FFF)
    return ram[addr & 0x07FF];

  if (addr >= 0x2000 && addr <= 0x3FFF)
    return ppu.cpuRead(addr & 0x0007);

  if (addr == 0x4016 || addr == 0x4017) {
    int controller = addr & 0x0001;
    uint8_t data_out = 0x40;
    if (controller_strobe & 0x01) {
      data_out |= controller_state[controller] & 0x01;
    } else {
      uint8_t bit = controller_shift[controller] & 0x01;
      data_out |= bit;
      controller_shift[controller] >>= 1;
      controller_shift[controller] |= 0x80;
    }
    return data_out;
  }

  // TODO: APU/IO handling
  return data;
}

void Bus::write(uint16_t addr, uint8_t value) {
  if (cart && cart->cpuWrite(addr, value))
    return;

  if (addr <= 0x1FFF) {
    ram[addr & 0x07FF] = value;
    return;
  }

  if (addr >= 0x2000 && addr <= 0x3FFF) {
    ppu.cpuWrite(addr & 0x0007, value);
    return;
  }

  if (addr == 0x4014) {
    dma_page = value;
    dma_addr = 0x00;
    dma_transfer = true;
    dma_dummy = true;
    return;
  }

  if (addr == 0x4016) {
    uint8_t prev_strobe = controller_strobe;
    controller_strobe = value & 0x01;

    if ((controller_strobe & 0x01) || ((prev_strobe & 0x01) && !(controller_strobe & 0x01))) {
      controller_shift[0] = controller_state[0];
      controller_shift[1] = controller_state[1];
    }
    return;
  }

  // TODO: APU and controller registers
}

void Bus::clock() {
  ppu.clock();

  if ((cycles % 3) == 0) {
    if (dma_transfer) {
      if (dma_dummy) {
        if ((cycles & 1) == 1) {
          dma_dummy = false;
        }
      } else {
        if ((cycles & 1) == 0) {
          uint16_t addr = (static_cast<uint16_t>(dma_page) << 8) | dma_addr;
          dma_data = read(addr);
        } else {
          ppu.dmaWrite(dma_data);
          dma_addr++;
          if (dma_addr == 0x00) {
            dma_transfer = false;
            dma_dummy = true;
          }
        }
      }
    } else {
      cpu.clock();
    }
  }

  if (ppu.nmi) {
    ppu.nmi = false;
    cpu.nmi();
  }

  cycles++;
}

void Bus::set_controller_button(int index, ControllerButton button, bool pressed) {
  if (index < 0 || index > 1)
    return;

  if (pressed) {
    controller_state[index] |= static_cast<uint8_t>(button);
  } else {
    controller_state[index] &= ~static_cast<uint8_t>(button);
  }

  if (controller_strobe & 0x01) {
    controller_shift[index] = controller_state[index];
  }
}
