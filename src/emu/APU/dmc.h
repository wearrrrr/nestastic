#pragma once

#include <cstdint>

#include "divider.h"
#include "../irq.h"
#include <functional>

struct DMC
{
    bool irqEnable = false;
    bool loop = false;

    int volume = 0;

    bool change_enabled = false;
    Divider change_rate { 0 };

    uint16_t sample_begin = 0;
    int sample_length = 0;

    int remaining_bytes = 0;
    uint16_t current_address = 0;

    uint8_t sample_buffer = 0;

    int shifter = 0;
    int remaining_bits = 0;
    bool silenced = false;

    bool interrupt = false;

    void set_irq_enable(bool enable);
    void set_rate(int idx);
    void control(bool enable);
    void clear_interrupt();

    DMC(IRQ& irq, std::function<uint8_t(uint16_t)> dma) : irq(irq), dma(dma) {}

    // Clocked at the cpu freq
    void clock();

    uint8_t sample() const;

    bool has_more_samples() const { return remaining_bytes > 0; }

private:
    // Load sample and return if it was succesfully loaded
    bool load_sample();
    int pop_delta();
    IRQ& irq;
    std::function<uint8_t(uint16_t)> dma;
};
