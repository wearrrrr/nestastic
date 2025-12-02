#include <cstdint>
#include <cstdio>

#include "opcodes.h"

typedef class CPU6502 CPU6502;

struct Op {
    void (CPU6502::*handler)();
    uint8_t cycles;
};

class CPU6502 {
public:
    CPU6502() : memory(), pc(0), regs(), cycles(0) {
        this->status.flags = 0x24;
    }
    ~CPU6502() = default;


    uint8_t read(uint16_t addr) {
        return memory[addr];
    }

    uint16_t read16() {
        uint16_t lo = read(pc++);
        uint16_t hi = read(pc++);
        return lo | (hi << 8);
    }

    void write(uint16_t addr, uint8_t value) {
        memory[addr] = value;
    }

    void set_zn(uint8_t value) {
        status.Z = (value == 0);
        status.N = (value & 0x80) != 0;
    }

    void reg_dump() {
        printf("A: %02X X: %02X Y: %02X SP: %02X\n", regs.A, regs.X, regs.Y, regs.SP);
    }

    void reset();
    void clock();

    void op_NOP();
    void op_LDA_imm();

private:
    uint8_t memory[0x10000];
    uint16_t pc;
    struct {
        uint8_t A;
        uint8_t X;
        uint8_t Y;
        uint8_t SP;
    } regs;
    union {
        uint8_t flags;
        struct {
            uint8_t C : 1;
            uint8_t Z : 1;
            uint8_t I : 1;
            uint8_t D : 1;
            uint8_t B : 1;
            uint8_t U : 1;
            uint8_t V : 1;
            uint8_t N : 1;
        };
    } status;
    uint64_t cycles;
    uint8_t cycles_remaining = 0;
    static const Op op_table[256];
};
