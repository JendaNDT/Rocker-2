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
    for (size_t i = 0; i < s.tracks.size(); ++i)
        audible[i].store(!s.tracks[i].mute && (!solo || s.tracks[i].solo));
}
Playback::Playback(const Song& s, const AudioLibrary& lib)
    : audible(std::make_unique<std::atomic<bool>[]>(s.tracks.size())) {
    updateMix(s);
    for (auto& c : s.clips) {
        auto t = std::find_if(s.tracks.begin(), s.tracks.end(),
                              [&](auto& track) { return track.id == c.track; });
        auto it = lib.data.find(c.asset);
        if (t != s.tracks.end() && it != lib.data.end())
            clips.push_back(
                {it->second, c.position, c.offset, c.length, (size_t)(t - s.tracks.begin())});
    }
}
float MappedWave::peak(Frame start, int length) const noexcept {
    float result = 0;
    for (auto i = (size_t)(start / bin); i < peaks.size() && i <= (size_t)((start + length - 1) / bin);
         ++i)
        result = std::max(result, peaks[i]);
    return result;
}
void Playback::prefetch(Frame from, Frame to) const {
    for (auto& c : clips) {
        for (Frame i = std::max(from, c.at); i < std::min(to, c.at + c.length); i += 512)
            c.wave->reader->touchSample(c.offset + i - c.at);
    }
}
void Playback::mix(float* const* out, int no, int size, Frame at) const noexcept {
    alignas(float) float block[2][256];
    int* raw[]{reinterpret_cast<int*>(block[0]), reinterpret_cast<int*>(block[1])};
    for (const auto& c : clips) {
        if (!audible[c.trackIndex].load(std::memory_order_relaxed))
            continue;
        const Frame to = std::min(at + size, c.at + c.length);
        auto& reader = *c.wave->reader;
        for (Frame pos = std::max(at, c.at); pos < to;) {
            int count = (int)std::min<Frame>(256, to - pos);
            reader.readSamples(raw, (int)reader.numChannels, 0, c.offset + pos - c.at, count);
            for (int ch = 0; ch < std::min(2, no); ++ch)
                if (out[ch]) {
                    float* destination = out[ch] + (pos - at);
                    const float* source = block[std::min(ch, (int)reader.numChannels - 1)];
                    for (int i = 0; i < count; ++i)
                        destination[i] += source[i];
                }
            pos += count;
        }
    }
}
bool renderSong(const Song& song, const AudioLibrary& library, const File& destination) {
    TemporaryFile output(destination);
    auto writer = waveWriter(output.getFile(), song.grid.rate, 2);
    if (!writer)
        return false;
    Playback mix(song, library);
    AudioBuffer<float> block(2, 1024);
    for (Frame at = 0; at < song.end(); at += block.getNumSamples()) {
        int count = (int)std::min<Frame>(block.getNumSamples(), song.end() - at);
        block.clear();
        mix.mix(block.getArrayOfWritePointers(), 2, count, at);
        if (!writer->writeFromAudioSampleBuffer(block, 0, count))
            return false;
    }
    writer.reset();
    return output.overwriteTargetFileWithTemporary();
}
Performance::Performance(const Song& s, const AudioLibrary& library, Frame from, bool rec,
                         bool metronome, int count)
    : grid(s.grid), playback(s, library), start(from),
      end(rec ? std::numeric_limits<Frame>::max() : s.end()), clock(from - s.grid.beat(count)),
      recording(rec), click(metronome), cursor(clock) {
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
    if (!atomicJson(journal, object({{"poloha", start},
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
        playback.mix(outputs, std::min(2, no), n - skip, clock + skip);
        for (auto& t : takes)
            if (!t.queue->hasRoom(n - skip)) {
                drop = clock + skip;
                finished = true;
                return;
            }
        for (auto& t : takes) {
            auto channel = [&](int c) { return c >= 0 && c < ni && in[c] ? in[c] + skip : nullptr; };
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
        next.clips.push_back({id, track, id, at, delay, length - delay});
    }
    next.bars = std::max(
        next.bars, (int)(next.grid.beatAt(std::max<Frame>(0, next.end() - 1)) / next.grid.beats + 1));
    if (!next.save(dir))
        return false;
    song = std::move(next);
    return journal.deleteFile();
}
} // namespace rocker
