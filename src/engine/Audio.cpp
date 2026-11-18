#include "Audio.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <limits>

namespace rocker {
static_assert(std::atomic<float>::is_always_lock_free && std::atomic<int>::is_always_lock_free &&
              std::atomic<bool>::is_always_lock_free && std::atomic<std::uint32_t>::is_always_lock_free);
float decibels(float value) noexcept {
    return value > 0 ? 20.0f * std::log10(value) : -std::numeric_limits<float>::infinity();
}
void Audio::prepare(int inputs, int sampleRate) {
    count = inputs;
    rate = sampleRate;
    position = -1;
    command = 0;
    meters = std::make_unique<Meter[]>(count);
}
Reading Audio::read(int channel) const noexcept {
    if (channel < 0 || channel >= count)
        return {};
    const auto& m = meters[channel];
    return {m.peak.load(), m.held.load(), m.clipped.load(), m.activity.load(std::memory_order_relaxed)};
}
void Audio::clearClip(int channel) noexcept {
    if (channel >= 0 && channel < count)
        meters[channel].clipped = false;
}
void Audio::process(const float* const* inputs, int inputCount, float* const* outputs, int outputCount,
                    int frames) noexcept {
    for (int c = 0; c < count; ++c) {
        auto& m = meters[c];
        const float* samples = c < inputCount ? inputs[c] : nullptr;
        float peak = 0;
        for (int i = 0; i < frames; ++i) {
            const float sample = samples ? std::abs(samples[i]) : 0;
            peak = std::max(peak, sample);
            if (sample >= m.hold || --m.remaining <= 0) {
                m.hold = sample;
                m.remaining = (3 * rate + 1) / 2;
            }
        }
        // Keep short plucks visible to input identification between UI polls (-36 dBFS).
        if (peak >= 0.015848932f)
            m.activity.fetch_add(1, std::memory_order_relaxed);
        m.peak.store(peak);
        m.held.store(m.hold);
        if (peak >= 0.9885530947f)
            m.clipped.store(true); // 10^(-0.1/20)
    }
    const auto action = command.exchange(0);
    if (action != 0)
        position = action > 0 ? 0 : -1;
    for (int c = 0; c < outputCount; ++c)
        if (outputs[c])
            std::fill_n(outputs[c], frames, 0.0f);
    for (int i = 0; i < frames && position >= 0; ++i) {
        const float value = static_cast<float>(
            0.125892541179 * std::sin(2.0 * std::numbers::pi * 440.0 * position / rate));
        if (outputCount > 0 && outputs[0] &&
            (outputCount == 1 || position < rate || position >= 2 * rate))
            outputs[0][i] = value;
        if (outputCount > 1 && outputs[1] && position >= rate)
            outputs[1][i] = value;
        if (++position == 3 * rate)
            position = -1;
    }
}
} // namespace rocker
