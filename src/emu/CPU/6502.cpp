#include "6502.h"
#include "../bus/bus.h"
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <cstdio>

#define OP(code, name, addr, cyc, extra) \
    void op_##name##_##code(CPU6502 &cpu);

#include "6502_ops.inc"

#undef OP

static uint8_t rotate_left(CPU6502 &cpu, uint8_t value) {
    uint8_t carry_in = (cpu.get_status() & FLAG_C) ? 1 : 0;
    uint8_t new_carry = (value & 0x80) ? 1 : 0;
    value <<= 1;
    value |= carry_in;
    cpu.set_flag(FLAG_C, new_carry);
    cpu.set_zn(value);
    return value;
}

void op_BRK_0x00(CPU6502 &cpu) {
    // BRK has an extra padding byte: pretend we fetched it
    cpu.read(cpu.pc++);

    // push return address (PC - 1) high then low
    uint16_t return_addr = cpu.pc - 1;
    cpu.push((return_addr >> 8) & 0xFF);
    cpu.push(return_addr & 0xFF);

    // push flags with B flag and unused bit set in the pushed value
    uint8_t push_flags = cpu.get_status() | 0x10 | 0x20;
    cpu.push(push_flags);

    // set interrupt disable
    cpu.set_flag(FLAG_I, true);
    uint8_t lo = cpu.read(0xFFFE);
    uint8_t hi = cpu.read(0xFFFF);
    cpu.pc = (uint16_t)hi << 8 | lo;
}

