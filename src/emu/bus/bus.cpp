#include "bus.h"
#include <cstring>
#include <cstdio>

// Global pointer to the IRQ handler created for the APU. Stored at file-scope so
// the lambda passed to the APU can reference a stable pointer to the IRQ object.
static IRQ* g_apu_irq = nullptr;

Bus::Bus(const char *rom_path) {
    cart = load_cartridge(rom_path);

    // Create the audio player first. The integer passed is the sample-rate
    // of the audio frames the APU will push into the player's ring buffer.
    // Choose 44100 here (matches the AudioPlayer default output rate).
    audio_player = new AudioPlayer(44100);
    audio_player->start();

    // Prefill the audio queue with a small burst of silence so the audio callback
    // has some headroom during startup and short scheduling hiccups. This reduces
    // the likelihood of initial crackle caused by immediate underflow.
    // Push ~200ms of silence as a warm-up buffer.
    const int prefill_ms = 200;
    const int prefill_samples = (audio_player->output_sample_rate * prefill_ms) / 1000;
    for (int i = 0; i < prefill_samples; ++i) {
        audio_player->audio_queue.push(0.0f);
    }

    // Create a persistent IRQ handler for the APU and store a pointer for callbacks.
    IRQ &handler = cpu.createIRQHandler();
    g_apu_irq = &handler;

    // Construct the APU, providing:
    //  - reference to the audio player so it can push samples into audio_player->audio_queue
    //  - reference to the IRQ handler (FrameCounter / DMC may need it)
    //  - a DMC memory-read callback that forwards to the Bus::read method
    apu = new APU(*audio_player, handler, [this](uint16_t addr) -> uint8_t {
        return this->read(addr);
    });

    // The APU implementation already accepts the IRQ and DMC callback above,
    // so no additional setMemoryReadCallback / setIRQCallback wiring is required here.
}

Bus::~Bus() {
    // Release the IRQ handler we created for the APU so the CPU's pulldown state
    // is cleaned up before destruction.
    if (g_apu_irq) {
        g_apu_irq->release();
        g_apu_irq = nullptr;
    }

    delete apu;
    delete audio_player;
    delete cart;
}

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

    // APU/IO handling (0x4000..0x4017)
    if (addr >= 0x4000 && addr <= 0x4017) {
        // Only $4015 (status) is readable from the APU; other APU registers are write-only here.
        if (addr == 0x4015 && apu) {
            uint8_t status = apu->readStatus();
            if (getAPULogging()) {
                std::fprintf(stderr, "[APU READ ] addr=$%04X -> $%02X\n", addr, status);
            }
            return status;
        }
    }

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

    // APU and IO registers (0x4000..0x4017)
    if (addr >= 0x4000 && addr <= 0x4017) {
        if (apu) {
            apu->writeRegister(addr, value);
            if (getAPULogging()) {
                std::fprintf(stderr, "[APU WRITE] addr=$%04X <= $%02X\n", addr, value);
            }
            return;
        }
    }
}

void Bus::clock() {
    ppu.clock();

    if ((cycles % 3) == 0) {
        apu->step();
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
    // APU save/load not performed here. Persist APU state inside the APU implementation
    // or reintroduce APUState to SaveState if you want the bus to store it.
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
    // APU state restore not performed here. If APU state restore is required,
    // call apu->load_state(...) when an APUState/serialization API is available.
}
