#include "6502.h"
#include <cstdlib>

#define OP(fn, cycles) { &CPU6502::fn, cycles }
#define OP_NONE         { nullptr, 0 }

const Op CPU6502::op_table[256] = {
    /* 00 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 04 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 08 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 0C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* 10 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 14 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 18 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 1C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* 20 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 24 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 28 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 2C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* 30 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 34 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 38 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 3C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* 40 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 44 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 48 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 4C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* 50 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 54 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 58 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 5C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* 60 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 64 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 68 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 6C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* 70 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 74 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 78 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 7C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* 80 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 84 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 88 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 8C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* 90 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 94 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 98 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* 9C */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* A0 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* A4 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* A8 */ OP_NONE, OP(op_LDA_imm, 2), OP_NONE, OP_NONE,
    /* AC */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* B0 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* B4 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* B8 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* BC */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* C0 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* C4 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* C8 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* CC */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* D0 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* D4 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* D8 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* DC */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* E0 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* E4 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* E8 */ OP_NONE, OP_NONE, OP(op_NOP, 2), OP_NONE,
    /* EC */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,

    /* F0 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* F4 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* F8 */ OP_NONE, OP_NONE, OP_NONE, OP_NONE,
    /* FC */ OP_NONE, OP_NONE, OP_NONE, OP_NONE
};

void CPU6502::op_NOP() {
    // do nothing
}

void CPU6502::op_LDA_imm() {
    uint8_t value = read(pc++);
    regs.A = value;
    set_zn(regs.A);
    reg_dump();
}

void CPU6502::reset() {
    printf("Reset!\n");
    pc = 0xFFFC;
    regs.A = 0;
    regs.X = 0;
    regs.Y = 0;
    regs.SP = 0xFF;
    status.flags = 0x24;

    for (int i = 0; i < 0x10000; i++)
        memory[i] = 0xEA;

    // For now, throw some demo instructions into the memory
    memory[0xFFFC] = 0xA9; // LDA #$42
    memory[0xFFFD] = 0x42;
}

void CPU6502::clock() {
    // If we're not currently in the middle of an instruction, fetch a new one
    if (cycles_remaining == 0) {
        uint8_t opcode = read(pc++);
        const Op &op = op_table[opcode];

        if (!op.handler) {
            printf("Unimplemented opcode: %02X\n", opcode);
            exit(1);
        }

        // Execute the instruction
        (this->*op.handler)();

        // Load the base cycle cost
        cycles_remaining = op.cycles;
    }

    // One cycle passes
    cycles_remaining--;
    cycles++;
}
