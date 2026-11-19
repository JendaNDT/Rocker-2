#include "Voice.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace rocker {
void VoiceCommand::publish(const VoiceSound& sound, bool recall) noexcept {
    const auto next = sound.values.pack() | (sound.preset != 0 ? enabled : 0) | (recall ? reset : 0);
    auto previous = word.load();
    while (!word.compare_exchange_weak(previous, next | (previous & reset))) {
    }
}

std::uint64_t VoiceCommand::consume() noexcept {
    auto snapshot = word.load();
    return snapshot & reset ? word.fetch_and(~reset) : snapshot;
}

void Voice::prepare(int sampleRate) {
    const auto pole = [sampleRate](float hz) { return 1.f - std::exp(-2.f * std::numbers::pi_v<float> * hz / sampleRate); };
    lowPole = pole(220);
    widePole = pole(2400);
    attack = 1.f - std::exp(-1.f / (.008f * sampleRate));
    release = 1.f - std::exp(-1.f / (.105f * sampleRate));
    smoothing = 1.f - std::exp(-1.f / (.018f * sampleRate));
    constexpr std::array<float, 3> seconds{.0197f, .0273f, .0419f};
    for (size_t i = 0; i < seconds.size(); ++i)
        reflections[i].ring.resize(std::max(1, int(std::lround(seconds[i] * sampleRate))));
    reset();
}

void Voice::configure(VoiceParameters p) noexcept {
    auto gain = [](int value) { return std::pow(10.f, (std::clamp(value, 0, 24) - 12) / 20.f); };
    target = {gain(p.low), gain(p.mid), gain(p.high), std::min(100, int(p.compression)) / 100.f,
              std::min(100, int(p.room)) / 100.f};
}

void Voice::reset() noexcept {
    current = target;
    low = wide = detector = 0;
    for (auto& reflection : reflections)
        reflection.reset();
}

float Voice::Reflection::process(float input) noexcept {
    const float delayed = valid < ring.size() ? 0.f : ring[index];
    ring[index] = input + .38f * delayed;
    if (++index == ring.size())
        index = 0;
    valid = std::min(ring.size(), valid + 1);
    return delayed;
}

float Voice::process(float input) noexcept {
    auto smooth = [this](float& value, float targetValue) { value += smoothing * (targetValue - value); };
    smooth(current.low, target.low);
    smooth(current.mid, target.mid);
    smooth(current.high, target.high);
    smooth(current.compression, target.compression);
    smooth(current.room, target.room);
    low += lowPole * (input - low);
    wide += widePole * (input - wide);
    float sample = current.low * low + current.mid * (wide - low) + current.high * (input - wide);
    const auto level = std::abs(sample);
    detector += (level > detector ? attack : release) * (level - detector);
    const auto threshold = .4f - .28f * current.compression;
    if (detector > threshold) {
        const auto ratio = 1.f + 4.f * current.compression;
        sample *= std::pow(threshold / detector, 1.f - 1.f / ratio);
    }
    float reflected = 0;
    for (auto& line : reflections)
        reflected += line.process(sample);
    return sample + current.room * .22f * reflected;
}
} // namespace rocker
