#pragma once

// Simple SDL-based audio output utility to act as a drop-in replacement
// for the previous miniaudio-based AudioPlayer. This header-only class
// exposes a small API very similar to the original:
//   - AudioPlayer(int input_rate)
//   - ~AudioPlayer()
//   - bool start()
//   - void mute()
// and it exposes a public spsc::RingBuffer<float> called `audio_queue` for
// the producer (APU) to push generated float samples (mono, range approx -1..1).
//
// Implementation notes:
//  - Uses SDL audio callback to pull samples from the ring buffer.
//  - Performs a simple linear resampling from `input_sample_rate` -> device rate.
//  - The audio device is opened with float samples (AUDIO_F32SYS), mono.
//  - The ring buffer is single-producer single-consumer as provided by spsc::RingBuffer.
//  - This header purposely keeps the implementation inlined for simplicity.

#include <SDL.h>
#include <vector>
#include <atomic>
#include <algorithm>
#include <chrono>

#include "spsc.hpp"

class AudioPlayer {
public:
    // target output sample rate we'd like to use (used as a hint when opening the device)
    // This mirrors the previous implementation which used 44100 as the standard output.
    const int output_sample_rate = 44100;

    explicit AudioPlayer(int input_rate)
      : input_sample_rate(input_rate)
      // allocate a ring-buffer big enough to keep several callbacks worth of samples
      , audio_queue(4 * input_rate * (callback_period_ms.count() / 100))
    {
        // initialize members with defaults
        device_id_ = 0;
        device_opened_ = false;
        mute_.store(false);
        src_pos_ = 0.0;
        have_sample_rate_ = output_sample_rate; // will be overwritten with actual device rate on start()
    }

    ~AudioPlayer()
    {
        stopAndClose();
    }

    // Start the SDL audio device and unpause it.
    // Returns true on success.
    bool start()
    {
        if (device_opened_)
            return true; // already started

        // Ensure SDL audio subsystem is initialized. SDL_InitSubSystem is safe to call multiple times.
        if (SDL_WasInit(SDL_INIT_AUDIO) == 0)
        {
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
            {
                SDL_Log("AudioPlayer: Failed to init SDL audio subsystem: %s", SDL_GetError());
                return false;
            }
        }

        SDL_AudioSpec want;
        SDL_zero(want);
        want.freq = output_sample_rate;
        want.format = AUDIO_F32SYS; // request float samples
        want.channels = 1; // mono
        want.callback = &AudioPlayer::sdlAudioCallback;
        want.userdata = this;

        SDL_AudioSpec have;
        SDL_zero(have);

        device_id_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
        if (device_id_ == 0)
        {
            SDL_Log("AudioPlayer: Failed to open SDL audio device: %s", SDL_GetError());
            return false;
        }

        // Save the actual sample rate (device may not match requested exactly).
        have_sample_rate_ = have.freq > 0 ? have.freq : want.freq;
        device_samples_ = have.samples;
        device_opened_ = true;

        // Reset resampling state (so we don't have large jumps when starting)
        src_pos_ = 0.0;
        input_cache_.clear();
        last_output_zero_fill_ = 0;

        // Unpause device to start callbacks
        SDL_PauseAudioDevice(device_id_, 0);
        return true;
    }

    // Signal that audio output should be muted (affects the callback).
    void mute()
    {
        mute_.store(true);
    }

    // Un-mute audio output.
    void unmute()
    {
        mute_.store(false);
    }

    // Public: input sample rate (samples/sec) for the data being pushed into `audio_queue`.
    const int input_sample_rate;

    // Public: single-producer/single-consumer audio queue. Producer (APU) writes here,
    // SDL audio callback reads from it. Values are float mono samples.
    spsc::RingBuffer<float> audio_queue;

private:
    // small constant used for sizing heuristic in the constructor
    static constexpr std::chrono::milliseconds callback_period_ms { 120 };

    // SDL device handle and bookkeeping
    SDL_AudioDeviceID device_id_;
    bool device_opened_;
    int have_sample_rate_;
    Uint16 device_samples_;

    std::atomic<bool> mute_;

    // Simple resampling state kept per-instance and only used from audio thread callback
    // `src_pos_` is the fractional read index into `input_cache_` (in input-sample-space).
    // Use a circular buffer for `input_cache_` to avoid expensive front-erase operations
    // which can cause GC/alloc stalls visible as crackling in the audio callback.
    double src_pos_;

    // Ring-buffer storage for input samples. We avoid repeatedly erasing from the front
    // by maintaining a head index and size. The underlying vector may reserve and reuse
    // capacity to minimize allocations during audio callbacks.
    std::vector<float> input_cache_;
    size_t input_cache_head_ = 0;
    size_t input_cache_size_ = 0;
    static constexpr size_t input_cache_capacity_ = 1 << 16; // 65536 samples capacity

    int last_output_zero_fill_ = 0; // diagnostic: how many times we filled with silence

    // Helper to stop and close the device (idempotent)
    void stopAndClose()
    {
        if (device_opened_ && device_id_ != 0)
        {
            // pause first
            SDL_PauseAudioDevice(device_id_, 1);
            SDL_CloseAudioDevice(device_id_);
            device_id_ = 0;
            device_opened_ = false;
        }
    }

    // The SDL callback is a C-style static function that dispatches to the instance method.
    static void sdlAudioCallback(void* userdata, Uint8* stream, int len)
    {
        auto* self = static_cast<AudioPlayer*>(userdata);
        if (!self) return;
        self->audioCallbackImpl(stream, len);
    }

