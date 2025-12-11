#pragma once

class CPU;

class IRQ {
    int bit;
    CPU &cpu;

public:
    IRQ(int bit, CPU &cpu) : bit(bit), cpu(cpu) {};

    void release();
    void pull();
};
