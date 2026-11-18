#include "Amp.h"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace rocker {
namespace {
constexpr double pi = std::numbers::pi;
double sinc(double x) { return std::abs(x) < 1e-10 ? 1.0 : std::sin(pi * x) / (pi * x); }
// Complementary half-band allpass branches at 2x. Each section implements
// (a + z^-1)/(1 + a z^-1); all delay state belongs to this one amp.
constexpr std::array<float, 3> even{0.036681503f, 0.561098970f, 0.892260818f};
constexpr std::array<float, 3> odd{0.274631759f, 0.769741834f, 0.962094548f};
float allpass(float x, std::array<float, 3>& memory, const std::array<float, 3>& coefficients) noexcept {
    for (size_t i = 0; i < memory.size(); ++i) {
        const float y = coefficients[i] * x + memory[i];
        memory[i] = x - coefficients[i] * y;
        x = y;
    }
    return x;
}
float distort(float x, int shape) noexcept {
    switch (shape) {
    case 0:
        return std::tanh(x);
    case 1:
        return std::tanh(x + 0.22f) - std::tanh(0.22f);
    default:
        return std::clamp(x, -0.8f, 0.8f);
    }
}
} // namespace
void Amp::prepare(int sampleRate) {
    rate = sampleRate;
    auto pole = [&](float hz) { return static_cast<float>(1 - std::exp(-2 * pi * hz / rate)); };
    lowPole = pole(220);
    midPole = pole(1450);
    airPole = pole(3200);
    attack = 1 - std::exp(-1.f / (rate * 0.002f));
    release = std::exp(-1.f / (rate * 0.040f));
    // Original windowed-sinc cabinet IR, designed at the actual device rate.
    // Band-limited speaker with a midrange peak and a softer upper midrange.
    const int half = std::max(8, static_cast<int>(std::round(rate * 0.0015)));
    cabinet.resize(2 * half + 1);
    history.resize(cabinet.size());
    for (int i = -half; i <= half; ++i) {
        auto lp = [&](double hz) {
            const auto f = 2 * hz / rate;
            return f * sinc(i * f);
        };
        const auto window = 0.42 + 0.5 * std::cos(pi * i / half) + 0.08 * std::cos(2 * pi * i / half);
        cabinet[i + half] = static_cast<float>(
            window * (lp(4200) - lp(100) + 0.32 * (lp(1550) - lp(700)) - 0.25 * (lp(3400) - lp(2300))));
    }
    configure(params);
    reset();
}
void Amp::configure(AmpParameters p) {
    params = p;
    target.gain = std::pow(10.f, p.drive * 0.015f);
    target.level = 0.75f * std::pow(p.volume / 100.f, 1.6f);
    target.threshold = std::pow(10.f, -p.gate / 20.f);
    auto eq = [](int n) { return std::pow(10.f, (n - 50) * 0.012f); };
    target.bass = eq(p.bass);
    target.mid = eq(p.mid);
    target.treble = eq(p.treble);
    target.presence = eq(p.presence) - 1;
}
void Amp::reset() noexcept {
    upEven = {};
    upOdd = {};
    downEven = {};
    downOdd = {};
    previousOdd = 0;
    std::fill(history.begin(), history.end(), 0);
    current = target;
    head = 0;
    env = gate = low = wide = air = 0;
}
float Amp::process(float x) noexcept {
    auto smooth = [this](float& value, float destination) { value += (destination - value) * attack; };
    smooth(current.gain, target.gain);
    smooth(current.level, target.level);
    smooth(current.threshold, target.threshold);
    smooth(current.bass, target.bass);
    smooth(current.mid, target.mid);
    smooth(current.treble, target.treble);
    smooth(current.presence, target.presence);
    env = std::max(std::abs(x), env * release);
    const float openness = env >= current.threshold ? 1.f : 0.f;
    gate += (openness - gate) * (openness > gate ? attack : 1 - release);
    const float a = distort(current.gain * allpass(x * gate, upEven, even), params.shape);
    const float b = distort(current.gain * allpass(x * gate, upOdd, odd), params.shape);
    const float wet = 0.5f * (allpass(a, downEven, even) + previousOdd);
    previousOdd = allpass(b, downOdd, odd);
    low += lowPole * (wet - low);
    wide += midPole * (wet - wide);
    air += airPole * (wet - air);
    history[head] = current.bass * low + current.mid * (wide - low) + current.treble * (wet - wide) +
                    current.presence * (wet - air);
    float result = 0;
    int pos = head;
    for (float tap : cabinet) {
        result += tap * history[pos];
        if (--pos < 0)
            pos = static_cast<int>(history.size()) - 1;
    }
    if (++head == static_cast<int>(history.size()))
        head = 0;
    return std::clamp(result * current.level, -1.f, 1.f);
}
} // namespace rocker
