#include "Export.h"
#include "storage/AtomicFile.h"
#include <numbers>

namespace rocker {
using namespace juce;
namespace {
// Fractional-delay low-pass filter. The same phase is used for both channels.
constexpr int radius = 32, phases = 1024;
using Kernel = std::array<float, radius * 2>;
std::vector<Kernel> kernels(double cutoff) {
    std::vector<Kernel> table(phases);
    constexpr double pi = std::numbers::pi;
    for (int phase = 0; phase < phases; ++phase) {
        double sum = 0;
        for (int k = 0; k < radius * 2; ++k) {
            const double x = k - radius + 1 - double(phase) / phases;
            const double window =
                0.42 + 0.5 * std::cos(pi * x / radius) + 0.08 * std::cos(2 * pi * x / radius);
            sum += table[phase][k] =
                float(window * (x == 0 ? cutoff : std::sin(pi * cutoff * x) / (pi * x)));
        }
        for (auto& value : table[phase])
            value /= float(sum);
    }
    return table;
}
struct Noise {
    uint32_t state = 42893;
    double next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return double(state) / 4294967296.;
    }
};
bool convert(AudioFormatReader& input, const File& path, int rate, int bits, Frame begin,
             Playback::Meter& meter) {
    std::unique_ptr<OutputStream> stream = path.createOutputStream();
    if (!stream)
        return false;
    auto writer = WavAudioFormat().createWriterFor(
        stream,
        AudioFormatWriterOptions{}.withSampleRate(rate).withNumChannels(2).withBitsPerSample(bits));
    if (!writer)
        return false;
    const int sourceRate = roundToInt(input.sampleRate);
    const double ratio = double(sourceRate) / rate;
    const auto table = rate == sourceRate ? std::vector<Kernel>{} : kernels(std::min(1., 1. / ratio));
    constexpr int block = 512;
    AudioBuffer<float> source(2, int(std::ceil(block * ratio)) + radius * 2 + 2);
    int pcm[2][block];
    const int* channels[]{pcm[0], pcm[1]};
    Noise noise;
    const int scale = 1 << (bits - 1);
    const Frame frames = std::llround(input.lengthInSamples / ratio);
    for (Frame pos = 0; pos < frames; pos += block) {
        const int count = int(std::min<Frame>(block, frames - pos));
        const Frame first = pos * sourceRate / rate - radius + 1;
        if (!input.read(&source, 0, source.getNumSamples(), first, true, true))
            return false;
        for (int i = 0; i < count; ++i) {
            const Frame time = (pos + i) * sourceRate;
            int centre = int(time / rate - first);
            int phase = int((time % rate) * phases / rate);
            for (int ch = 0; ch < 2; ++ch) {
                const float* data = source.getReadPointer(ch);
                double sample = 0;
                if (table.empty())
                    sample = data[centre];
                else
                    for (int k = 0; k < radius * 2; ++k)
                        sample += data[centre - radius + 1 + k] * table[phase][k];
                float level = float(std::abs(sample));
                if (level > meter.maximum) {
                    meter.maximum = level;
                    meter.loudest = begin + time / rate;
                }
                if (level > 1 && meter.over < 0)
                    meter.over = begin + time / rate;
                const double a = bits == 16 ? noise.next() : 0.;
                const double b = bits == 16 ? noise.next() : 0.;
                auto quantized = std::llround(std::clamp(sample, -1., 1.) * scale + a - b);
                pcm[ch][i] = int(std::clamp(quantized, -(long long)scale, (long long)scale - 1)) *
                             (1 << (32 - bits));
            }
        }
        if (!writer->write(channels, count))
            return false;
    }
    return writer->flush();
}
} // namespace
bool deliveryAvailable(Delivery format) {
    switch (format) {
    case Delivery::wave24:
    case Delivery::wave16:
    case Delivery::stems:
        return true;
    case Delivery::mp3:
        return mp3Available();
    case Delivery::m4a:
#if JUCE_MAC
        return true;
#else
        return false;
#endif
    }
    return false;
}
MixFile exportMix(const Song& song, const AudioLibrary& lib, const File& dest, Delivery format,
                  Frame begin, Frame end, bool louder) {
    juce::ignoreUnused(louder);
    MixFile result{dest};
    result.begin = begin;
    if (dest.getParentDirectory().createDirectory().failed() ||
        !deliveryAvailable(format) || format == Delivery::stems)
        return result;
    TemporaryFile stage(".wav"), pcm(".wav"), output(dest);
    if (!renderSong(song, lib, stage.getFile(), begin, end))
        return result;
    auto input = waveReader(stage.getFile());
    if (!input)
        return result;
    const bool compressed = format == Delivery::mp3 || format == Delivery::m4a;
    if (!convert(*input, compressed ? pcm.getFile() : output.getFile(),
                 format == Delivery::wave24 ? 48000 : 44100, format == Delivery::wave24 ? 24 : 16, begin,
                 result.meter))
        return result;
    if (format == Delivery::mp3 && (!encodeMp3(pcm.getFile(), output.getFile()) ||
                !tagMp3Length(output.getFile(),std::llround(input->lengthInSamples*44100./input->sampleRate))))
        return result;
    if (format == Delivery::m4a && !encodeAac(pcm.getFile(), output.getFile(), result.detail))
        return result;
    result.ok = commitTemporary(output);
    return result;
}
MixFile exportStems(const Song& song, const AudioLibrary& library, const File& destination,
                    const String& drumsName) {
    MixFile result{destination};
    if (destination.exists() || destination.getParentDirectory().createDirectory().failed())
        return result;
    struct StagedDirectory {
        File path;
        ~StagedDirectory() { path.deleteRecursively(); }
    } staged{destination.getSiblingFile(".stems-" + Uuid().toString())};
    if (staged.path.createDirectory().failed())
        return result;
    struct Stem {
        String name, drumGroup;
        std::vector<size_t> tracks;
    };
    std::vector<Stem> stems;
    for (size_t index = 0; index < song.tracks.size(); ++index) {
        const auto& track = song.tracks[index];
        if (track.mute)
            continue;
        auto group = track.kind == "bici" ? track.group : String{};
        if (group.isNotEmpty()) {
            auto stem = std::find_if(stems.begin(), stems.end(), [&](const Stem& x) { return x.drumGroup == group; });
            if (stem != stems.end()) {
                stem->tracks.push_back(index);
                continue;
            }
        }
        stems.push_back({group.isEmpty() ? track.name : drumsName, group, {index}});
    }
    if (stems.empty())
        return result;
    auto isolated = song;
    isolated.masterDb = 0;
    const auto length = song.end() + 2 * song.grid.rate;
    for (size_t i = 0; i < stems.size(); ++i) {
        isolated.tracks.clear();
        for (auto index : stems[i].tracks) {
            isolated.tracks.push_back(song.tracks[index]);
            isolated.tracks.back().solo = false;
        }
        auto name = String(i + 1).paddedLeft('0', std::max(2, String(stems.size()).length())) + " " + File::createLegalFileName(stems[i].name).trim();
        if (!renderSong(isolated, library, staged.path.getChildFile(name.trimEnd() + ".wav"), 0,
                        length))
            return result;
    }
    result.ok = moveNewDirectory(staged.path, destination);
    return result;
}
std::optional<bool> containsClick(const File& file, const BeatGrid& grid, Frame start) {
    auto input = waveReader(file);
    if (!input || input->numChannels == 0)
        return {};
    BeatGrid clock = grid;
    clock.rate = roundToInt(input->sampleRate);
    Frame origin = std::llround(double(start) * clock.rate / grid.rate);
    int n = clock.rate / 80;
    AudioBuffer<float> samples(2, n);
    int hits = 0;
    for (Frame beat = clock.beatAt(origin); clock.beat(beat) - origin + n <= input->lengthInSamples;
         ++beat) {
        auto pos = clock.beat(beat) - origin;
        if (pos < 0)
            continue;
        if (!input->read(&samples, 0, n, pos, true, true))
            return {};
        double match = 0, energy = 0, reference = 0;
        for (int i = 0; i < n; ++i) {
            double expected =
                std::exp(-i / (clock.rate * .0025)) *
                std::cos(2 * std::numbers::pi * (beat % clock.beats == 0 ? 1600 : 1000) * i / clock.rate);
            double actual = .5 * (samples.getSample(0, i) + samples.getSample(1, i));
            match += expected * actual;
            energy += actual * actual;
            reference += expected * expected;
        }
        if (match > .035 * reference && match * match > .65 * reference * energy && ++hits >= 2)
            return true;
    }
    return false;
}
} // namespace rocker
