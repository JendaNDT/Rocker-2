#include "Live.h"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace rocker {
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<Measurement>::is_always_lock_free &&
              std::atomic<Monitor>::is_always_lock_free);
void Live::prepare(int sampleRate) {
    rate = sampleRate;
    stride = std::max(1, static_cast<int>(std::round(rate / 12000.)));
    amp.prepare(rate);
    applied = 0;
    previousMode = Monitor::off;
    monitor = Monitor::off;
    input = 0;
    stereoRight = -1;
    phase = Measurement::idle;
    cancel = false;
    noiseLeft = cueLeft = cueCommand = noiseCommand = 0;
    learned = -1;
    pitchReady = false;
    tuning = false;
    ++generation;
    outputPeak = 0;
    cpuPercent = 0;
    response.assign(rate / 2 + probe.size(), 0);
    // Period-127 bipolar sequence, four samples/chip for analogue bandwidth.
    unsigned state = 0x7f;
    for (size_t i = 0; i < probe.size(); ++i) {
        probe[i] = (state & 1) ? 0.0630957f : -0.0630957f;
        if ((i % 4) == 3)
            state = (state >> 1) | (((state ^ (state >> 1)) & 1) << 6);
    }
}
void Live::selectInput(int index) {
    stereoRight = -1;
    input = index;
    ++generation;
    noiseCommand = -1;
    learned = -1;
}
void Live::setTuner(bool active) {
    ++generation;
    tuning = active;
    cueCommand = -1;
}
void Live::cue(double hz, bool confirmation) {
    // Positive millihertz, or a short negative-frequency confirmation.
    int value = static_cast<int>(std::round(1000 * hz));
    cueCommand = confirmation ? -value : value;
}
void Live::silence() {
    monitor = Monitor::off;
    setTuner(false);
    noiseCommand = -1;
    learned = -1;
    cancel = true;
}
void Live::learnNoise() {
    learned = -1;
    noiseCommand = 1;
}
std::optional<int> Live::takeThreshold() {
    const int result = learned.exchange(-1);
    if (result < 0)
        return {};
    return result;
}
bool Live::startMeasurement() {
    auto state = Measurement::idle;
    monitor = Monitor::off;
    cueCommand = -1;
    cancel = false;
    return phase.compare_exchange_strong(state, Measurement::requested);
}
std::optional<LatencyResult> Live::takeMeasurement() {
    if (phase.load() != Measurement::ready)
        return {};
    double probeEnergy = 0, energy = 0;
    for (size_t i = 0; i < probe.size(); ++i) {
        probeEnergy += probe[i] * probe[i];
        energy += response[i] * response[i];
    }
    LatencyResult best{-1, 0};
    for (size_t delay = 0; delay + probe.size() <= response.size(); ++delay) {
        if (energy > probe.size() * 1e-7) {
            double dot = 0;
            for (size_t i = 0; i < probe.size(); ++i)
                dot += response[delay + i] * probe[i];
            const auto score = static_cast<float>(std::abs(dot) / std::sqrt(energy * probeEnergy));
            if (score > best.confidence)
                best = {static_cast<int>(delay), score};
        }
        if (delay + probe.size() < response.size())
            energy += response[delay + probe.size()] * response[delay + probe.size()] -
                      response[delay] * response[delay];
    }
    if (best.confidence < 0.65f)
        best.samples = -1;
    phase = Measurement::idle;
    return best;
}
void Live::collectPitch(float x) noexcept {
    groupSum += x;
    if (++samplesInGroup != stride)
        return;
    pitchFrame[pitchIndex++] = groupSum / stride;
    samplesInGroup = 0;
    groupSum = 0;
    if (pitchIndex != static_cast<int>(pitchFrame.size()))
        return;
    pitchIndex = 0;
    if (!pitchReady.load(std::memory_order_acquire)) {
        mailbox = pitchFrame;
        publishedGeneration = writingGeneration;
        pitchReady.store(true, std::memory_order_release);
    }
}
std::optional<double> Live::takePitch() {
    if (!pitchReady.load(std::memory_order_acquire))
        return {};
    std::optional<double> hz;
    if (publishedGeneration == generation.load())
        hz = Pitch{}.detect(mailbox, static_cast<double>(rate) / stride);
    pitchReady.store(false, std::memory_order_release);
    return hz;
}
void Live::render(const float* const* in, int ni, float* const* out, int no, int frames) noexcept {
    if (cancel.exchange(false))
        phase = Measurement::idle;
    auto state = phase.load();
    if (state == Measurement::requested) {
        cursor = 0;
        phase = state = Measurement::recording;
    }
    const auto mode = monitor.load();
    const auto snapshot = parameters.consume();
    const auto packed = snapshot & AmpCommand::valueMask;
    const bool reset =
        (snapshot & AmpCommand::recallBit) || (mode == Monitor::amp && previousMode != mode);
    if (applied != packed || reset) {
        amp.configure(AmpParameters::unpack(packed));
        if (reset)
            amp.reset();
        applied = packed;
    }
    previousMode = mode;
    const auto epoch = generation.load();
    if (writingGeneration != epoch) {
        writingGeneration = epoch;
        pitchIndex = samplesInGroup = 0;
        groupSum = 0;
    }
    const bool tune = tuning.load();
    const auto noise = noiseCommand.exchange(0);
    if (noise != 0) {
        noiseLeft = noise > 0 ? 2 * rate : 0;
        noisePeak = 0;
    }
    const auto command = cueCommand.exchange(0);
    if (command == -1)
        cueLeft = 0;
    else if (command == -2) {
        releasing = true;
        cueLeft = std::min(cueLeft, std::min(rate / 200, cueLength - cueLeft));
    } else if (command != 0 && !(command < 0 && reference && cueLeft > 0 && !releasing)) {
        releasing = false;
        reference = command > 0;
        cueLength = reference ? rate : rate / 8;
        cueLeft = cueLength;
        angle = 0;
        step = 2 * std::numbers::pi * std::abs(command) / (1000. * rate);
    }
    const int right = stereoRight.load();
    const auto channel = input.load();
    const float* source = channel >= 0 && channel < ni ? in[channel] : nullptr;
    float peak = 0;
    for (int i = 0; i < frames; ++i) {
        const float x = source ? source[i] : 0;
        float y = 0;
        if (state == Measurement::recording) {
            response[cursor] = x;
            if (cursor < static_cast<int>(probe.size()) && no > 0 && out[0])
                out[0][i] = probe[cursor];
            if (++cursor == static_cast<int>(response.size()))
                phase = state = Measurement::ready;
            continue;
        }
        if (tune)
            collectPitch(x);
        if (noiseLeft > 0) {
            noisePeak = std::max(noisePeak, std::abs(x));
            if (--noiseLeft == 0)
                learned = std::clamp(
                    static_cast<int>(std::round(-20 * std::log10(std::max(0.00001f, noisePeak)) - 6)),
                    24, 84);
        }
        if (mode == Monitor::clean)
            y = x;
        else if (mode == Monitor::amp)
            y = amp.process(x);
        if (cueLeft > 0) {
            const int edge = std::min(cueLeft, cueLength - cueLeft);
            const auto wave = reference ? 0.80 * std::sin(angle) + 0.16 * std::sin(2 * angle) +
                                              0.08 * std::sin(3 * angle)
                                        : std::sin(angle);
            y += 0.12f * static_cast<float>(wave) * std::min(1.f, edge / (rate * 0.005f));
            angle += step;
            if (angle >= 2 * std::numbers::pi)
                angle -= 2 * std::numbers::pi;
            if (!reference || releasing || cueLeft > cueLength / 2)
                --cueLeft;
        }
        for (int c = 0; c < std::min(2, no); ++c)
            if (out[c]) {
                out[c][i] = std::clamp(out[c][i] + y +
                                           (mode == Monitor::clean && c == 1 && right >= 0
                                                ? (right < ni && in[right] ? in[right][i] : 0.f) - x
                                                : 0.f),
                                       -1.f, 1.f);
                peak = std::max(peak, std::abs(out[c][i]));
            }
    }
    outputPeak = peak;
}
} // namespace rocker