    // Core callback implementation. Called from SDL audio thread.
    // `stream` points to a buffer of `len` bytes that must be filled.
    void audioCallbackImpl(Uint8* stream, int len)
    {
        // We requested AUDIO_F32SYS and mono, so len should be a multiple of sizeof(float).
        const size_t out_bytes = static_cast<size_t>(len);
        const size_t out_frames = out_bytes / sizeof(float);
        float* out = reinterpret_cast<float*>(stream);

        // If muted, output silence and do not advance any internal position.
        if (mute_.load())
        {
            std::fill(out, out + out_frames, 0.0f);
            return;
        }

        // Compute the resampling ratio: how many input samples correspond to one output sample.
        // pos increment (in input-sample units) per output sample:
        const double src_inc = static_cast<double>(input_sample_rate) / static_cast<double>(have_sample_rate_);

        // Estimate how many input samples we'll need to produce `out_frames` outputs:
        // We need at least ceil(out_frames * src_inc) + 2 samples to safely interpolate.
        const size_t required_input = static_cast<size_t>(std::ceil(out_frames * src_inc)) + 2;

        // Ensure the underlying ring buffer has capacity reserved to avoid repeated allocations.
        if (input_cache_.capacity() < input_cache_capacity_) {
            input_cache_.reserve(input_cache_capacity_);
        }

        // How many samples are currently available in our ring buffer view?
        size_t available = input_cache_size_;

        // If we don't have enough samples, pop more from the producer ring buffer.
        if (available < required_input)
        {
            const size_t need = required_input - available;

            // Temporary local buffer to receive popped samples
            std::vector<float> tmp;
            tmp.resize(need);

            size_t popped = audio_queue.pop(tmp.data(), tmp.size());
            // If we popped fewer than requested, shrink tmp accordingly.
            if (popped < tmp.size()) {
                tmp.resize(popped);
            }

            // Append popped samples into our circular storage without reallocating front.
            for (size_t i = 0; i < tmp.size(); ++i)
            {
                // If underlying storage hasn't grown to full capacity, push_back to grow it.
                if (input_cache_.size() < input_cache_capacity_)
                {
                    input_cache_.push_back(tmp[i]);
                }
                else
                {
                    // Overwrite at tail position when at capacity
                    size_t tail_idx = (input_cache_head_ + input_cache_size_) % input_cache_capacity_;
                    input_cache_[tail_idx] = tmp[i];
                }
                input_cache_size_ = std::min(input_cache_size_ + 1, input_cache_capacity_);
            }

            // If we still didn't get enough samples from the producer, pad with zeros.
            if (input_cache_size_ < required_input)
            {
                size_t pad = required_input - input_cache_size_;
                for (size_t p = 0; p < pad; ++p)
                {
                    if (input_cache_.size() < input_cache_capacity_)
                    {
                        input_cache_.push_back(0.0f);
                    }
                    else
                    {
                        size_t tail_idx = (input_cache_head_ + input_cache_size_) % input_cache_capacity_;
                        input_cache_[tail_idx] = 0.0f;
                    }
                    input_cache_size_ = std::min(input_cache_size_ + 1, input_cache_capacity_);
                }
                ++last_output_zero_fill_;
            }
        }

        // Now perform linear resampling: for each output frame, sample input at src_pos_ and advance.
        double pos = src_pos_; // fractional position into the logical input_cache_ (in input-sample units)

        // Determine the maximum index we can interpolate up to.
        const double max_index_for_sample = static_cast<double>(input_cache_size_) - 2.0;

        for (size_t i = 0; i < out_frames; ++i)
        {
            if (pos > max_index_for_sample)
            {
                out[i] = 0.0f;
            }
            else
            {
                const size_t idx = static_cast<size_t>(pos);
                const double frac = pos - static_cast<double>(idx);

                // Map logical indices into the underlying circular buffer
                size_t index0 = (input_cache_head_ + idx) % input_cache_capacity_;
                size_t index1 = (input_cache_head_ + idx + 1) % input_cache_capacity_;

                float x0 = input_cache_.empty() ? 0.0f : input_cache_[index0];
                float x1 = input_cache_.empty() ? 0.0f : input_cache_[index1];

                out[i] = static_cast<float>(x0 + (x1 - x0) * frac);
            }
            pos += src_inc;
        }

        // Advance src_pos_ and drop consumed samples from the circular buffer.
        const size_t consumed = static_cast<size_t>(std::floor(pos));
        src_pos_ = pos - static_cast<double>(consumed);

        if (consumed > 0)
        {
            // Advance the head by 'consumed' samples (mod capacity) and shrink size.
            if (consumed >= input_cache_size_)
            {
                // Consumed all available samples: reset buffer head/size.
                input_cache_head_ = 0;
                input_cache_size_ = 0;
                input_cache_.clear(); // keep capacity
            }
            else
            {
                input_cache_head_ = (input_cache_head_ + consumed) % input_cache_capacity_;
                input_cache_size_ -= consumed;

                // Periodic compaction: if the underlying vector grew large and head is near capacity,
                // move valid data back to start to keep index math simple and to reduce cache misses.
                // This is done rarely (only when head is near the end and size is smallish).
                if (input_cache_head_ != 0 && input_cache_size_ > 0 && input_cache_head_ > (input_cache_capacity_ / 2))
                {
                    // Move data to the front of the vector to reset head to zero.
                    std::vector<float> tmp;
                    tmp.reserve(input_cache_size_);
                    for (size_t i = 0; i < input_cache_size_; ++i)
                    {
                        size_t idx = (input_cache_head_ + i) % input_cache_capacity_;
                        tmp.push_back(input_cache_[idx]);
                    }
                    input_cache_.swap(tmp);
                    input_cache_head_ = 0;
                }
            }
        }
    }
};