void op_ORA_0x01(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t ptr_lo = cpu.read((uint8_t)(zp + cpu.regs.x));
    uint8_t ptr_hi = cpu.read((uint8_t)(zp + cpu.regs.x + 1));
    uint16_t addr = (ptr_hi << 8) | ptr_lo;
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

void op_ASL_0x06(CPU6502 &cpu) { // ASL zp
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t val = cpu.read(zp);
    cpu.set_flag(FLAG_C, (val & 0x80) != 0);
    uint8_t res = val << 1;
    cpu.write(zp, res);
    cpu.set_zn(res);
}


void op_PHP_0x08(CPU6502 &cpu) {
    uint8_t push_flags = cpu.get_status() | 0x10 | 0x20;
    cpu.push(push_flags);
}

void op_ORA_0x09(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.ac |= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_ASL_0x0A(CPU6502 &cpu) { // ASL A
    uint8_t val = cpu.regs.ac;
    cpu.set_flag(FLAG_C, (val & 0x80) != 0);
    cpu.regs.ac = val << 1;
    cpu.set_zn(cpu.regs.ac);
}

void op_ORA_0x0D(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr);
    cpu.regs.ac |= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_ASL_0x0E(CPU6502 &cpu) { // ASL abs
    uint16_t addr = cpu.read16();
    uint8_t val = cpu.read(addr);
    cpu.set_flag(FLAG_C, (val & 0x80) != 0);
    uint8_t res = val << 1;
    cpu.write(addr, res);
    cpu.set_zn(res);
}

void op_BPL_0x10(CPU6502 &cpu) {
    int8_t offset = static_cast<int8_t>(cpu.read(cpu.pc++));
    if ((cpu.get_status() & FLAG_N) == 0) {
        cpu.pc += offset;
    }
}

void op_CLC_0x18(CPU6502 &cpu) {
    cpu.set_flag(FLAG_C, 0);
}

void op_ORA_0x19(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr + cpu.regs.y);
    cpu.regs.ac |= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_NOP_0x1A(CPU6502 &cpu) {
    // No operation
}

void op_NOP_0x1C(CPU6502 &cpu) {
    // No operation
}

void op_ORA_0x1D(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr + cpu.regs.x);
    cpu.regs.ac |= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_ASL_0x1E(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr + cpu.regs.x);
    cpu.set_flag(FLAG_C, value & 0x80);
    value <<= 1;
    cpu.write(addr + cpu.regs.x, value);
    cpu.set_zn(value);
}

void op_JSR_0x20(CPU6502 &cpu) {
    uint16_t target = cpu.read16();
    uint16_t return_addr = cpu.pc - 1;
    cpu.push((return_addr >> 8) & 0xFF);
    cpu.push(return_addr & 0xFF);
    cpu.pc = target;
}

void op_AND_0x21(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read((uint8_t)(zp + 1));
    uint16_t addr = ((uint16_t)hi << 8) | lo;
    addr = addr + cpu.regs.x;
    uint8_t value = cpu.read(addr);
    cpu.regs.ac &= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_RLA_0x23(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read((uint8_t)(zp + 1));
    uint16_t addr = ((uint16_t)hi << 8) | lo;
    addr = addr + cpu.regs.x;
    uint8_t value = cpu.read(addr);
    value = rotate_left(cpu, value);
    cpu.write(addr, value);
    cpu.regs.ac &= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_BIT_0x24(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(zp);
    cpu.set_flag(FLAG_N, value & 0x80);
    cpu.set_flag(FLAG_V, value & 0x40);
    cpu.set_flag(FLAG_Z, (value & cpu.regs.ac) == 0);
}

void op_AND_0x25(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(zp);
    cpu.regs.ac &= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_PLP_0x28(CPU6502 &cpu) {
    uint8_t flags = cpu.pop();
    cpu.status.flags = flags | 0x20; // bit 5 always set
}

void op_AND_0x29(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.ac &= value;
    cpu.set_zn(cpu.regs.ac);
}

static uint8_t rotate_right(CPU6502 &cpu, uint8_t value) {
    uint8_t carry_in = (cpu.get_status() & FLAG_C) ? 1 : 0;
    uint8_t new_carry = value & 0x01;
    value >>= 1;
    if (carry_in) {
        value |= 0x80;
    }
    cpu.set_flag(FLAG_C, new_carry);
    cpu.set_zn(value);
    return value;
}

static void adc(CPU6502 &cpu, uint8_t value) {
    uint8_t A = cpu.regs.ac;
    uint8_t carry = (cpu.get_status() & FLAG_C) ? 1 : 0;
    uint16_t sum = static_cast<uint16_t>(A) + value + carry;
    uint8_t result = static_cast<uint8_t>(sum & 0xFF);
    cpu.set_flag(FLAG_C, sum > 0xFF);
    cpu.set_flag(FLAG_V, (~(A ^ value) & (A ^ result) & 0x80) != 0);
    cpu.regs.ac = result;
    cpu.set_zn(result);
}

static void sbc(CPU6502 &cpu, uint8_t value)
{
    uint8_t A = cpu.regs.ac;
    uint8_t carry = (cpu.get_status() & FLAG_C) ? 1 : 0;
    uint16_t result16 = static_cast<uint16_t>(A) - static_cast<uint16_t>(value) - (1 - carry);
    uint8_t result = static_cast<uint8_t>(result16 & 0xFF);
    cpu.regs.ac = result;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, result16 < 0x100);
    cpu.set_flag(FLAG_V, ((A ^ result) & (A ^ value) & 0x80) != 0);
}

static void isb(CPU6502 &cpu, uint16_t addr)
{
    uint8_t value = cpu.read(addr);
    value++;
    cpu.write(addr, value);
    sbc(cpu, value);
}

void op_ROL_0x2A(CPU6502 &cpu) {
    cpu.regs.ac = rotate_left(cpu, cpu.regs.ac);
}

void op_ROL_0x26(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);
    value = rotate_left(cpu, value);
    cpu.write(addr, value);
}

void op_ROL_0x36(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    addr = static_cast<uint8_t>(addr + cpu.regs.x);
    uint8_t value = cpu.read(addr);
    value = rotate_left(cpu, value);
    cpu.write(addr, value);
}

void op_ROL_0x2E(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr);
    value = rotate_left(cpu, value);
    cpu.write(addr, value);
}

void op_ROL_0x3E(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    addr = addr + cpu.regs.x;
    uint8_t value = cpu.read(addr);
    value = rotate_left(cpu, value);
    cpu.write(addr, value);
}

void op_BIT_0x2C(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr);
    cpu.set_flag(FLAG_N, value & 0x80);
    cpu.set_flag(FLAG_V, value & 0x40);
    cpu.set_flag(FLAG_Z, (value & cpu.regs.ac) == 0);
}

