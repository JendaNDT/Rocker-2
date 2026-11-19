#include "storage/AtomicFile.h"
#include "Timeline.h"
#include <cmath>
#include <numbers>
namespace rocker {
using namespace juce;
static_assert(std::atomic<Frame>::is_always_lock_free && std::atomic<Performance*>::is_always_lock_free);
CaptureQueue::CaptureQueue(int capacity, int channels) : buffer(channels, capacity) { buffer.clear(); }
bool CaptureQueue::hasRoom(int frames) const noexcept {
    return frames <= buffer.getNumSamples() - (written.load(std::memory_order_relaxed) -
                                               read.load(std::memory_order_acquire));
}
bool CaptureQueue::push(const float* l, const float* r, int count) noexcept {
    auto w = written.load(std::memory_order_relaxed);
    const int cap = buffer.getNumSamples();
    if (count > cap - (w - read.load(std::memory_order_acquire)))
        return false;
    const float* inputs[]{l, r};
    for (int offset = 0; offset < count;) {
        int at = (int)((w + offset) % cap), n = std::min(count - offset, cap - at);
        for (int c = 0; c < buffer.getNumChannels(); ++c) {
            if (inputs[c])
                std::copy_n(inputs[c] + offset, n, buffer.getWritePointer(c, at));
            else
                buffer.clear(c, at, n);
        }
        offset += n;
    }
    written.store(w + count, std::memory_order_release);
    return true;
}
bool CaptureQueue::drain(AudioFormatWriter& writer, int rate) {
    Frame pos = read.load(std::memory_order_relaxed), end = written.load(std::memory_order_acquire);
    while (pos < end) {
        int at = (int)(pos % buffer.getNumSamples());
        int n = (int)std::min<Frame>({end - pos, buffer.getNumSamples() - at, rate - (pos - flushed)});
        if (!writer.writeFromAudioSampleBuffer(buffer, at, n))
            return false;
        pos += n;
        if (pos - flushed >= rate) {
            if (!writer.flush())
                return false;
            flushed = pos;
        }
        read.store(pos, std::memory_order_release);
    }
    return true;
}
bool AudioLibrary::load(const Song& s, const File& dir) {
    bool ok = true;
    for (auto& a : s.assets) {
        if (data.contains(a.id))
            continue;
        auto wave = std::make_shared<MappedWave>();
        wave->reader.reset(WavAudioFormat().createMemoryMappedReader(dir.getChildFile(a.path)));
        auto* reader = wave->reader.get();
        if (!reader || reader->sampleRate != s.grid.rate || reader->lengthInSamples < a.length ||
            reader->numChannels != (unsigned)a.channels || !reader->usesFloatingPointData ||
            reader->bitsPerSample != 32 || !reader->mapEntireFile()) {
            ok = false;
            continue;
        }
        wave->peaks.reserve((size_t)((a.length + MappedWave::bin - 1) / MappedWave::bin));
        for (Frame pos = 0; pos < a.length; pos += MappedWave::bin) {
            Range<float> peaks[2];
            reader->readMaxLevels(pos, std::min<Frame>(MappedWave::bin, a.length - pos), peaks,
                                  a.channels);
            float peak = 0;
            for (int ch = 0; ch < a.channels; ++ch)
                peak = std::max({peak, std::abs(peaks[ch].getStart()), std::abs(peaks[ch].getEnd())});
            wave->peaks.push_back(peak);
        }
        data[a.id] = wave;
    }
    return ok;
}
void Playback::updateMix(const Song& s) {
    const bool solo = std::any_of(s.tracks.begin(), s.tracks.end(), [](auto& t) { return t.solo; });
    master.store(Decibels::decibelsToGain(s.masterDb, -96.f));
    for (size_t i = 0; i < strips.size(); ++i) {
        auto& strip = *strips[i];
        const auto& t = s.tracks[i];
        strip.audible.store(!t.mute && (!solo || t.solo));
        const auto amplitude = Decibels::decibelsToGain(t.gainDb, -96.f);
        const double p = t.pan;
        double left = strip.channels == 1 ? std::cos((p + 1) * MathConstants<double>::pi / 4)
                                          : (p > 0 ? std::cos(p * MathConstants<double>::halfPi) : 1);
        double right = strip.channels == 1 ? std::sin((p + 1) * MathConstants<double>::pi / 4)
                                           : (p < 0 ? std::cos(p * MathConstants<double>::halfPi) : 1);
        strip.gain[0].store(p == 1 ? 0.f : float(left) * amplitude);
        strip.gain[1].store(p == -1 ? 0.f : float(right) * amplitude);
    }
}
Playback::Meter Playback::meter() const noexcept {
    return {leftPeak.load(), rightPeak.load(), maxPeak.load(), loudest.load(), over.load()};
}
float Playback::trackPeak(size_t i) const noexcept {
    return i < strips.size() ? strips[i]->peak.load() : 0.f;
}
void Playback::setSound(size_t track, const GuitarSound& sound, bool recall) {
    auto& strip = *strips[track];
    strip.command.publish(sound.values, sound.enabled, recall);
}
Playback::Playback(const Song& s, const AudioLibrary& lib) {
    cutFade = std::max(1, roundToInt(.005 * s.grid.rate));
    int bar = 0;
    for (const auto& o : s.order) {
        bar += o.length;
        if (o.crop + o.length < s.sections.at(o.section).bars)
            cuts.push_back(s.barFrame(bar));
    }
    for (auto& track : s.tracks) {
        auto& strip = *strips.emplace_back(std::make_unique<Strip>());
        strip.channels = std::clamp(track.inputs.size(), 1, 2);
        strip.fade = std::max(1, roundToInt(s.grid.rate * 0.010));
        auto addClips = [&](const std::vector<Clip>& clips, std::vector<PlaybackClip>& output) {
            for (const auto& clip : clips) {
                if (track.id != clip.track)
                    continue;
                auto it = lib.data.find(clip.asset);
                if (it == lib.data.end())
                    continue;
                output.push_back({it->second, clip.position, clip.offset, clip.length});
                strip.channels = std::max(strip.channels, int(it->second->reader->numChannels));
                hasClips = true;
            }
        };
        addClips(s.clips, strip.clips);
        for (const auto& group : s.groups) {
            if (group.track != track.id)
                continue;
            auto& overlay = *strip.groups.emplace_back(std::make_unique<Overlay>());
            overlay.id = group.id;
            overlay.from = group.from;
            overlay.to = group.to;
            for (const auto& attempt : group.attempts)
                addClips(attempt.clips, overlay.variants.emplace_back());
            overlay.weights.resize(overlay.variants.size());
            overlay.active.store(group.active);
        }
        if (track.kind == "kytara") {
            strip.amps.resize(strip.channels);
            for (auto& amp : strip.amps)
                amp.prepare(s.grid.rate);
            setSound(strips.size() - 1, track.amp, true);
            if (!strip.clips.empty() || !strip.groups.empty())
                tail = 2 * s.grid.rate;
        }
    }
    updateMix(s);
}
float MappedWave::peak(Frame start, int length) const noexcept {
    float result = 0;
    for (auto i = (size_t)(start / bin); i < peaks.size() && i <= (size_t)((start + length - 1) / bin);
         ++i)
        result = std::max(result, peaks[i]);
    return result;
}
void Playback::prefetch(Frame from, Frame to) const {
    auto warm = [&](const auto& clips) {
        for (const auto& c : clips)
            for (Frame i = std::max(from, c.at); i < std::min(to, c.at + c.length); i += 512)
                c.wave->reader->touchSample(c.offset + i - c.at);
    };
    for (const auto& strip : strips) {
        warm(strip->clips);
        for (const auto& group : strip->groups)
            for (const auto& variant : group->variants)
                warm(variant);
    }
}
void Playback::selectAttempt(const String& id, int index) {
    for (auto& strip : strips)
        for (auto& group : strip->groups)
            if (group->id == id && index >= 0 && index <= (int)group->variants.size())
                group->active.store(index);
}
void Playback::setPunch(size_t index, Frame from, Frame to, int left, int right, bool clean) {
    auto& strip = *strips.at(index);
    strip.punchFrom = from;
    strip.punchTo = to;
    strip.cleanPunch = clean;
    strip.livePorts[0] = left;
    strip.livePorts[1] = right;
}
static float envelope(Frame position, Frame from, Frame to, int fade) noexcept {
    if (position < from || position >= to)
        return 0.f;
    Frame width = std::min<Frame>(fade, std::max<Frame>(1, (to - from) / 2));
    return (float)std::min({width, position - from, to - 1 - position}) / (float)width;
}
void Playback::Strip::read(const std::vector<PlaybackClip>& material, float (&block)[2][256], int count,
                           Frame at) const noexcept {
    for (int ch = 0; ch < channels; ++ch)
        std::fill_n(block[ch], count, 0.f);
    float source[2][256];
    int* raw[]{reinterpret_cast<int*>(source[0]), reinterpret_cast<int*>(source[1])};
    for (const auto& clip : material) {
        auto first = std::max(at, clip.at);
        int n =
            static_cast<int>(std::max<Frame>(0, std::min(at + count, clip.at + clip.length) - first));
        if (n == 0)
            continue;
        auto& reader = *clip.wave->reader;
        reader.readSamples(raw, int(reader.numChannels), 0, clip.offset + first - clip.at, n);
        for (int ch = 0; ch < channels; ++ch) {
            auto* dest = block[ch] + (first - at);
            auto* input = source[std::min(ch, int(reader.numChannels) - 1)];
            for (int i = 0; i < n; ++i)
                dest[i] += input[i];
        }
    }
}
void Playback::Strip::process(float* const* out, int outputs, int size, Frame at,
                              const float* const* input, int inputs, int inputOffset) noexcept {
    if (clips.empty() && groups.empty() && punchTo <= punchFrom)
        return;
    const auto snapshot = command.consume();
    const auto word = snapshot & ~AmpCommand::recallBit;
    const bool wet = (word & AmpCommand::enabledBit) != 0 && !amps.empty();
    const int delay = wet ? amps.front().delaySamples() : 0;
    const bool restart = next != at || (snapshot & AmpCommand::recallBit) ||
                         ((applied ^ word) & AmpCommand::enabledBit) != 0;
    if (wet && (restart || word != applied)) {
        for (auto& amp : amps) {
            amp.configure(AmpParameters::unpack(word & AmpCommand::valueMask));
            if (restart)
                amp.reset();
        }
    }
    applied = word;
    next = at + size;
    const bool heard = audible.load();
    float channelGain[]{gain[0].load(), gain[1].load()};
    float level = 0;
    // On start/seek feed the first delay samples before delivering output sample zero.
    for (int offset = restart ? -delay : 0; offset < size;) {
        int count = std::min(256, offset < 0 ? -offset : size - offset);
        float block[2][256];
        const Frame position = at + delay + offset;
        read(clips, block, count, position);
        for (const auto& group : groups) {
            const int index = group->active.load();
            if (position >= group->to || position + count <= group->from)
                continue;
            if (index != group->target || (restart && offset <= 0)) {
                group->remaining = group->target < 0 || restart ? 0 : fade;
                group->target = index;
            }
            float original[2][256];
            for (int ch = 0; ch < channels; ++ch)
                std::copy_n(block[ch], count, original[ch]);
            for (size_t variant = 0; variant < group->variants.size(); ++variant) {
                float wanted = int(variant) + 1 == index ? 1.f : 0.f;
                float oldWeight = group->remaining ? group->weights[variant] : wanted;
                if (oldWeight == 0 && wanted == 0) {
                    group->weights[variant] = 0;
                    continue;
                }
                float step = group->remaining ? (wanted - oldWeight) / group->remaining : 0.f;
                float take[2][256];
                read(group->variants[variant], take, count, position);
                for (int i = 0; i < count; ++i) {
                    float blend = i < group->remaining ? oldWeight + i * step : wanted;
                    float weight = blend * envelope(position + i, group->from, group->to, fade);
                    for (int ch = 0; ch < channels; ++ch)
                        block[ch][i] += weight * (take[ch][i] - original[ch][i]);
                }
                group->weights[variant] = count < group->remaining ? oldWeight + count * step : wanted;
            }
            group->remaining = std::max(0, group->remaining - count);
        }
        if (position < punchTo && position + count > punchFrom && punchTo > punchFrom) {
            for (int i = 0; i < count; ++i) {
                float weight = envelope(position + i, punchFrom, punchTo, fade);
                for (int ch = 0; ch < channels; ++ch) {
                    int port = livePorts[ch];
                    float live = !cleanPunch && input && port >= 0 && port < inputs && input[port] &&
                                         offset + i >= 0
                                     ? input[port][inputOffset + offset + i]
                                     : 0.f;
                    block[ch][i] += weight * (live - block[ch][i]);
                }
            }
        }
        if (wet)
            for (int ch = 0; ch < channels; ++ch)
                for (int i = 0; i < count; ++i)
                    block[ch][i] = amps[ch].process(block[ch][i]);
        if (cleanPunch && input && offset >= 0) {
            for (int ch = 0; ch < channels; ++ch) {
                int port = livePorts[ch];
                if (port >= 0 && port < inputs && input[port])
                    for (int i = 0; i < count; ++i)
                        block[ch][i] += envelope(at + offset + i, punchFrom, punchTo, fade) *
                                        input[port][inputOffset + offset + i];
            }
        }
        if (heard && offset >= 0)
            for (int ch = 0; ch < std::min(2, outputs); ++ch)
                if (out[ch]) {
                    auto* source = block[std::min(ch, channels - 1)];
                    for (int i = 0; i < count; ++i) {
                        const float sample = source[i] * channelGain[ch];
                        out[ch][offset + i] += sample;
                        level = std::max(level, std::abs(sample));
                    }
                }
        offset += count;
    }
    blockPeak = std::max(blockPeak, level);
}
void Playback::mix(float* const* out, int outputs, int size, Frame at, const float* const* input,
                   int inputs, int inputOffset) noexcept {
    const ScopedNoDenormals guard;
    for (auto& strip : strips)
        strip->blockPeak = 0;
    const float masterGain = master.load();
    float peaks[2]{}, maximum = maxPeak.load();
    Frame maximumAt = loudest.load(), clipped = over.load();
    for (int offset = 0; offset < size;) {
        Frame position = at + offset;
        auto next = std::upper_bound(cuts.begin(), cuts.end(), position);
        Frame after = next == cuts.end() ? std::numeric_limits<Frame>::max() : *next;
        Frame before = next == cuts.begin() ? -Frame(cutFade) : *std::prev(next);
        if (position == before)
            for (auto& strip : strips)
                strip->next = -1;
        int n = (int)std::min<Frame>({256, size - offset, after - position});
        float left[256]{}, right[256]{};
        float* block[]{left, right};
        for (auto& strip : strips)
            strip->process(block, 2, n, position, input, inputs, inputOffset + offset);
        for (int i = 0; i < n; ++i) {
            float gain =
                (float)std::min<Frame>({cutFade, position + i - before, after - position - i - 1}) /
                cutFade;
            for (int ch = 0; ch < 2; ++ch) {
                float value = gain * masterGain * block[ch][i];
                float level = std::abs(value);
                peaks[ch] = std::max(peaks[ch], level);
                if (level > maximum) {
                    maximum = level;
                    maximumAt = position + i;
                }
                if (level > 1.f && clipped < 0)
                    clipped = position + i;
                if (ch < outputs && out[ch])
                    out[ch][offset + i] += value;
            }
        }
        offset += n;
    }
    for (auto& strip : strips)
        strip->peak.store(strip->blockPeak);
    leftPeak.store(peaks[0]);
    rightPeak.store(peaks[1]);
    maxPeak.store(maximum);
    loudest.store(maximumAt);
    over.store(clipped);
}
bool renderSong(const Song& song, const AudioLibrary& library, const File& destination, Frame begin,
                Frame end) {
    TemporaryFile output(destination);
    auto writer = waveWriter(output.getFile(), song.grid.rate, 2);
    if (!writer)
        return false;
    Playback mix(song, library);
    AudioBuffer<float> block(2, 1024);
    if (end < 0)
        end = song.end() + 2 * song.grid.rate;
    if (begin < 0 || end <= begin)
        return false;
    for (Frame at = 0; at < end; at += block.getNumSamples()) {
        int count = (int)std::min<Frame>(block.getNumSamples(), end - at);
        block.clear();
        mix.mix(block.getArrayOfWritePointers(), 2, count, at);
        const int skip = (int)std::clamp<Frame>(begin - at, 0, count);
        if (skip < count && !writer->writeFromAudioSampleBuffer(block, skip, count - skip))
            return false;
    }
    writer.reset();
    return commitTemporary(output);
}
Performance::Performance(const Song& s, const AudioLibrary& library, Frame from, bool rec,
                         bool metronome, int count)
    : grid(s.grid), playback(s, library), start(from),
      end(rec ? std::numeric_limits<Frame>::max() : s.end() + playback.tailFrames()),
      clock(from - s.grid.beat(count)), recording(rec), click(metronome), cursor(clock) {
    // Anchor count-in to the punch position; normal beats follow the song grid.
    beatIndex = count ? -count : grid.beatAt(from);
    nextBeat = count ? clock : grid.beat(beatIndex);
    if (nextBeat < clock)
        nextBeat = grid.beat(++beatIndex);
    if (!playback.empty()) {
        playback.prefetch(start, start + 2 * grid.rate);
        readAhead = std::thread([this] {
            Frame warmed = start + 2 * grid.rate;
            while (!stopping.load()) {
                auto from = cursor.load();
                if (from + grid.rate > warmed) {
                    playback.prefetch(std::max(from, warmed), from + 2 * grid.rate);
                    warmed = from + 2 * grid.rate;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
}
Performance::~Performance() { finishDisk(); }
bool Performance::arm(const Song& s, const File& dir, const StringArray& ports, int delay,
                      bool useWorker) {
    if (dir.getChildFile("Zvuk").createDirectory().failed())
        return false;
    Array<var> entries;
    for (auto& t : s.tracks)
        if (t.armed) {
            Take take;
            take.track = t.id;
            take.left = t.inputs.isEmpty() ? -1 : ports.indexOf(t.inputs[0]);
            take.right = t.inputs.size() == 2 ? ports.indexOf(t.inputs[1]) : -1;
            if (take.left < 0 || (t.inputs.size() == 2 && take.right < 0))
                return false;
            if (isPunch())
                playback.setPunch(
                    size_t(&t - s.tracks.data()), punchFrom, punchTo, monitorPunch ? take.left : -1,
                    monitorPunch ? (take.right >= 0 ? take.right : take.left) : -1, cleanPunch);
            take.asset.id = Uuid().toString();
            take.asset.path = "Zvuk/" + take.asset.id + ".wav";
            take.asset.channels = t.inputs.size();
            take.writer = waveWriter(dir.getChildFile(take.asset.path), grid.rate, take.asset.channels);
            if (!take.writer)
                return false;
            take.queue = std::make_unique<CaptureQueue>(grid.rate * 10, take.asset.channels);
            entries.add(object({{"id", take.asset.id}, {"stopa", t.id}}));
            takes.push_back(std::move(take));
        }
    if (takes.empty())
        return false;
    compensation = std::max(0, delay);
    journal = dir.getChildFile("Zachrana").getChildFile(Uuid().toString() + ".json");
    int anchor = s.occurrenceAt((int)(s.grid.beatAt(isPunch() ? punchFrom : start) / s.grid.beats));
    if (!atomicJson(journal, object({{"poloha", start},
                                     {"vyskyt", anchor >= 0 ? s.order[anchor].id : String{}},
                                     {"dil", anchor >= 0 ? s.order[anchor].section : String{}},
                                     {"opravaOd", punchFrom},
                                     {"opravaDo", punchTo},
                                     {"vznik", Time::currentTimeMillis()},
                                     {"kompenzace", compensation},
                                     {"sampleRate", grid.rate},
                                     {"stopy", entries}})))
        return false;
    if (!useWorker)
        return true;
    worker = std::thread([this] {
        while (!stopping.load()) {
            if (!drain()) {
                failed = true;
                finished = true;
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        if (!drain())
            failed = true;
    });
    return true;
}
bool Performance::drain() {
    for (auto& t : takes)
        if (!t.queue->drain(*t.writer, grid.rate))
            return false;
    return true;
}
void Performance::storeInputMarks(Song& song) const {
    for (auto& t : takes)
        if (t.inputOver >= 0)
            for (auto& track : song.tracks)
                if (track.id == t.track)
                    track.inputOver = t.inputOver;
}
void Performance::finishDisk() {
    stopping = true;
    if (readAhead.joinable())
        readAhead.join();
    if (worker.joinable())
        worker.join();
    for (auto& t : takes)
        t.writer.reset();
}
void Performance::process(const float* const* in, int ni, float* const* out, int no,
                          int frames) noexcept {
    if (finished.load())
        return;
    const int n = (int)(clock + frames > end ? end - clock : frames);
    int skip = (int)std::clamp<Frame>(start - clock, 0, n);
    if (n > skip) {
        float* outputs[]{no > 0 && out[0] ? out[0] + skip : nullptr,
                         no > 1 && out[1] ? out[1] + skip : nullptr};
        playback.mix(outputs, std::min(2, no), n - skip, clock + skip, in, ni, skip);
        for (auto& t : takes)
            if (!t.queue->hasRoom(n - skip)) {
                drop = clock + skip;
                finished = true;
                return;
            }
        for (auto& t : takes) {
            auto channel = [&](int c) { return c >= 0 && c < ni && in[c] ? in[c] + skip : nullptr; };
            if (t.inputOver < 0) {
                auto left = channel(t.left), right = channel(t.right);
                for (int i = 0; i < n - skip; ++i)
                    if ((left && std::abs(left[i]) >= 0.9885531f) ||
                        (right && std::abs(right[i]) >= 0.9885531f)) {
                        t.inputOver = std::max<Frame>(start, clock + skip + i - compensation);
                        break;
                    }
            }
            if (!t.queue->push(channel(t.left), channel(t.right), n - skip)) {
                drop = clock + skip;
                finished = true;
                break;
            }
        }
    }
    for (int i = 0; i < n; ++i) {
        Frame now = clock + i;
        if (now == nextBeat) {
            if (syncPulse && now < Frame(grid.rate) * 600 && no > 0 && out[0])
                out[0][i] += 0.125f;
            accent = beatIndex % grid.beats == 0;
            clickAge = (click || now < start) ? 0 : -1;
            ++beatIndex;
            if (now < start && beatIndex <= 0) {
                nextBeat = start + grid.beat(beatIndex);
                if (beatIndex == 0) {
                    beatIndex = grid.beatAt(start);
                    nextBeat = grid.beat(beatIndex);
                    if (nextBeat < start)
                        nextBeat = grid.beat(++beatIndex);
                }
            } else
                nextBeat = grid.beat(beatIndex);
        }
        if (clickAge >= 0) {
            float pulse =
                (float)(0.16 * std::exp(-clickAge / (grid.rate * .0025)) *
                        std::cos(2 * std::numbers::pi * (accent ? 1600 : 1000) * clickAge / grid.rate));
            for (int c = 0; c < std::min(2, no); ++c)
                if (out[c])
                    out[c][i] += pulse;
            if (++clickAge >= grid.rate / 80)
                clickAge = -1;
        }
    }
    clock += n;
    cursor.store(clock);
    if (clock >= end)
        finished = true;
}
void Timeline::detach() noexcept {
    active.store(nullptr);
    while (reading.load())
        std::this_thread::yield();
}
void Timeline::process(const float* const* in, int ni, float* const* out, int no, int frames) noexcept {
    reading.store(true);
    if (auto* p = active.load())
        p->process(in, ni, out, no, frames);
    reading.store(false);
}
std::optional<std::pair<int, int>> syncOffsets(AudioFormatReader& reader, const BeatGrid& grid,
                                               int delay) {
    const int radius = grid.rate / 10;
    AudioBuffer<float> block(1, 2 * radius + 1);
    std::pair<int, int> result{};
    for (Frame i = 1; grid.beat(i) < Frame(grid.rate) * 600; ++i) {
        Frame expected = grid.beat(i) + delay;
        if (!reader.read(&block, 0, block.getNumSamples(), expected - radius, true, false))
            return {};
        float peak = 0;
        int index = 0;
        for (int j = 0; j < block.getNumSamples(); ++j) {
            float amplitude = std::abs(block.getSample(0, j));
            if (amplitude > peak) {
                peak = amplitude;
                index = j;
            }
        }
        if (peak < 0.001f)
            return {};
        if (i == 1)
            result.first = index - radius;
        result.second = index - radius;
    }
    return result;
}
bool recoverTake(Song& song, const File& dir, const File& journal) {
    auto record = JSON::parse(journal);
    auto entries = record.getProperty("stopy", {});
    if (!entries.isArray() || !record.hasProperty("poloha") ||
        (int)record.getProperty("sampleRate", 0) != song.grid.rate)
        return false;
    const Frame at = record.getProperty("poloha", -1), delay = record.getProperty("kompenzace", -1);
    if (at < 0 || delay < 0)
        return false;
    auto occurrence = record.getProperty("vyskyt", {}).toString();
    if (occurrence.isNotEmpty()) {
        Frame repairEnd = record.getProperty("opravaDo", 0),
              repairStart = record.getProperty("opravaOd", 0);
        int index = song.occurrenceAt(
            (int)(song.grid.beatAt(repairEnd > repairStart ? repairStart : at) / song.grid.beats));
        if (index < 0 || song.order[index].id != occurrence ||
            song.order[index].section != record.getProperty("dil", {}).toString())
            return false;
    }
    auto next = song;
    for (auto& e : *entries.getArray()) {
        auto id = e.getProperty("id", {}).toString(), track = e.getProperty("stopa", {}).toString();
        if (id.length() != 32 || !id.containsOnly("0123456789abcdefABCDEF"))
            return false;
        if (std::none_of(next.tracks.begin(), next.tracks.end(), [&](auto& t) { return t.id == track; }))
            return false;
        if (std::any_of(next.assets.begin(), next.assets.end(), [&](auto& a) { return a.id == id; }))
            continue;
        auto path = "Zvuk/" + id + ".wav";
        auto reader = waveReader(dir.getChildFile(path));
        if (!reader || reader->sampleRate != song.grid.rate)
            return false;
        auto length = reader->lengthInSamples;
        if (length <= delay)
            continue;
        next.assets.push_back({id, path, length, (int)reader->numChannels});
        Clip pass{id, track, id, at, delay, length - delay};
        Frame from = record.getProperty("opravaOd", 0), to = record.getProperty("opravaDo", 0);
        if (to <= from) {
            next.clips.push_back(std::move(pass));
            next.commitClips();
        } else {
            // An interrupted pass remains recoverable without replacing an incomplete bar.
            bool partial = pass.position + pass.length < to;
            Frame first = partial ? pass.position : from;
            Frame last = partial ? pass.position + pass.length : to;
            if (!next.addAttempt(pass, first, last, record.getProperty("vznik", 0), partial))
                return false;
        }
    }
    next.bars = std::max(
        next.bars, (int)(next.grid.beatAt(std::max<Frame>(0, next.end() - 1)) / next.grid.beats + 1));
    if (!next.save(dir))
        return false;
    song = std::move(next);
    return journal.deleteFile();
}
} // namespace rocker
