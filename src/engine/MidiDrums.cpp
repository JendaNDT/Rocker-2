#include "MidiDrums.h"
#include "storage/AtomicFile.h"
#include <numbers>

namespace rocker {
using namespace juce;
bool HitQueue::push(Hit h) noexcept {
    const auto w = write.load(std::memory_order_relaxed);
    if (w - read.load(std::memory_order_acquire) >= capacity)
        return false;
    data[w % capacity] = h;
    write.store(w + 1, std::memory_order_release);
    return true;
}
bool HitQueue::peek(Hit& h) const noexcept {
    auto r = read.load(std::memory_order_relaxed);
    if (r == write.load(std::memory_order_acquire))
        return false;
    h = data[r % capacity];
    return true;
}
void HitQueue::pop() noexcept {
    read.fetch_add(1, std::memory_order_release);
}
namespace {
int instrument(int note) {
    switch (note) {
    case 35:
    case 36:
        return 0;
    case 37:
    case 38:
    case 39:
    case 40:
        return 1;
    case 42:
    case 44:
        return 2;
    case 46:
        return 3;
    case 41:
    case 43:
    case 45:
        return 4;
    case 47:
    case 48:
    case 50:
        return 5;
    case 49:
    case 52:
    case 55:
    case 57:
        return 6;
    case 51:
    case 53:
    case 59:
        return 7;
    default:
        return -1;
    }
}
} // namespace
void DrumPlayer::prepare(int rate) {
    reset();
    // Original synthesized one-shots. Both banks are generated before the callback starts.
    constexpr double seconds[]{.42, .28, .09, .46, .43, .35, .84, .72};
    constexpr double freq[]{54, 180, 7500, 6100, 108, 156, 3500, 2400};
    constexpr double noiseMix[]{.018, .72, .96, .93, .05, .05, .83, .61};
    for (int k = 0; k < 2; ++k)
        for (int pad = 0; pad < 8; ++pad) {
            auto& wave = samples[k][pad];
            const double length = seconds[pad] * (k ? .73 : 1.);
            wave.resize((size_t)std::ceil(rate * length));
            uint32_t random = 1234567u + 89 * pad;
            double phase = 0, previous = 0;
            for (size_t i = 0; i < wave.size(); ++i) {
                const double t = double(i) / rate;
                random = random * 1664525u + 1013904223u;
                const double noise = double(random) / 2147483648. - 1;
                const double high = (noise - previous) * .55;
                previous = noise;
                phase += 2 * std::numbers::pi * freq[pad] * (k ? 1.24 : 1.) *
                         (1. + std::exp(-t * 65.) * .85) / rate;
                wave[i] =
                    float(.78 * std::min(1., (i + 1.) / (rate * .0007)) * std::exp(-7 * t / length) *
                          ((1. - noiseMix[pad]) * std::sin(phase) + noiseMix[pad] * high));
            }
        }
}
void DrumPlayer::reset() noexcept {
    voices = {};
    next = 0;
}
void DrumPlayer::trigger(int kit, int note, int velocity) noexcept {
    int pad = instrument(note);
    if (pad < 0 || velocity <= 0)
        return;
    if (pad == 2) { // Closing the hi-hat chokes the open sample.
        for (auto& v : voices)
            if (v.remaining > 0) {
                for (auto& bank : samples) {
                    auto& open = bank[3];
                    if (v.wave + v.remaining == open.data() + open.size())
                        v.remaining = 0;
                }
            }
    }
    auto& wave = samples[std::clamp(kit, 0, 1)][pad];
    voices[next++ % voices.size()] = {wave.data(), (int)wave.size(), std::min(velocity, 127) / 127.f};
}
float DrumPlayer::sample() noexcept {
    float value = 0;
    for (auto& v : voices)
        if (v.remaining > 0) {
            value += *v.wave++ * v.gain;
            --v.remaining;
        }
    return value;
}
void DrumInput::prepare(int hz) {
    rate = hz;
    position = 0;
    player.prepare(rate);
    Hit h;
    while (queue.peek(h))
        queue.pop();
    for (int i = 0; i < 128; ++i)
        mapping[i] = i;
    overflow = false;
    lastNote = -1;
}
bool DrumInput::push(Hit h) noexcept {
    if (h.note < 0 || h.note > 127 || h.velocity <= 0 || h.velocity > 127)
        return false;
    lastNote = h.note;
    h.note = mapping[h.note].load();
    if (queue.push(h))
        return true;
    overflow = true;
    return false;
}
bool DrumInput::native(double sec, int note, int velocity) noexcept {
    // Negative values carry the host timestamp in microseconds until the audio clock consumes them.
    return push({-1 - std::llround(sec * 1000000.), note, velocity});
}
std::span<const Hit> DrumInput::block(int frames, double sec) noexcept {
    size_t count = 0;
    Hit h;
    while (queue.peek(h)) {
        if (h.frame < 0)
            h.frame = position + std::llround((double(-h.frame - 1) / 1000000. - sec) * rate);
        if (h.frame >= position + frames)
            break;
        queue.pop();
        if (count < hits.size())
            hits[count++] = h;
        else
            overflow = true;
    }
    return {hits.data(), count};
}
void DrumInput::render(float* const* out, int outputs, int frames, std::span<const Hit> events) noexcept {
    bool hear = monitor.load();
    int preset = kit.load();
    if (hear) {
        size_t event = 0;
        for (int i = 0; i < frames; ++i) {
            while (event < events.size() && events[event].frame <= position + i) {
                const auto& h = events[event++];
                player.trigger(preset, h.note, h.velocity);
            }
            float v = player.sample();
            for (int c = 0; c < std::min(outputs, 2); ++c)
                if (out[c])
                    out[c][i] += v;
        }
    } else
        player.reset();
    position += frames;
}
bool saveMidi(const File& file, int rate, Frame length, std::span<const Hit> hits) {
    if (rate < 1 || length < 0 || file.getParentDirectory().createDirectory().failed())
        return false;
    MidiFile midi;
    // A fixed timebase preserves individual sample positions at all supported sample rates.
    int micros = 500000;
    while (std::llround(double(rate) * micros / 1000000) > 32767)
        micros /= 2;
    const int ppq = std::max(1, int(std::llround(double(rate) * micros / 1000000)));
    midi.setTicksPerQuarterNote(ppq);
    MidiMessageSequence notes;
    notes.addEvent(MidiMessage::tempoMetaEvent(micros));
    auto ticks = [rate, ppq, micros](Frame n) {
        return double(std::llround(n * (1000000. * ppq / micros) / rate));
    };
    for (auto& h : hits)
        if (h.frame >= 0 && h.frame < length && h.note >= 0 && h.note < 128 && h.velocity > 0 &&
            h.velocity <= 127) {
            notes.addEvent(MidiMessage::noteOn(10, h.note, (uint8)h.velocity), ticks(h.frame));
            notes.addEvent(MidiMessage::noteOff(10, h.note),
                           ticks(std::min(length, h.frame + rate / 100)));
        }
    notes.addEvent(MidiMessage::endOfTrack(), ticks(length));
    midi.addTrack(notes);
    TemporaryFile tmp(file);
    {
        FileOutputStream stream(tmp.getFile());
        if (!stream.openedOk() || !midi.writeTo(stream, 0))
            return false;
        stream.flush();
        if (stream.getStatus().failed())
            return false;
    }
    return commitTemporary(tmp);
}
std::optional<std::vector<Hit>> readMidi(const File& file, int rate) {
    MidiFile midi;
    FileInputStream in(file);
    if (!in.openedOk() || !midi.readFrom(in))
        return {};
    midi.convertTimestampTicksToSeconds();
    std::vector<Hit> hits;
    for (int t = 0; t < midi.getNumTracks(); ++t)
        for (auto* e : *midi.getTrack(t)) {
            auto& m = e->message;
            if (m.isNoteOn())
                hits.push_back(
                    {std::llround(m.getTimeStamp() * rate), m.getNoteNumber(), m.getVelocity()});
        }
    std::stable_sort(hits.begin(), hits.end(), [](auto& a, auto& b) { return a.frame < b.frame; });
    return hits;
}
std::optional<int> compareSnare(std::span<const Hit> hits, AudioFormatReader& audio, Frame midiOffset,
                                Frame audioOffset) {
    const int radius = roundToInt(audio.sampleRate * .10);
    AudioBuffer<float> block((int)std::min(2u, audio.numChannels), 2 * radius + 1);
    std::vector<int> shifts;
    for (auto& h : hits) {
        if (h.note != 38 && h.note != 40)
            continue;
        Frame centre = h.frame - midiOffset + audioOffset, from = std::max<Frame>(0, centre - radius);
        int count = int(std::min<Frame>(block.getNumSamples(), audio.lengthInSamples - from));
        if (count < 1 || !audio.read(&block, 0, count, from, true, true))
            continue;
        float max = block.getMagnitude(0, count);
        if (max < .02f)
            continue;
        int onset = 0;
        while (onset < count && block.getMagnitude(onset, 1) < max * .15f)
            ++onset;
        if (onset < count)
            shifts.push_back(int(centre - from - onset));
        if (shifts.size() == 16)
            break;
    }
    if (shifts.size() != 16)
        return {};
    int sum = 0;
    for (auto offset : shifts)
        sum += offset;
    return roundToInt(sum / 16.);
}
bool renderMidi(const File& midi, const File& output, int kit, int rate, Frame length) {
    auto hits = readMidi(midi, rate);
    if (!hits || output.getParentDirectory().createDirectory().failed())
        return false;
    TemporaryFile temp(output);
    auto writer = waveWriter(temp.getFile(), rate, 1);
    if (!writer)
        return false;
    DrumPlayer player;
    player.prepare(rate);
    size_t hit = 0;
    AudioBuffer<float> block(1, 4096);
    for (Frame at = 0; at < length;) {
        int n = int(std::min<Frame>(block.getNumSamples(), length - at));
        for (int i = 0; i < n; ++i) {
            while (hit < hits->size() && (*hits)[hit].frame <= at + i) {
                auto& h = (*hits)[hit++];
                player.trigger(kit, h.note, h.velocity);
            }
            block.setSample(0, i, player.sample());
        }
        if (!writer->writeFromAudioSampleBuffer(block, 0, n))
            return false;
        at += n;
    }
    if (!writer->flush())
        return false;
    writer.reset();
    return commitTemporary(temp);
}
} // namespace rocker