void op_BMI_0x30(CPU6502 &cpu) {
    int8_t offset = static_cast<int8_t>(cpu.read(cpu.pc++));
    if ((cpu.get_status() & FLAG_N) != 0) {
        cpu.pc += offset;
    }
}

void op_AND_0x35(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t addr = static_cast<uint8_t>(zp + cpu.regs.x);
    uint8_t value = cpu.read(addr);
    cpu.regs.ac &= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_SEC_0x38(CPU6502 &cpu) {
    cpu.set_flag(FLAG_C, 1);
}

void op_AND_0x39(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;
    uint16_t addr = base + cpu.regs.y;
    uint8_t value = cpu.read(addr);
    cpu.regs.ac &= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_AND_0x3D(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;
    uint16_t addr = base + cpu.regs.x;
    uint8_t value = cpu.read(addr);
    cpu.regs.ac &= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_RTI_0x40(CPU6502 &cpu) {
    uint8_t flags = cpu.pop();
    cpu.status.flags = flags | 0x20; // bit 5 always set
    uint8_t lo = cpu.pop();
    uint8_t hi = cpu.pop();
    cpu.pc = (hi << 8) | lo;
}

void op_EOR_0x41(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read((uint8_t)(zp + 1));
    uint16_t addr = ((uint16_t)hi << 8) | lo;
    addr = addr + cpu.regs.x;
    uint8_t value = cpu.read(addr);
    cpu.regs.ac ^= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_EOR_0x45(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(zp);
    cpu.regs.ac ^= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_LSR_0x46(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(zp);
    cpu.set_flag(FLAG_C, value & 0x01);
    value >>= 1;
    cpu.write(zp, value);
    cpu.set_zn(value);
}

void op_PHA_0x48(CPU6502 &cpu) {
    cpu.push(cpu.regs.ac);
}

void op_EOR_0x49(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.ac ^= value;
    cpu.set_zn(cpu.regs.ac);
}

void op_LSR_0x4A(CPU6502 &cpu) {
    uint8_t value = cpu.regs.ac;
    cpu.set_flag(FLAG_C, value & 0x01);
    value >>= 1;
    cpu.regs.ac = value;
    cpu.set_zn(cpu.regs.ac);
}

void op_JMP_0x4C(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    cpu.pc = (hi << 8) | lo;
}

void op_BVC_0x50(CPU6502 &cpu) {
    uint8_t offset = cpu.read(cpu.pc++);
    if (!(cpu.get_status() & FLAG_V)) {
        uint16_t target = cpu.pc + offset;
        cpu.pc = target;
    }
}

void op_RTS_0x60(CPU6502 &cpu) {
    uint8_t lo = cpu.pop();
    uint8_t hi = cpu.pop();
    uint16_t return_addr = (hi << 8) | lo;
    cpu.pc = return_addr + 1;
}

void op_ADC_0x61(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read((uint8_t)(zp + 1));
    uint16_t addr = ((uint16_t)hi << 8) | lo;
    addr = addr + cpu.regs.x;
    uint8_t value = cpu.read(addr);
    adc(cpu, value);
}

void op_ADC_0x65(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);
    adc(cpu, value);
}

void op_PLA_0x68(CPU6502 &cpu) {
    cpu.regs.ac = cpu.pop();
    cpu.set_zn(cpu.regs.ac);
}

void op_ADC_0x69(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    adc(cpu, value);
}

void op_ADC_0x6D(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr);
    adc(cpu, value);
}

void op_ADC_0x71(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read(static_cast<uint8_t>(zp + 1));
    uint16_t addr = (static_cast<uint16_t>(hi) << 8) | lo;
    addr = addr + cpu.regs.y;
    uint8_t value = cpu.read(addr);
    adc(cpu, value);
}

void op_ADC_0x75(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    addr = static_cast<uint8_t>(addr + cpu.regs.x);
    uint8_t value = cpu.read(addr);
    adc(cpu, value);
}

void op_ADC_0x79(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    addr = addr + cpu.regs.y;
    uint8_t value = cpu.read(addr);
    adc(cpu, value);
}

void op_ADC_0x7D(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    addr = addr + cpu.regs.x;
    uint8_t value = cpu.read(addr);
    adc(cpu, value);
}

void op_ROR_0x6A(CPU6502 &cpu) {
    cpu.regs.ac = rotate_right(cpu, cpu.regs.ac);
}

void op_ROR_0x66(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);
    value = rotate_right(cpu, value);
    cpu.write(addr, value);
}

void op_ROR_0x76(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    addr = static_cast<uint8_t>(addr + cpu.regs.x);
    uint8_t value = cpu.read(addr);
    value = rotate_right(cpu, value);
    cpu.write(addr, value);
}

void op_ROR_0x6E(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr);
    value = rotate_right(cpu, value);
    cpu.write(addr, value);
}

void op_ROR_0x7E(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    addr = addr + cpu.regs.x;
    uint8_t value = cpu.read(addr);
    value = rotate_right(cpu, value);
    cpu.write(addr, value);
}

void op_BVC_0x70(CPU6502 &cpu) {
    uint8_t offset = cpu.read(cpu.pc++);
    if (!(cpu.get_status() & FLAG_V)) {
        uint16_t target = cpu.pc + offset;
        cpu.pc = target;
    }
}

void op_SEI_0x78(CPU6502& cpu) {
    cpu.set_flag(FLAG_I, true);
}

void op_NOP_0x80(CPU6502& cpu) {
    // No operation
}

void op_STA_0x81(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read((uint8_t)(zp + 1));
    uint16_t addr = ((uint16_t)hi << 8) | lo;
    addr = addr + cpu.regs.x;
    cpu.write(addr, cpu.regs.ac);
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

void op_TXA_0x8A(CPU6502 &cpu) {
    cpu.regs.ac = cpu.regs.x;
    cpu.set_zn(cpu.regs.ac);
}

void op_STY_0x8C(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.write(addr, cpu.regs.y);
}

void op_STA_0x8D(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.write(addr, cpu.regs.ac);
}

void op_STX_0x8E(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.write(addr, cpu.regs.x);
}

void op_BCC_0x90(CPU6502 &cpu) {
    uint8_t offset = cpu.read(cpu.pc++);
    if (!(cpu.get_status() & FLAG_C)) {
        cpu.pc += offset;
    }
}

void op_STA_0x91(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read((uint8_t)(zp + 1));
    uint16_t addr = ((uint16_t)hi << 8) | lo;
    addr = addr + cpu.regs.y;
    cpu.write(addr, cpu.regs.ac);
}

void op_STA_0x95(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t addr = static_cast<uint8_t>(zp + cpu.regs.x);
    cpu.write(addr, cpu.regs.ac);
}

void op_TYA_0x98(CPU6502 &cpu) {
    cpu.regs.ac = cpu.regs.y;
    cpu.set_zn(cpu.regs.ac);
}

void op_STA_0x99(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.write(addr, cpu.regs.ac);
}

void op_STA_0x9D(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;
    uint16_t addr = base + cpu.regs.x;
    cpu.write(addr, cpu.regs.ac);
}

void op_TXS_0x9A(CPU6502 &cpu) {
    cpu.sp = cpu.regs.x;
}

void op_SHY_0x9C(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;

    uint16_t addr = base + cpu.regs.x;
    cpu.write(addr, cpu.regs.y);
}

void op_LDY_0xA0(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.y = value;
    cpu.set_zn(cpu.regs.y);
}

void op_LDA_0xA1(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read((uint8_t)(zp + 1));
    uint16_t addr = ((uint16_t)hi << 8) | lo;
    addr = addr + cpu.regs.x;
    cpu.regs.ac = cpu.read(addr);
    cpu.set_zn(cpu.regs.ac);
}

void op_LDX_0xA2(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.x = value;
    cpu.set_zn(cpu.regs.x);
}

void op_LDY_0xA4(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    cpu.regs.y = cpu.read(addr);
    cpu.set_zn(cpu.regs.y);
}

void op_LDA_0xA5(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    cpu.regs.ac = cpu.read(addr);
    cpu.set_zn(cpu.regs.ac);
}

void op_LDX_0xA6(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    cpu.regs.x = cpu.read(addr);
    cpu.set_zn(cpu.regs.x);
}

void op_TAY_0xA8(CPU6502 &cpu) {
    cpu.regs.y = cpu.regs.ac;
    cpu.set_zn(cpu.regs.y);
}

void op_LDA_0xA9(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    cpu.regs.ac = value;
    cpu.set_zn(cpu.regs.ac);
}

void op_TAX_0xAA(CPU6502 &cpu) {
    cpu.regs.x = cpu.regs.ac;
    cpu.set_zn(cpu.regs.x);
}

void op_LDY_0xAC(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.regs.y = cpu.read(addr);
    cpu.set_zn(cpu.regs.y);
}

void op_LDA_0xAD(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.regs.ac = cpu.read(addr);
    cpu.set_zn(cpu.regs.ac);
}

void op_LDX_0xAE(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    cpu.regs.x = cpu.read(addr);
    cpu.set_zn(cpu.regs.x);
}

void op_BCS_0xB0(CPU6502 &cpu) {
    int8_t offset = static_cast<int8_t>(cpu.read(cpu.pc++));
    if ((cpu.get_status() & FLAG_C) != 0) {
        cpu.pc += offset;
    }
}


void op_LDA_0xB1(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t ptr_lo = cpu.read(zp);
    uint8_t ptr_hi = cpu.read(static_cast<uint8_t>(zp + 1));
    uint16_t addr = (static_cast<uint16_t>(ptr_hi) << 8) | ptr_lo;
    cpu.regs.ac = cpu.read(addr + cpu.regs.y);
    cpu.set_zn(cpu.regs.ac);
}

void op_LDY_0xB4(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t addr = static_cast<uint8_t>(zp + cpu.regs.x);
    cpu.regs.y = cpu.read(addr);
    cpu.set_zn(cpu.regs.y);
}

void op_LDA_0xB5(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    cpu.regs.ac = cpu.read(addr + cpu.regs.x);
    cpu.set_zn(cpu.regs.ac);
}

void op_LDX_0xB6(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t addr = static_cast<uint8_t>(zp + cpu.regs.y);
    cpu.regs.x = cpu.read(addr);
    cpu.set_zn(cpu.regs.x);
}

void op_CLV_0xB8(CPU6502 &cpu) {
    cpu.set_flag(FLAG_V, 0);
}

void op_LDA_0xB9(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;
    uint16_t addr = base + cpu.regs.y;
    uint8_t value = cpu.read(addr);
    cpu.regs.ac = value;
    cpu.set_zn(value);
}

void op_TSX_0xBA(CPU6502 &cpu) {
    cpu.regs.x = cpu.sp;
    cpu.set_zn(cpu.regs.x);
}

void op_LDY_0xBC(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;

    uint16_t addr = base + cpu.regs.x;
    uint8_t value = cpu.read(addr);

    cpu.regs.y = value;
    cpu.set_zn(value);
}

void op_LDA_0xBD(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;

    uint16_t addr = base + cpu.regs.x;
    uint8_t value = cpu.read(addr);

    cpu.regs.ac = value;
    cpu.set_zn(value);
}

void op_LDX_0xBE(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;

    uint16_t addr = base + cpu.regs.y;
    uint8_t value = cpu.read(addr);

    cpu.regs.x = value;
    cpu.set_zn(value);
}

void op_LDA_0xBF(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;

    uint16_t addr = base + cpu.regs.y;
    uint8_t value = cpu.read(addr);

    cpu.regs.ac = value;
    cpu.set_zn(value);
}

void op_CPY_0xC0(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    uint8_t Y = cpu.regs.y;
    uint8_t result = Y - value;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, Y >= value);
}

void op_CMP_0xC1(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;

    uint16_t addr = base + cpu.regs.x;
    uint8_t value = cpu.read(addr);

    uint8_t A = cpu.regs.ac;
    uint8_t result = A - value;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, A >= value);
}

