#pragma once

#include <cstdint>
#include <cstdio>

typedef class Bus Bus;

enum CPU_FLAGS {
    FLAG_C = 1 << 0,
    FLAG_Z = 1 << 1,
    FLAG_I = 1 << 2,
    FLAG_D = 1 << 3,
    FLAG_B = 1 << 4,
    FLAG_U = 1 << 5,
    FLAG_V = 1 << 6,
    FLAG_N = 1 << 7
};

typedef class CPU6502 CPU6502;

typedef void (*OpHandler)(CPU6502 &);

struct Op {
    OpHandler handler;
    uint8_t cycles;
};

class CPU6502 {
public:

    // program counter
    uint16_t pc;
    // stack pointer and stack
    uint8_t sp;
    uint8_t stack[0x100];

    Bus *bus;

    // accumulator, x, and y.
    struct {
        uint8_t ac;
        uint8_t x;
        uint8_t y;
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

    CPU6502();
    ~CPU6502() = default;

    void Connect(Bus *bus) {
        this->bus = bus;
    }


    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);

    uint16_t read16() {
        uint16_t lo = read(pc++);
        uint16_t hi = read(pc++);
        return lo | (hi << 8);
    }

    void set_flag(uint8_t flag, bool v) {
        if (v) status.flags |= flag;
        else   status.flags &= ~flag;
    }

    void set_zn(uint8_t value) {
        status.Z = (value == 0);
        status.N = (value & 0x80) != 0;
    }

    void reg_dump() {
        printf("A: %02X X: %02X Y: %02X SP: %02X\n", regs.ac, regs.x, regs.y, sp);
    }

    uint8_t get_status() {
        return status.flags;
    }

    void push(uint8_t value) {
        stack[sp--] = value;
    }

    uint8_t pop() {
        sp++;
        return stack[sp];
    }

    void reset();
    void clock();
    void nmi();

private:
    uint8_t memory[0x10000];
    uint64_t cycles;
    uint8_t cycles_remaining = 0;
    bool nmi_pending = false;

    Op ops[256];
};
