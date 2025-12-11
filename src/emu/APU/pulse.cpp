#include "constants.h"

#include "divider.h"
#include "pulse.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

/***** Pulse *****/

inline int calc_note_freq(int period, int seq_length, nanoseconds clock_period)
{
    return 1e9 / ((float)clock_period.count() * period * seq_length);
}

std::string freq_to_note(double freq) {
    std::vector<std::string> notes = { "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#" };

    double note_number_double = 12 * std::log2(freq / 440.0) + 49;
    int note_number = static_cast<int>(std::round(note_number_double));

    int note_index = (note_number - 1) % notes.size();
    if (note_index < 0) { // Handle negative modulo result in C++
        note_index += notes.size();
    }
    std::string note = notes[note_index];

    int octave = (note_number + 8) / notes.size();

    return note + std::to_string(octave);
}

void Pulse::set_period(int p) {
    period = p & 0x7FF;
    sequencer.set_period(period);
}

// Clocked at half the cpu freq
void Pulse::clock() {
    if (sequencer.clock()) {
        // NES counts downwards in sequencer
        seq_idx = (8 + (seq_idx - 1)) % 8;
    }
}

uint8_t Pulse::sample() const {
    if (length_counter.muted()) {
        return 0;
    }

    if (period < 8) {
        return 0;
    }

    if (sweep.enabled) {
        const int target = sweep.calculate_target(period);
        // TODO: cache the target to avoid recalculation?
        if (sweep.is_muted(period, target)) {
            return 0;
        }
    }

    if (!PulseDuty::active(seq_type, seq_idx)) {
        return 0;
    }

    return volume.get();
}

/***** Sweep *****/

void Sweep::half_frame_clock()
{
    divider.set_period(period);

    bool divider_pulse = false;
    if (reload) {
        divider.reset();
        reload = false;
    } else {
        divider_pulse = divider.clock();
    }

    const int current = pulse.period;
    const int target  = calculate_target(current);
    const bool muted  = enabled && is_muted(current, target);

    if (!muted && enabled && shift > 0 && divider_pulse) {
        pulse.set_period(target);
    }
}

int Sweep::calculate_target(int current) const {
    if (shift == 0) {
        return current; // ignore negation effects for shift=0
    }

    const int amt = current >> shift;
    if (!negate) {
        return current + amt;
    }

    const int negated = ones_complement ? current - amt - 1 : current - amt;
    return std::max(0, negated);
}
