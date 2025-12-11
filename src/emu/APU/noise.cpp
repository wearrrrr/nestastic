#include "noise.h"
#include "divider.h"

void Noise::set_period_from_table(int idx) {
    const static int periods[] {
        4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
    };

    divider.set_period(periods[idx]);
}

void Noise::clock() {
    if (!divider.clock()) {
        return;
    }

    const int tap = mode == Bit1 ? 1 : 6;
    int feedback = ((shift_register >> 0) & 0x1) ^ ((shift_register >> tap) & 0x1);
    shift_register >>= 1;
    shift_register |= (feedback << 14);
}

uint8_t Noise::sample() const {
    if (length_counter.muted()) {
        return 0;
    }

    if (shift_register & 0x1) {
        return 0;
    }

    return volume.get();
}
