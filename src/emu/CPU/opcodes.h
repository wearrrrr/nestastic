#pragma once

const auto InstructionModeMask = 0x3;

const constexpr auto OperationMask = 0xE0;
const constexpr auto OperationShift = 5;

const constexpr auto AddrModeMask = 0x1C;
const constexpr auto AddrModeShift = 2;

const constexpr auto BranchInstructionMask = 0x1F;
const constexpr auto BranchInstructionMaskResult = 0x10;
const constexpr auto BranchConditionMask = 0x20;
const constexpr auto BranchOnFlagShift = 6;

enum BranchOnFlag {
    Negative,
    Overflow,
    Carry,
    Zero
};

enum Operation1 {
    ORA,
    AND,
    EOR,
    ADC,
    STA,
    LDA,
    CMP,
    SBC,
};

enum AddrMode1 {
    IndexedIndirectX,
    ZeroPage,
    Immediate,
    Absolute,
    IndirectY,
    IndexedX,
    AbsoluteY,
    AbsoluteX,
};

enum Operation2 {
    ASL,
    ROL,
    LSR,
    ROR,
    STX,
    LDX,
    DEC,
    INC,
};

enum AddrMode2 {
    Addr_Immediate,
    Addr_ZeroPage,
    Addr_Accumulator,
    Addr_Absolute,
    Addr_Indexed = 5,
    Addr_AbsoluteIndexed = 7,
};

enum Operation0 {
    BIT = 1,
    STY = 4,
    LDY,
    CPY,
    CPX,
};

enum OperationImplied {
    NOP  = 0xEA,
    BRK  = 0x00,
    JSR  = 0x20,
    RTI  = 0x40,
    RTS  = 0x60,

    JMP  = 0x4C,
    JMPI = 0x6C,

    PHP  = 0x08,
    PLP  = 0x28,
    PHA  = 0x48,
    PLA  = 0x68,

    DEY  = 0x88,
    DEX  = 0xCa,
    TAY  = 0xA8,
    INY  = 0xC8,
    INX  = 0xE8,

    CLC  = 0x18,
    SEC  = 0x38,
    CLI  = 0x58,
    SEI  = 0x78,
    TYA  = 0x98,
    CLV  = 0xB8,
    CLD  = 0xD8,
    SED  = 0xF8,

    TXA  = 0x8A,
    TXS  = 0x9A,
    TAX  = 0xAA,
    TSX  = 0xBA,
};

enum InterruptType {
    INTR_IRQ,
    INTR_NMI,
    INTR_BRK
};

// 0 = unused opcode
static const int OperationCycles[0x100] = {
    7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
    2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,
    2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
};