void op_CPY_0xC4(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(zp);
    uint8_t Y = cpu.regs.y;
    uint8_t result = Y - value;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, Y >= value);
}

void op_CMP_0xC5(CPU6502 &cpu) {
    uint8_t addr = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(addr);

    uint8_t A = cpu.regs.ac;
    uint8_t result = A - value;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, A >= value);
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

void op_CMP_0xC9(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    uint8_t A = cpu.regs.ac;
    uint8_t result = A - value;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, A >= value);
}

void op_DEX_0xCA(CPU6502 &cpu) {
    cpu.regs.x--;
    cpu.set_zn(cpu.regs.x);
}

void op_CMP_0xCD(CPU6502 &cpu) {
    uint16_t addr = cpu.read(cpu.pc) | (cpu.read(cpu.pc++) << 8);
    cpu.pc += 2;

    uint8_t value = cpu.read(addr);
    uint8_t A = cpu.regs.ac;
    uint8_t result = A - value;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, A >= value);
}

void op_DEC_0xCE(CPU6502 &cpu) {
    // Fetch 16-bit absolute address
    uint16_t addr = cpu.read(cpu.pc) | (cpu.read(cpu.pc + 1) << 8);
    cpu.pc += 2;

    // Read, decrement, write back
    uint8_t value = cpu.read(addr);
    value--;
    cpu.write(addr, value);

    // Update flags
    cpu.set_zn(value);
}

