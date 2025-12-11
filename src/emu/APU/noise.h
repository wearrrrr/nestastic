#pragma once

#include "divider.h"
#include "units.h"

struct Noise {
    Volume        volume;
    LengthCounter length_counter;
    Divider       divider { 0 };

    enum Mode : bool
    {
        Bit1 = 0,
        Bit6 = 1,
    } mode = Bit1;
    int period = 0;
    int shift_register = 1;

    void set_period_from_table(int idx);

    // Clocked at the cpu freq
    void clock();

    uint8_t sample() const;
};
