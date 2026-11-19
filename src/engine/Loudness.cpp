#include "Loudness.h"
#include "song/Song.h"
#include <ebur128.h>
#include <deque>

namespace rocker {
using namespace juce;
namespace {
constexpr int blockSize = 1024;
struct Meter {
    ebur128_state* state = nullptr;
    explicit Meter(int rate) : state(ebur128_init(2, rate, EBUR128_MODE_I | EBUR128_MODE_TRUE_PEAK)) {}
    ~Meter() {
        if (state)
            ebur128_destroy(&state);
    }
};
} // namespace

std::optional<LoudnessReading> measureLoudness(AudioFormatReader& reader) {
    if (reader.numChannels != 2 || reader.sampleRate < 8000 || reader.sampleRate > 384000)
        return {};
    Meter meter(roundToInt(reader.sampleRate));
    if (!meter.state)
        return {};
    AudioBuffer<float> input(2, blockSize);
    std::array<float, 2 * blockSize> interleaved;
    for (Frame at = 0; at < reader.lengthInSamples; at += blockSize) {
        int count = int(std::min<Frame>(blockSize, reader.lengthInSamples - at));
        if (!reader.read(&input, 0, count, at, true, true))
            return {};
        for (int i = 0; i < count; ++i) {
            interleaved[2 * i] = input.getSample(0, i);
            interleaved[2 * i + 1] = input.getSample(1, i);
        }
        if (ebur128_add_frames_float(meter.state, interleaved.data(), size_t(count)) != EBUR128_SUCCESS)
            return {};
    }
    double loudness = 0, left = 0, right = 0;
    if (ebur128_loudness_global(meter.state, &loudness) != EBUR128_SUCCESS ||
        ebur128_true_peak(meter.state, 0, &left) != EBUR128_SUCCESS ||
        ebur128_true_peak(meter.state, 1, &right) != EBUR128_SUCCESS)
        return {};
    return LoudnessReading{loudness, std::max(left, right)};
}

bool writeLimitedMix(AudioFormatReader& reader, const File& path, double gain, double ceiling) {
    if (reader.numChannels != 2 || !std::isfinite(gain) || gain <= 0 || ceiling <= 0)
        return false;
    const auto rate = roundToInt(reader.sampleRate);
    auto writer = waveWriter(path, rate, 2);
    if (!writer)
        return false;
    const int ahead = std::max(1, int(rate * .006));
    const auto release = 1. - std::exp(-1. / (.09 * rate));
    AudioBuffer<float> input(2, blockSize + ahead);
    AudioBuffer<float> output(2, blockSize);
    double applied = gain;
    for (Frame at = 0; at < reader.lengthInSamples; at += blockSize) {
        const int count = int(std::min<Frame>(blockSize, reader.lengthInSamples - at));
        if (!reader.read(&input, 0, count + ahead, at, true, true))
            return false;
        auto peak = [&](int i) {
            return double(std::max(std::abs(input.getSample(0, i)), std::abs(input.getSample(1, i))));
        };
        std::deque<int> maxima;
        auto add = [&](int i) {
            while (!maxima.empty() && peak(maxima.back()) <= peak(i))
                maxima.pop_back();
            maxima.push_back(i);
        };
        for (int i = 0; i < ahead; ++i)
            add(i);
        for (int i = 0; i < count; ++i) {
            add(i + ahead);
            while (maxima.front() < i)
                maxima.pop_front();
            const double allowed = std::min(gain, ceiling / std::max(1e-12, peak(maxima.front())));
            const auto distance = maxima.front() - i + 1;
            if (allowed < applied)
                applied += (allowed - applied) / distance;
            else
                applied = std::min(allowed, applied + release * (gain - applied));
            applied = std::min(applied, ceiling / std::max(1e-12, peak(i)));
            for (int ch = 0; ch < 2; ++ch)
                output.setSample(ch, i, float(input.getSample(ch, i) * applied));
        }
        if (!writer->writeFromAudioSampleBuffer(output, 0, count))
            return false;
    }
    return writer->flush();
}
} // namespace rocker