void op_BNE_0xD0(CPU6502 &cpu) {
    int8_t offset = static_cast<int8_t>(cpu.read(cpu.pc++));
    if ((cpu.get_status() & FLAG_Z) == 0) {
        cpu.pc += offset;
    }
}

void op_DCP_0xD3(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t ptr_lo = cpu.read(zp);
    uint8_t ptr_hi = cpu.read(static_cast<uint8_t>(zp + 1));
    uint16_t addr = (static_cast<uint16_t>(ptr_hi) << 8) | ptr_lo;
    addr += cpu.regs.y;

    uint8_t value = cpu.read(addr);
    value--;
    cpu.write(addr, value);

    uint8_t result = cpu.regs.ac - value;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, cpu.regs.ac >= value);
}

void op_DEC_0xD6(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t addr = static_cast<uint8_t>(zp + cpu.regs.x);
    uint8_t value = cpu.read(addr);
    value--;
    cpu.write(addr, value);

    cpu.set_zn(value);
}

void op_CLD_0xD8(CPU6502 &cpu) {
    cpu.set_flag(FLAG_D, false);
}

void op_CPX_0xE0(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    uint8_t X = cpu.regs.x;
    uint8_t result = X - value;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, X >= value);
}

void op_SBC_0xE1(CPU6502 &cpu)
{
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t ptr = (zp + cpu.regs.x) & 0xFF;
    uint8_t low  = cpu.read(ptr);
    uint8_t high = cpu.read((ptr + 1) & 0xFF);
    uint16_t addr = (uint16_t(high) << 8) | low;
    uint8_t value = cpu.read(addr);
    sbc(cpu, value);
}

