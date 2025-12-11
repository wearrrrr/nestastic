#pragma once

#include <cstdint>

#include "divider.h"
#include "units.h"

struct Triangle {
    LengthCounter length_counter;
    LinearCounter linear_counter;

    uint32_t seq_idx { 0 };
    Divider sequencer { 0 };
    int period = 0;

    void set_period(int p);

    // Reset runtime state (sequencer, counters, active flag)
    void Reset();

    // Clocked at the cpu freq
    void clock();

    uint8_t sample() const;

    int volume() const;
};
