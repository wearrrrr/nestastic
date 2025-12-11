#pragma once

#include "AudioPlayer.h"
#include "dmc.h"
#include "frame_counter.h"
#include "noise.h"
#include "pulse.h"
#include "timer.h"
#include "triangle.h"
#include "spsc.hpp"
#include "../irq.h"

class APU
{
public:
    Pulse        pulse1 { Pulse::Type::Pulse1 };
    Pulse        pulse2 { Pulse::Type::Pulse2 };
    Triangle     triangle;
    Noise        noise;
    DMC          dmc;

    FrameCounter frame_counter;

public:
    APU(AudioPlayer& player, IRQ& irq, std::function<uint8_t(uint16_t)> dmcDma) :
      dmc(irq, dmcDma),
      frame_counter(setup_frame_counter(irq)),
      audio_queue(player.audio_queue),
      sampling_timer(nanoseconds(int64_t(1e9) / int64_t(player.output_sample_rate))) {}

    // clock at the same frequency as the cpu
    void step();

    void writeRegister(uint16_t addr, uint8_t value);
    uint8_t readStatus();

private:
    FrameCounter             setup_frame_counter(IRQ &irq);
    bool                     divideByTwo = false;

    spsc::RingBuffer<float> &audio_queue;
    Timer sampling_timer;
};