void op_CPX_0xE4(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(zp);
    uint8_t X = cpu.regs.x;
    uint8_t result = X - value;
    cpu.set_zn(result);
    cpu.set_flag(FLAG_C, X >= value);
}

void op_SBC_0xE5(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t value = cpu.read(zp);
    sbc(cpu, value);
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

void op_SBC_0xE9(CPU6502 &cpu) {
    uint8_t value = cpu.read(cpu.pc++);
    sbc(cpu, value);
}

void op_NOP_0xEA(CPU6502 &cpu) {
    // do nothing
}

void op_SBC_0xED(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr);
    sbc(cpu, value);
}

void op_INC_0xEE(CPU6502 &cpu) {
    uint16_t addr = cpu.read16();
    uint8_t value = cpu.read(addr);
    value++;
    cpu.write(addr, value);
    cpu.set_zn(value);
}

void op_BEQ_0xF0(CPU6502 &cpu) {
    int8_t offset = static_cast<int8_t>(cpu.read(cpu.pc++));
    if ((cpu.get_status() & FLAG_Z) != 0) {
        cpu.pc += offset;
    }
}

void op_SBC_0xF1(CPU6502 &cpu) {
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read(static_cast<uint8_t>(zp + 1));
    uint16_t addr = (static_cast<uint16_t>(hi) << 8) | lo;
    addr += cpu.regs.y;

    uint8_t value = cpu.read(addr);
    sbc(cpu, value);
}

