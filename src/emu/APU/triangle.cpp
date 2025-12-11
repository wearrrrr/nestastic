#include "triangle.h"
#include "divider.h"
#include "units.h"

// Provide simple helper to compute note frequency (unused directly here but
// kept for parity with earlier implementations).
inline int calc_note_freq(int period, int seq_length, nanoseconds clock_period) {
    return 1e9 / ((float)clock_period.count() * period * seq_length);
}

void Triangle::set_period(int p) {
    period = p;
    sequencer.set_period(p);
}

// Reset triangle runtime state. Mirrors Nestopia's Triangle::Reset behavior by
// clearing runtime counters, sequencer and active state so the channel is fully
// silent until explicitly enabled by writes.
void Triangle::Reset()
{
    seq_idx = 0;
    period = 0;
    sequencer.reset();
    linear_counter.counter = 0;
    linear_counter.reload = false;
    linear_counter.reloadValue = 0;
    length_counter.set_enable(false);
}

// Clocked at the cpu freq
void Triangle::clock() {

    // Respect standard muting conditions (length / linear counters).
    if (length_counter.muted()) {
        return;
    }
    if (linear_counter.counter == 0) {
        return;
    }

    if (sequencer.clock()) {
        seq_idx = (seq_idx + 1) % 32;
    }
}

uint8_t Triangle::sample() const {

    // Standard muting conditions.
    if (length_counter.muted()) {
        return 0;
    }
    if (linear_counter.counter == 0) {
        return 0;
    }

    return volume();
}

int Triangle::volume() const {
    if (seq_idx < 16) {
        return 15 - seq_idx;
    } else {
        return seq_idx - 16;
    }
}
