#include "bus.h"
#include <cstring>

Bus::Bus(const char *rom_path) {
  cart = load_cartridge(rom_path);
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
    cpu.pendingNMI = true;
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

SaveState Bus::save_state() const
{
  SaveState state{};
  state.cpu_regs = cpu.get_regs();
  state.cpu_flags = cpu.get_flags();
  state.cpu_pending_nmi = cpu.pendingNMI;
  state.ppu_state = ppu.save_state();
  std::memcpy(state.ram, ram, sizeof(ram));
  state.cycles = cycles;
  state.dma_page = dma_page;
  state.dma_addr = dma_addr;
  state.dma_data = dma_data;
  state.dma_transfer = dma_transfer;
  state.dma_dummy = dma_dummy;
  state.controller_state[0] = controller_state[0];
  state.controller_state[1] = controller_state[1];
  state.controller_shift[0] = controller_shift[0];
  state.controller_shift[1] = controller_shift[1];
  state.controller_strobe = controller_strobe;
  return state;
}

void Bus::load_state(const SaveState &state)
{
  cpu.load_state(state.cpu_regs, state.cpu_flags);
  cpu.pendingNMI = state.cpu_pending_nmi;
  ppu.load_state(state.ppu_state);
  std::memcpy(ram, state.ram, sizeof(ram));
  cycles = state.cycles;
  dma_page = state.dma_page;
  dma_addr = state.dma_addr;
  dma_data = state.dma_data;
  dma_transfer = state.dma_transfer;
  dma_dummy = state.dma_dummy;
  controller_state[0] = state.controller_state[0];
  controller_state[1] = state.controller_state[1];
  controller_shift[0] = state.controller_shift[0];
  controller_shift[1] = state.controller_shift[1];
  controller_strobe = state.controller_strobe;
}