void op_SBC_0xF5(CPU6502 &cpu) {
    uint8_t zp_addr = cpu.read(cpu.pc++);
    uint8_t addr = (zp_addr + cpu.regs.x) & 0xFF;
    uint8_t M = cpu.read(addr);
    sbc(cpu, M);
}

void op_SBC_0xF9(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;
    uint16_t addr = base + cpu.regs.y;
    uint8_t value = cpu.read(addr);
    sbc(cpu, value);
}

void op_SED_0xF8(CPU6502 &cpu) {
    cpu.set_flag(FLAG_D, 1);
}

void op_SBC_0xFD(CPU6502 &cpu) {
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (hi << 8) | lo;
    uint16_t addr = base + cpu.regs.x;
    uint8_t value = cpu.read(addr);
    sbc(cpu, value);
}

void op_ISC_0xE3(CPU6502 &cpu)
{
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t ptr = static_cast<uint8_t>(zp + cpu.regs.x);
    uint8_t lo = cpu.read(ptr);
    uint8_t hi = cpu.read(static_cast<uint8_t>(ptr + 1));
    uint16_t addr = (static_cast<uint16_t>(hi) << 8) | lo;
    isb(cpu, addr);
}

void op_ISC_0xE7(CPU6502 &cpu)
{
    uint8_t zp = cpu.read(cpu.pc++);
    isb(cpu, zp);
}

void op_ISC_0xEF(CPU6502 &cpu)
{
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t addr = (static_cast<uint16_t>(hi) << 8) | lo;
    isb(cpu, addr);
}

void op_ISC_0xF3(CPU6502 &cpu)
{
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t lo = cpu.read(zp);
    uint8_t hi = cpu.read(static_cast<uint8_t>(zp + 1));
    uint16_t addr = (static_cast<uint16_t>(hi) << 8) | lo;
    addr += cpu.regs.y;
    isb(cpu, addr);
}

void op_ISC_0xF7(CPU6502 &cpu)
{
    uint8_t zp = cpu.read(cpu.pc++);
    uint8_t addr = static_cast<uint8_t>(zp + cpu.regs.x);
    isb(cpu, addr);
}

void op_ISC_0xFB(CPU6502 &cpu)
{
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (static_cast<uint16_t>(hi) << 8) | lo;
    uint16_t addr = base + cpu.regs.y;
    isb(cpu, addr);
}

void op_ISC_0xFF(CPU6502 &cpu)
{
    uint8_t lo = cpu.read(cpu.pc++);
    uint8_t hi = cpu.read(cpu.pc++);
    uint16_t base = (static_cast<uint16_t>(hi) << 8) | lo;
    uint16_t addr = base + cpu.regs.x;
    isb(cpu, addr);
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

    if (!bus || !bus->cart)
    {
        fprintf(stderr, "CPU reset called before cartridge was attached\n");
        pc = 0x0000;
    }
    else
    {
        // Fetch reset vector through the bus so mappers mirror correctly
        uint8_t lo = read(0xFFFC);
        uint8_t hi = read(0xFFFD);
        pc = (uint16_t)(hi << 8) | lo;
    }

    printf("PC: %04X\n", pc);

    cycles_remaining = 0;
}

void CPU6502::nmi() {
	nmi_pending = true;
}


void CPU6502::clock() {
    if (cycles_remaining == 0) {
        if (nmi_pending) {
            push((pc >> 8) & 0xFF);
            push(pc & 0xFF);
            set_flag(FLAG_B, false);
            set_flag(FLAG_U, true);
            push(status.flags);
            set_flag(FLAG_I, true);
            uint8_t lo = read(0xFFFA);
            uint8_t hi = read(0xFFFB);
            pc = (hi << 8) | lo;
            cycles_remaining = 8;
            nmi_pending = false;
        } else {
            uint8_t opcode = read(pc++);
            printf("Opcode: %02X, current address: %04X\n", opcode, pc - 1);
            const Op &op = ops[opcode];

            if (!op.handler) {
                printf("Unimplemented opcode: %02X\n", opcode);
                return;
                // exit(1);
            }

            op.handler(*this);

            // Load the base cycle cost
            cycles_remaining = op.cycles;
        }
    }

    cycles_remaining--;
    cycles++;
}
