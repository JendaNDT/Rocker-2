#pragma once
#include "Live.h"
#include "Timeline.h"
#include <atomic>
#include <cstdint>
#include <memory>

namespace rocker {
struct Reading {
    float peak, held;
    bool clipped;
    std::uint32_t activity;
};
float decibels(float amplitude) noexcept;

class Audio {
  public:
    // The device must be stopped before changing the channel layout.
    Live live;
    Timeline timeline;
    void prepare(int inputs, int sampleRate);
    void playTone() noexcept { command.store(1); }
    void stopTone() noexcept { command.store(-1); }
    Reading read(int channel) const noexcept;
    void clearClip(int channel) noexcept;
    void process(const float* const* inputs, int inputCount, float* const* outputs, int outputCount,
                 int frames) noexcept;

  private:
    struct Meter {
        float hold = 0;
        int remaining = 0;
        std::atomic<float> peak{0}, held{0};
        std::atomic<bool> clipped{false};
        std::atomic<std::uint32_t> activity{0};
    };
    std::unique_ptr<Meter[]> meters;
    int count = 0, rate = 48000, position = -1;
    std::atomic<int> command{0};
};
} // namespace rocker
