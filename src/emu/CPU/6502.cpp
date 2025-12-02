#include "6502.h"
#include "../bus/bus.h"
#include <cstdlib>
#include <cstring>
#include <sys/types.h>

#define OP(code, name, addr, cyc, extra) \
    void op_##name##_##code(CPU6502 &cpu);

#include "6502_ops.inc"

#undef OP

void op_BRK_0x00(CPU6502 &cpu) {
    cpu.push(cpu.pc >> 8);
    cpu.push(cpu.pc & 0xFF);
    cpu.push(cpu.get_status());
    cpu.sr |= 0x10;
    cpu.pc = cpu.read16();
}

void op_ORA_0x01(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);
    cpu.regs.ac |= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_ORA_0x05(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);
    cpu.regs.ac |= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_ASL_0x06(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);
    cpu.write(addr, value << 1);
    cpu.set_zn(value << 1);
}

void op_PHP_0x08(CPU6502 &cpu) {
    cpu.push(cpu.get_status());
}

void op_ORA_0x09(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.ac |= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_ASL_0x0A(CPU6502 &cpu) {
    uint8_t value = cpu.regs.ac;
    cpu.regs.ac = value << 1;
    cpu.set_zn(cpu.regs.ac);
}

void op_ORA_0x0D(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr);
    cpu.regs.ac |= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_ASL_0x0E(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr);
    cpu.write(addr, value << 1);
    cpu.set_zn(value << 1);
}

void op_BPL_0x10(CPU6502 &cpu) {
    if ((cpu.sr & 0x80) == 0) {
        cpu.pc += cpu.read(cpu.pc++);
    }
}

void op_JSR_0x20(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.pc += 2;
    cpu.push(cpu.pc >> 8);
    cpu.push(cpu.pc & 0xFF);
    cpu.pc = addr;
}

void op_PLP_0x28(CPU6502 &cpu) {
    cpu.sr = cpu.pop();
}

void op_BMI_0x30(CPU6502 &cpu) {
    if ((cpu.sr & 0x80) != 0) {
        cpu.pc += cpu.read(cpu.pc++);
    }
}

void op_PHA_0x48(CPU6502 &cpu) {
    cpu.push(cpu.regs.ac);
}

void op_JMP_0x4C(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.pc = addr;
}

void op_RTS_0x60(CPU6502 &cpu) {
    cpu.pc = cpu.pop();
    cpu.pc |= cpu.pop() << 8;
}

void op_ADC_0x65(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);
    cpu.regs.ac += value;
    cpu.set_zn(cpu.regs.ac);
}

void op_PLA_0x68(CPU6502 &cpu) {
    cpu.regs.ac = cpu.pop();
    cpu.set_zn(cpu.regs.ac);
}

void op_ADC_0x69(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.ac += value;
    cpu.set_zn(cpu.regs.ac);
}

void op_STY_0x84(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    cpu.write(addr, cpu.regs.y);
}

void op_STA_0x85(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    cpu.write(addr, cpu.regs.ac);
}

void op_STX_0x86(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    cpu.write(addr, cpu.regs.x);
}

void op_DEY_0x88(CPU6502 &cpu) {
    cpu.regs.y--;
    cpu.set_zn(cpu.regs.y);
}

void op_STA_0x8D(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.pc += 2;
    cpu.write(addr, cpu.regs.ac);
}

void op_LDY_0xA0(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    cpu.regs.y = cpu.read(addr);
    cpu.set_zn(cpu.regs.y);
}

void op_LDX_0xA2(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.x = value;
    cpu.set_zn(cpu.regs.x);
}

void op_LDA_0xA5(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    cpu.regs.ac = cpu.read(addr);
    cpu.set_zn(cpu.regs.ac);
}

void op_LDA_0xA9(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.ac = value;
    cpu.set_zn(cpu.regs.ac);
}

void op_LDA_0xAD(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.pc += 2;
    uint8_t value = cpu.read(addr);
    cpu.regs.ac = value;
    cpu.set_zn(cpu.regs.ac);
}

void op_DEC_0xC6(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);
    value--;
    cpu.write(addr, value);
    cpu.set_zn(value);
}

void op_INY_0xC8(CPU6502 &cpu) {
    cpu.regs.y++;
    cpu.set_zn(cpu.regs.y);
}

void op_DEX_0xCA(CPU6502 &cpu) {
    cpu.regs.x--;
    cpu.set_zn(cpu.regs.x);
}

void op_BNE_0xD0(CPU6502 &cpu) {
    int8_t offset = static_cast<int8_t>(cpu.read(cpu.pc++));
    if ((cpu.get_status() & FLAG_Z) == 0) {
        cpu.pc += offset;
    }
}

void op_INC_0xE6(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);
    value++;
    cpu.write(addr, value);
    cpu.set_zn(value);
}

void op_INX_0xE8(CPU6502 &cpu) {
    cpu.regs.x++;
    cpu.set_zn(cpu.regs.x);
}

void op_BEQ_0xF0(CPU6502 &cpu) {
    uint8_t offset = cpu.read(cpu.pc++);
    if ((cpu.get_status() & FLAG_Z) != 0) {
        cpu.pc += offset;
    }
}

void op_NOP_0xEA(CPU6502 &cpu) {
    // do nothing
}

// x-macros
#define OP(code, name, addr, cyc, extra) \
    void op_##name##_##code(CPU6502 &cpu);

#include "6502_ops.inc"
#undef OP

CPU6502::CPU6502() : pc(0), regs(), memory(), cycles(0), cycles_remaining(0) {
    status.flags = 0x24;

    for (int i = 0; i < 256; ++i) {
        ops[i].handler = nullptr;
        ops[i].cycles = 0;
    }

    #define OP(code, name, addr, cyc, extra) \
        ops[code].handler = op_##name##_##code; \
        ops[code].cycles  = cyc;

    #include "6502_ops.inc"
    #undef OP
}

uint8_t CPU6502::read(uint16_t addr) {
    return bus->read(addr);
}

void CPU6502::write(uint16_t addr, uint8_t value) {
    bus->write(addr, value);
}

void CPU6502::reset() {
    printf("Reset!\n");

    regs.ac = 0;
    regs.x = 0;
    regs.y = 0;
    sp = 0xFF;

    status.flags = 0x24;

    for (size_t i = 0; i < sizeof(memory); i++)
        memory[i] = 0xEA;

    static const unsigned char imm_arith_bin[] = {
      0xa9, 0x10, 0x69, 0x05, 0x4c, 0x00, 0x80, 0x00, 0x80
    };
    memcpy(&memory[0x8000], imm_arith_bin, sizeof(imm_arith_bin));

    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x80;

    // Read reset vector
    pc = memory[0xFFFC] | (memory[0xFFFD] << 8);

    cycles_remaining = 0;
}


void CPU6502::clock() {
    if (cycles_remaining == 0) {
        uint8_t opcode = read(pc++);
        // printf("Opcode: %02X, current address: %04X\n", opcode, pc - 1);
        const Op &op = ops[opcode];

        if (!op.handler) {
            printf("Unimplemented opcode: %02X\n", opcode);
            exit(1);
        }

        op.handler(*this);

        // Load the base cycle cost
        cycles_remaining = op.cycles;
    }

    cycles_remaining--;
    cycles++;
}
