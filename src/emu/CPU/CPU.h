#pragma once
#include "opcodes.h"
#include <cstdint>
#include <list>

class Bus;
class CPU;

union CPUFlags {
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
    uint8_t all;
};

struct CPURegisters {
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t sp;
    uint8_t status;
};


class IRQ {
    int  bit;
    CPU &cpu;

public:
    IRQ(int bit, CPU &cpu) : bit(bit), cpu(cpu) {};

    void release();
    void pull();
};

class CPU {
public:
    CPU(Bus &mem);

    void clock();
    void reset();
    void reset(uint16_t start_addr);
    void log();

    uint16_t getPC() { return regs.pc; }
    void skipOAMDMACycles();
    void skipDMCDMACycles();

    void nmiInterrupt();

    IRQ& createIRQHandler();
    void setIRQPulldown(int bit, bool state);

    void load_state(const CPURegisters &regs, const CPUFlags &flags) {
        this->regs = regs;
        this->flags = flags;
    };


    CPUFlags get_flags() const { return flags; }
    CPURegisters get_regs() const;
    int getCycleCount() const { return cycles; }

private:
    void interruptSequence(InterruptType type);

    // Instructions are split into five sets to make decoding easier.
    // These functions return true if they succeed
    bool executeImplied(uint8_t opcode);
    bool executeBranch(uint8_t opcode);
    bool executeType0(uint8_t opcode);
    bool executeType1(uint8_t opcode);
    bool executeType2(uint8_t opcode);

    uint16_t read_address(uint16_t addr);

    void stack_push(uint8_t value);
    uint8_t stack_pop();

    void skipPageCrossCycle(uint16_t a, uint16_t b);
    void set_zn(uint8_t value);

    int skipCycles;
    int cycles;

    CPURegisters regs;
    CPUFlags flags;

public:
    bool pendingNMI;
private:
    bool isPendingIRQ() const { return !flags.I && m_irqPulldowns != 0; };
    Bus &bus;

    // Each bit is assigned to an IRQ handler.
    // If any bits are set, it means the irq must be triggered
    int m_irqPulldowns = 0;
    std::list<IRQ> m_irqHandlers;
};
