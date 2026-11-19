#include "engine/Export.h"
#include "FakeDevice.h"
#include "song/Song.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>
namespace {
using namespace rocker;
using namespace juce;
struct Workspace {
    File dir =
        File::getSpecialLocation(File::tempDirectory).getChildFile("rocker-song-" + Uuid().toString());
    Workspace() { REQUIRE(dir.createDirectory().wasOk()); }
    ~Workspace() { dir.deleteRecursively(); }
};
Song example() {
    Song s;
    s.title = String(u8"Noční čtvrť");
    s.tracks = {{"dr", "Drums", "bici", {"In 1"}, false},
                {"gt", String(u8"Řezavá kytara"), "kytara", {"In 2"}, true}};
    s.tracks.front().pan = -1; // Unity on the measured left channel for timing and edge tests.
    return s;
}
TEST_CASE("A-03 song, version fallback, invalid clip and view round trip") {
    Workspace w;
    auto s = example();
    s.assets = {{"a", "Zvuk/test.wav", 48000, 1}};
    s.clips = {{"c", "dr", "a", 9231, 384, 24000}};
    CHECK(s.save(w.dir));
    CHECK(s.archive(w.dir));
    bool bad = false;
    auto read = Song::load(w.dir, bad);
    REQUIRE(read);
    CHECK_FALSE(bad);
    CHECK(JSON::toString(read->json()) == JSON::toString(s.json()));
    ViewState v{240012, 1, 16, true, false, true};
    CHECK(v.save(w.dir));
    auto view = ViewState::load(w.dir);
    CHECK(view.cursor == v.cursor);
    CHECK(view.selected == 1);
    CHECK(view.zoom == 16);
    CHECK(view.distant);
    CHECK_FALSE(view.click);
    auto old = object({{"schemaVersion", 0}, {"nazev", s.title}, {"stopy", Array<var>{}}});
    auto migrated = Song::parse(old, bad);
    REQUIRE(migrated);
    CHECK((int)migrated->json().getProperty("schemaVersion", 0) == 1);
    s.clips.front().offset = 47000;
    REQUIRE(Song::parse(s.json(), bad));
    CHECK(bad);
    REQUIRE(w.dir.getChildFile("skladba.json").replaceWithText("{broken"));
    CHECK_FALSE(Song::load(w.dir, bad));
    auto backup = Song::latestVersion(w.dir);
    REQUIRE(backup.existsAsFile());
    REQUIRE(Song::parse(JSON::parse(backup), bad)->title == s.title);
}
TEST_CASE("A-26 versions preserve changes and skip unchanged states") {
    Workspace w;
    auto s = example();
    auto versions = w.dir.getChildFile("Verze");
    auto count = [&] { return versions.findChildFiles(File::findFiles, false, "*.json").size(); };
    REQUIRE(s.archive(w.dir));
    REQUIRE(s.archive(w.dir));
    CHECK(count() == 1);
    for (int bpm = 100; bpm < 160; ++bpm) {
        s.grid.bpm = bpm;
        REQUIRE(s.archive(w.dir));
        REQUIRE(s.archive(w.dir));
    }
    CHECK(count() == 50);
    auto files = versions.findChildFiles(File::findFiles, false, "*.json");
    files.sort();
    bool bad = false;
    auto first = Song::parse(JSON::parse(files.getFirst()), bad);
    auto last = Song::parse(JSON::parse(Song::latestVersion(w.dir)), bad);
    REQUIRE(first);
    REQUIRE(last);
    CHECK(first->grid.bpm == 110);
    CHECK(last->grid.bpm == 159);
}
TEST_CASE("A-29 exported stems sum to a unity mix, ignore solo and group the drums") {
    Workspace w;
    Song song;
    song.title = "Stems";
    song.bars = 1;
    song.masterDb = -18;
    song.tracks = {{"k", "Kick", "bici"}, {"s", "Snare", "bici"},
                   {"g", "Guitar", "kytara"}, {"m", "Muted", "jine"}};
    song.tracks[0].group = song.tracks[1].group = "acoustic";
    song.tracks[0].pan = -1;
    song.tracks[1].pan = .6f;
    song.tracks[2].gainDb = -9;
    song.tracks[2].solo = true;
    song.tracks[3].mute = true;
    auto raw = w.dir.getChildFile("Zvuk/signal.wav");
    REQUIRE(raw.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(raw, song.grid.rate, 1);
    REQUIRE(writer);
    const int length = 4096;
    AudioBuffer<float> samples(1, length);
    for (int i = 0; i < length; ++i)
        samples.setSample(0, i, float(.27 * std::sin(i * .13)));
    REQUIRE(writer->writeFromAudioSampleBuffer(samples, 0, length));
    writer.reset();
    song.assets = {{"audio", "Zvuk/signal.wav", length, 1}};
    for (size_t i = 0; i < song.tracks.size(); ++i)
        song.clips.push_back({String(i), song.tracks[i].id, "audio", Frame(i * 1300), 0, length});
    AudioLibrary media;
    REQUIRE(media.load(song, w.dir));
    const auto target = w.dir.getChildFile("Exporty/Stopy");
    CHECK(exportStems(song, media, target).ok);
    auto stems = target.findChildFiles(File::findFiles, false, "*.wav");
    REQUIRE(stems.size() == 2);
    CHECK_FALSE(exportStems(song, media, target).ok);
    auto unity = song;
    unity.tracks[2].solo = false;
    unity.masterDb = 0;
    auto reference = w.dir.getChildFile("reference.wav");
    REQUIRE(renderSong(unity, media, reference));
    auto full = waveReader(reference);
    auto drums = waveReader(stems[0]);
    auto guitar = waveReader(stems[1]);
    REQUIRE(full);
    REQUIRE(drums);
    REQUIRE(guitar);
    REQUIRE(full->lengthInSamples == drums->lengthInSamples);
    REQUIRE(full->lengthInSamples == guitar->lengthInSamples);
    CHECK(full->lengthInSamples == song.end() + 2 * song.grid.rate);
    CHECK(drums->sampleRate == song.grid.rate);
    CHECK(drums->numChannels == 2);
    constexpr int block = 1024;
    AudioBuffer<float> a(2, block), b(2, block), c(2, block);
    float error = 0;
    for (Frame pos = 0; pos < full->lengthInSamples; pos += block) {
        int n = int(std::min<Frame>(block, full->lengthInSamples - pos));
        REQUIRE(full->read(&a, 0, n, pos, true, true));
        REQUIRE(drums->read(&b, 0, n, pos, true, true));
        REQUIRE(guitar->read(&c, 0, n, pos, true, true));
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                error = std::max(error, std::abs(a.getSample(ch, i) - b.getSample(ch, i) - c.getSample(ch, i)));
    }
    CHECK(error < 1e-6f);
}
void voiceBlock(Playback& playback, float* const* out, int count, Frame at)
#if defined(ROCKER_RT_AUDIT)
    [[clang::nonblocking]]
#endif
{
    playback.mix(out, 1, count, at);
}
TEST_CASE("A-30 voice presets preserve raw audio and render independently of device blocks", "[realtime]") {
    Workspace w;
    Song song;
    song.title = "Voice";
    song.bars = 1;
    song.tracks = {{"v", "Vocal", "jine"}};
    song.tracks[0].pan = -1;
    const Frame length = 16384;
    auto file = w.dir.getChildFile("Zvuk/dry.wav");
    REQUIRE(file.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(file, song.grid.rate, 1);
    REQUIRE(writer);
    AudioBuffer<float> input(1, int(length));
    for (int i = 0; i < length; ++i)
        input.setSample(0, i, float(.58 * std::sin(i * .04) + .1 * std::sin(i * .5)));
    REQUIRE(writer->writeFromAudioSampleBuffer(input, 0, int(length)));
    writer.reset();
    MemoryBlock before;
    REQUIRE(file.loadFileAsData(before));
    song.assets = {{"wav", "Zvuk/dry.wav", length, 1}};
    song.clips = {{"c", "v", "wav", 0, 0, length}};
    AudioLibrary library;
    REQUIRE(library.load(song, w.dir));
    auto preview = [&](const Song& selected, int block, bool switchPresets) {
        Playback playback(selected, library);
        std::vector<float> result(size_t(length), 0.f);
        for (int at = 0; at < length; at += block) {
            if (switchPresets && at == 4096)
                playback.setVoice(0, {voicePresets[2], 2}, true);
            if (switchPresets && at == 8192)
                playback.setVoice(0, {voicePresets[0], 0}, true);
            float* out[]{result.data() + at};
            voiceBlock(playback, out, block, at);
        }
        return result;
    };
    auto dry = preview(song, 64, false);
    song.tracks[0].voice = {voicePresets[1], 1};
    auto reference = preview(song, 64, true);
    for (int block : {128, 512, 1024})
        CHECK(reference == preview(song, block, true));
    CHECK_FALSE(std::equal(reference.begin(), reference.begin() + 4096, dry.begin()));
    CHECK_FALSE(std::equal(reference.begin() + 4096, reference.begin() + 8192, dry.begin() + 4096));
    CHECK(std::equal(reference.begin() + 8192, reference.end(), dry.begin() + 8192));
    REQUIRE(song.save(w.dir));
    bool damaged = false;
    auto restored = Song::load(w.dir, damaged);
    REQUIRE(restored);
    CHECK_FALSE(damaged);
    CHECK(restored->tracks[0].voice == song.tracks[0].voice);
    auto legacy = song.json();
    legacy.getProperty("stopy", {})[0].getDynamicObject()->removeProperty("zvuk");
    auto old = Song::parse(legacy, damaged);
    REQUIRE(old);
    CHECK(old->tracks[0].voice == VoiceSound{});
    MemoryBlock after;
    REQUIRE(file.loadFileAsData(after));
    CHECK(after == before);
}
TEST_CASE("A-31 guide rhythm follows exact grid samples for ten minutes and stays out of WAV") {
    Workspace w;
    Song song;
    song.title = "Guide";
    song.grid = {48000, 4, 4, 113.};
    song.bars = 1000;
    const Frame duration = Frame(song.grid.rate) * 600;
    constexpr int block = 1024;
    Performance rhythm(song, {}, 0, false, true, 0, true);
    DrumPlayer expected;
    expected.prepare(song.grid.rate);
    Frame index = 0;
    auto time = [&] { return std::llround(index * 30. * song.grid.rate / song.grid.bpm); };
    std::array<float, block> left{};
    int events = 0;
    Frame next = time();
    for (Frame at = 0; at < duration; at += block) {
        left.fill(0);
        float* out[]{left.data()};
        int count = int(std::min<Frame>(block, duration - at));
        rhythm.process(nullptr, 0, out, 1, count);
        for (int i = 0; i < count; ++i) {
            if (at + i == next) {
                if (index % 4 == 0)
                    expected.trigger(0, 36, 92);
                if (index % 4 == 2)
                    expected.trigger(0, 38, 84);
                expected.trigger(0, 42, 57);
                ++index;
                ++events;
                next = time();
            }
            float reference = .24f * expected.sample();
            if (left[i] != reference) {
                CAPTURE(at, i);
                REQUIRE(left[i] == reference);
            }
        }
    }
    CHECK(events > 2200);
    song.bars = 1;
    auto dry = w.dir.getChildFile("dry.wav");
    REQUIRE(renderSong(song, {}, dry, 0, 16384));
    for (auto guide : {false, true}) {
        Performance count(song, {}, 0, false, true, 0, guide);
        left.fill(0);
        float* out[]{left.data()};
        count.process(nullptr, 0, out, 1, int(left.size()));
        CHECK(std::any_of(left.begin(), left.end(), [](float sample) { return sample != 0; }));
        auto mixed = w.dir.getChildFile(guide ? "rhythm.wav" : "click.wav");
        REQUIRE(renderSong(song, {}, mixed, 0, 16384));
        CHECK(dry.hasIdenticalContentTo(mixed));
    }
}
TEST_CASE("A-31 guide signals leave the captured input waveform identical") {
    for (int mode = 0; mode < 3; ++mode) {
        Workspace w;
        Song song;
        song.title = "Guide recording";
        song.bars = 1;
        song.tracks = {{"g", "Guitar", "kytara", {"in"}, true}};
        Performance take(song, {}, 0, true, mode != 0, 0, mode == 2);
        REQUIRE(take.arm(song, w.dir, {"in"}, 0, false));
        constexpr int size = 256;
        std::array<float, size> input{}, output{};
        input.fill(.125f);
        const float* channels[]{input.data()};
        float* monitor[]{output.data()};
        bool audible = false;
        for (int i = 0; i < 120; ++i) {
            output.fill(0);
            take.process(channels, 1, monitor, 1, size);
            REQUIRE(take.drain());
            audible |= std::any_of(output.begin(), output.end(), [](float v) { return v != 0; });
        }
        CHECK(audible == (mode != 0));
        take.finishDisk();
        REQUIRE(recoverTake(song, w.dir, take.journal));
        REQUIRE(song.assets.size() == 1);
        auto reader = waveReader(w.dir.getChildFile(song.assets[0].path));
        REQUIRE(reader);
        AudioBuffer<float> samples(1, int(reader->lengthInSamples));
        REQUIRE(reader->read(&samples, 0, samples.getNumSamples(), 0, true, false));
        auto* dry = samples.getReadPointer(0);
        CHECK(std::all_of(dry, dry + samples.getNumSamples(), [](float v) { return v == .125f; }));
    }
}

TEST_CASE("The metronome repeats odd meters and applies its output volume and tone") {
    for (const auto [beats, unit] : {std::pair{5, 4}, std::pair{7, 8}}) {
        Song song;
        song.grid = {48000, beats, unit, 120.};
        const Frame end = song.grid.beat(beats);
        Performance classic(song, {}, 0, false, true, 0, false, LoopSpan{0, end, 0});
        Performance chosen(song, {}, 0, false, true, 0, false, LoopSpan{0, end, 0});
        chosen.configureClick(50, 1);
        std::vector<float> a(size_t(end + song.grid.beat(2)), 0.f), b(a.size(), 0.f);
        float* const outputA[]{a.data()};
        float* const outputB[]{b.data()};
        classic.process(nullptr, 0, outputA, 1, int(a.size()));
        chosen.process(nullptr, 0, outputB, 1, int(b.size()));
        for (int i = 0; i < beats + 2; ++i) {
            const auto tick = size_t(song.grid.beat(i));
            CHECK(std::abs(a[tick] - .16f) < 1.e-6f);
            CHECK(std::abs(b[tick] - .08f) < 1.e-6f);
            if (tick > 0)
                CHECK(a[tick - 1] == 0.f);
        }
        CHECK(std::abs(a[20] * .5f - b[20]) > .001f);
        CHECK_FALSE(chosen.finished);
        chosen.configureClick(0, 2);
        std::array<float, 64> silent{};
        float* const muted[]{silent.data()};
        chosen.process(nullptr, 0, muted, 1, int(silent.size()));
        CHECK(std::all_of(silent.begin(), silent.end(), [](float sample) { return sample == 0.f; }));
    }
}
TEST_CASE("A-32 independently generated EBU Tech 3341 minimum cases pass within 0.1 LU") {
    Workspace w;
    constexpr int rate = 48000;
    constexpr int size = 1024;
    const std::vector<std::vector<std::pair<double, double>>> cases{
        {{20, -23}}, {{20, -33}}, {{10, -36}, {60, -23}, {10, -36}},
        {{10, -72}, {10, -36}, {60, -23}, {10, -36}, {10, -72}},
        {{20, -26}, {20.1, -20}, {20, -26}}};
    const double expected[]{-23, -33, -23, -23, -23};
    AudioBuffer<float> data(2, size);
    for (size_t test = 0; test < cases.size(); ++test) {
        CAPTURE(test);
        auto wave = w.dir.getChildFile(String(test) + ".wav");
        auto writer = waveWriter(wave, rate, 2);
        REQUIRE(writer);
        Frame start = 0;
        for (auto [seconds, level] : cases[test]) {
            auto length = std::llround(seconds * rate);
            auto gain = Decibels::decibelsToGain(level);
            for (Frame at = 0; at < length; at += size) {
                int count = int(std::min<Frame>(size, length - at));
                for (int n = 0; n < count; ++n) {
                    float sample = float(gain * std::sin(2 * std::numbers::pi * 1000 * (start + at + n) / rate));
                    data.setSample(0, n, sample);
                    data.setSample(1, n, sample);
                }
                REQUIRE(writer->writeFromAudioSampleBuffer(data, 0, count));
            }
            start += length;
        }
        writer.reset();
        auto reader = waveReader(wave);
        REQUIRE(reader);
        auto stats = measureLoudness(*reader);
        REQUIRE(stats);
        CHECK(std::abs(stats->integrated - expected[test]) <= .1);
        reader.reset();
        REQUIRE(wave.deleteFile());
    }
}
TEST_CASE("A-32 louder delivery meets the integrated and true-peak limits after export") {
    Workspace w;
    Song song;
    song.title = "Louder";
    song.bars = 1;
    song.tracks = {{"s", "Tone", "jine"}};
    constexpr int duration = 96000;
    auto path = w.dir.getChildFile("Zvuk/source.wav");
    REQUIRE(path.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(path, song.grid.rate, 2);
    REQUIRE(writer);
    AudioBuffer<float> material(2, duration);
    for (int channel = 0; channel < 2; ++channel) {
        for (int i = 0; i < duration; ++i)
            material.setSample(channel, i, float(.05 * std::sin(i * 2 * std::numbers::pi * 1000 / 48000)));
        material.setSample(channel, 48101, .97f);
    }
    REQUIRE(writer->writeFromAudioSampleBuffer(material, 0, duration));
    writer.reset();
    song.assets = {{"audio", "Zvuk/source.wav", duration, 2}};
    song.clips = {{"clip", "s", "audio", 0, 0, duration}};
    AudioLibrary sources;
    REQUIRE(sources.load(song, w.dir));
    for (auto format : {Delivery::wave24, Delivery::wave16}) {
        auto file = w.dir.getNonexistentChildFile("Louder", ".wav", true);
        REQUIRE(exportMix(song, sources, file, format, 0, duration, true).ok);
        auto reader = waveReader(file);
        REQUIRE(reader);
        CHECK(reader->lengthInSamples == Frame(reader->sampleRate) * 2);
        auto outcome = measureLoudness(*reader);
        REQUIRE(outcome);
        CHECK(std::abs(outcome->integrated + 14.) <= .5);
        CHECK(outcome->peak <= Decibels::decibelsToGain(-1.));
    }
}
TEST_CASE("A-33 three loop passes share one WAV and preserve every compensated sample") {
    Workspace w;
    Song song;
    song.title = "Loop";
    song.bars = 1;
    song.tracks = {{"t", "Loop", "jine", {"in"}, true}};
    constexpr Frame first = 1200, length = 2300, compensation = 79;
    const LoopSpan loop{first, first + length};
    REQUIRE(song.save(w.dir));
    Performance performance(song, {}, 0, true, false, 0, false, loop);
    REQUIRE(performance.arm(song, w.dir, {"in"}, compensation, false));
    constexpr int block = 379;
    std::array<float, block> input{};
    const float* in[]{input.data()};
    const Frame duration = first + 3 * length + compensation;
    for (Frame at = 0; at < duration; at += block) {
        const int n = int(std::min<Frame>(block, duration - at));
        for (int i = 0; i < n; ++i)
            input[i] = float(at + i);
        performance.process(in, 1, nullptr, 0, n);
        REQUIRE(performance.drain());
    }
    performance.finishDisk();
    REQUIRE(recoverTake(song, w.dir, performance.journal));
    REQUIRE(song.assets.size() == 1);
    REQUIRE(song.groups.size() == 1);
    auto& group = song.groups.front();
    CHECK(group.from == loop.from);
    CHECK(group.to == loop.to);
    REQUIRE(group.attempts.size() == 3);
    CHECK(group.active == 3);
    CHECK(group.cycle() == 0);
    auto reader = waveReader(w.dir.getChildFile(song.assets[0].path));
    REQUIRE(reader);
    AudioBuffer<float> written(1, int(length));
    for (int i = 0; i < 3; ++i) {
        const auto& take = group.attempts[i];
        CHECK_FALSE(take.discarded);
        REQUIRE(take.clips.size() == 1);
        const auto& clip = take.clips.front();
        CHECK(clip.position == first);
        CHECK(clip.offset == first + compensation + i * length);
        CHECK(clip.length == length);
        REQUIRE(reader->read(&written, 0, int(length), clip.offset, true, false));
        for (int n = 0; n < length; ++n)
            if (written.getSample(0, n) != float(clip.offset + n))
                FAIL("Incorrect sample after loop compensation");
    }
}
void loopPlaybackBlock(Performance& take, float* const* outputs, int size)
#if defined(ROCKER_RT_AUDIT)
    [[clang::nonblocking]]
#endif
{
    take.process(nullptr, 0, outputs, 1, size);
}
TEST_CASE("A live rhythm resumes at the next grid hit after an off-beat unmute", "[realtime][metronome]") {
    Song song;
    song.bars = 8;
    song.grid.bpm = 120;
    Performance take(song, {}, 0, false, false, 0, true);
    std::array<float, 256> samples{};
    float* out[]{samples.data()};
    const int offBeat = 4713;
    const int nextHit = int(song.grid.beat(1) / 2);
    auto advanceSilently = [&](int count) {
        while (count > 0) {
            const int n = std::min(count, int(samples.size()));
            samples.fill(0);
            loopPlaybackBlock(take, out, n);
            REQUIRE(std::all_of(samples.begin(), samples.end(), [](float v) { return v == 0.f; }));
            count -= n;
        }
    };
    advanceSilently(offBeat);
    take.setClickEnabled(true);
    advanceSilently(nextHit - offBeat);
    loopPlaybackBlock(take, out, int(samples.size()));
    CHECK(*std::max_element(samples.begin(), samples.end()) > .001f);
    take.setClickEnabled(false);
    advanceSilently(4096);
}
TEST_CASE("A-33 the monitor wraps at the exact sample inside arbitrary device blocks", "[realtime]") {
    Workspace w;
    Song song;
    song.title = "Monitor";
    song.bars = 1;
    song.tracks = {{"t", "Source", "jine"}};
    song.tracks[0].pan = -1;
    constexpr LoopSpan span{1200, 3500, 800};
    auto path = w.dir.getChildFile("Zvuk/monitor.wav");
    REQUIRE(path.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(path, song.grid.rate, 1);
    REQUIRE(writer);
    const int sourceLength = int(span.to + span.tail);
    AudioBuffer<float> recorded(1, sourceLength);
    for (int i = 0; i < sourceLength; ++i)
        recorded.setSample(0, i, (i % 64) / 128.f);
    REQUIRE(writer->writeFromAudioSampleBuffer(recorded, 0, sourceLength));
    writer.reset();
    song.assets = {{"a", "Zvuk/monitor.wav", sourceLength, 1}};
    song.clips = {{"c", "t", "a", 0, 0, sourceLength}};
    AudioLibrary media;
    REQUIRE(media.load(song, w.dir));
    Performance performance(song, media, 0, true, false, 0, false, span);
    constexpr int block = 379;
    std::array<float, block> samples{};
    const auto duration = span.from + 3 * (span.to - span.from);
    for (Frame at = 0; at < duration; at += block) {
        const auto n = int(std::min<Frame>(block, duration - at));
        samples.fill(0);
        float* out[]{samples.data()};
        loopPlaybackBlock(performance, out, n);
        for (int i = 0; i < n; ++i) {
            const auto clock = at + i;
            const auto expected = clock < span.from ? clock : span.from + (clock - span.from) % span.length();
            const auto sample = recorded.getSample(0, int(expected));
            if (samples[i] != sample) {
                CAPTURE(at, i, expected);
                REQUIRE(samples[i] == sample);
            }
        }
    }
    REQUIRE(performance.requestLoopStop());
    std::array<float, size_t(span.tail)> tail{};
    float* out[]{tail.data()};
    loopPlaybackBlock(performance, out, int(tail.size()));
    for (size_t i = 0; i < tail.size(); ++i)
        CHECK(tail[i] == recorded.getSample(0, int(span.to + i)));
    CHECK(performance.finished);
}
TEST_CASE("A-33 a stopped audio and MIDI loop retains the same intended number of passes") {
    Workspace w;
    Song song;
    song.title = "Loop MIDI";
    song.bars = 1;
    song.tracks = {{"a", "Audio", "jine", {"in"}, true}, {"m", "Module", "midi", {}, true}};
    REQUIRE(song.save(w.dir));
    constexpr int size = 480, pass = 4800;
    Performance take(song, {}, 0, true, false, 0, false, LoopSpan{0, pass});
    REQUIRE(take.arm(song, w.dir, {"in"}, 310, false, 70));
    std::array<float, size> input{};
    input.fill(.25f);
    const float* channels[]{input.data()};
    for (int at = 0; at < 3 * pass; at += size) {
        Hit hit{Frame(at + 120), 38, 100};
        take.process(channels, 1, nullptr, 0, size, std::span<const Hit>(&hit, 1), at);
        REQUIRE(take.drain());
    }
    REQUIRE(take.requestLoopStop());
    Hit hit{Frame(3 * pass + 120), 38, 100};
    take.process(channels, 1, nullptr, 0, size, std::span<const Hit>(&hit, 1), 3 * pass);
    CHECK(take.finished);
    REQUIRE(take.drain());
    take.finishDisk();
    REQUIRE(recoverTake(song, w.dir, take.journal));
    REQUIRE(song.groups.size() == 2);
    for (const auto& group : song.groups) {
        CAPTURE(group.track.toStdString());
        CHECK(group.attempts.size() == 3);
        CHECK(group.active == 3);
    }
}
TEST_CASE("A-04 ten minutes of bar downbeats land on their exact samples") {
    auto s = example();
    s.bars = 400;
    Audio device;
    device.prepare(1, s.grid.rate);
    Performance p(s, {}, 0, false, true, 0);
    device.timeline.attach(p);
    std::array<float, 509> out;
    float* channels[]{out.data()};
    std::vector<Frame> found;
    int previous = 0;
    for (Frame at = 0; at < 48000 * 600; at += out.size()) {
        device.process(nullptr, 0, channels, 1, (int)out.size());
        for (int i = 0; i < (int)out.size(); ++i) {
            if (out[i] > 0 && previous == 0)
                found.push_back(at + i);
            previous = out[i] != 0.f;
        }
    }
    device.timeline.detach();
    REQUIRE(found.size() > 1100);
    for (size_t i = 0; i < found.size(); ++i)
        REQUIRE(found[i] == std::llround(i * 48000. * 60 / 112.));
}
TEST_CASE("A-06 offline mix ignores headphones and count-in settings") {
    Workspace w;
    auto s = example();
    s.bars = 1;
    AudioLibrary lib;
    auto samples = std::make_shared<AudioBuffer<float>>(1, 4000);
    samples->clear();
    samples->setSample(0, 329, 0.75f);
    auto source = w.dir.getChildFile("Zvuk/source.wav");
    REQUIRE(source.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(source, s.grid.rate, 1);
    REQUIRE(writer);
    REQUIRE(writer->writeFromAudioSampleBuffer(*samples, 0, samples->getNumSamples()));
    writer.reset();
    s.assets.push_back({"a", "Zvuk/source.wav", samples->getNumSamples(), 1});
    samples.reset();
    REQUIRE(lib.load(s, w.dir));
    s.clips = {{"c", "dr", "a", 0, 0, 4000}};
    MemoryBlock previous;
    for (bool click : {false, true}) {
        ViewState view;
        view.click = click;
        view.countIn = click;
        REQUIRE(view.save(w.dir));
        auto file = w.dir.getChildFile(click ? "on.wav" : "off.wav");
        REQUIRE(exportMix(s, lib, file, Delivery::wave24).ok);
        auto reader = waveReader(file);
        REQUIRE(reader);
        AudioBuffer<float> rendered(2, 500);
        REQUIRE(reader->read(&rendered, 0, 500, 0, true, true));
        CHECK(rendered.getSample(0, 329) == 0.75f);
        CHECK(rendered.getSample(1, 329) == 0.f);
        MemoryBlock bytes;
        REQUIRE(file.loadFileAsData(bytes));
        if (click)
            CHECK(bytes == previous);
        previous = bytes;
    }
}
// Annotate the full call boundary in the realtime audit binary.
void overdubBlock(Audio& device, const float* const* inputs, float* const* outputs)
#if defined(ROCKER_RT_AUDIT)
    [[clang::nonblocking]]
#endif
{
    device.process(inputs, 2, outputs, 1, 128);
}
TEST_CASE("A-07 A-15 ten minute dry overdub with independent input and output delays", "[realtime]") {
    Workspace w;
    auto s = example();
    const int rate = s.grid.rate;
    const int duration = rate * 600;
    auto audio = std::make_shared<AudioBuffer<float>>(1, duration);
    audio->clear();
    std::vector<int> attacks;
    for (int beat = 0; s.grid.beat(beat) < duration; ++beat) {
        int pos = (int)s.grid.beat(beat);
        attacks.push_back(pos);
        audio->setSample(0, pos, 0.5f);
    }
    AudioLibrary lib;
    auto source = w.dir.getChildFile("Zvuk/source.wav");
    REQUIRE(source.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(source, s.grid.rate, 1);
    REQUIRE(writer);
    REQUIRE(writer->writeFromAudioSampleBuffer(*audio, 0, audio->getNumSamples()));
    writer.reset();
    s.assets.push_back({"source", "Zvuk/source.wav", audio->getNumSamples(), 1});
    audio.reset();
    REQUIRE(lib.load(s, w.dir));
    s.clips = {{"source-clip", "dr", "source", 0, 0, duration}};
    Performance p(s, lib, 0, true, false, 0);
    REQUIRE(p.arm(s, w.dir, {"In 1", "In 2"}, 211 + 173));
    Audio device;
    device.prepare(2, rate);
    device.timeline.attach(p);
    std::array<float, 173> outputDelay{};
    std::array<float, 211> inputDelay{};
    std::array<float, 128> in{}, out{};
    const float* inputs[]{nullptr, in.data()};
    float* outputs[]{out.data()};
    for (int at = 0; at < duration + 384; at += 128) {
        for (int i = 0; i < 128; ++i) {
            int time = at + i;
            in[i] = inputDelay[time % 211];
            inputDelay[time % 211] = outputDelay[time % 173];
        }
        overdubBlock(device, inputs, outputs);
        for (int i = 0; i < 128; ++i)
            outputDelay[(at + i) % 173] = out[i];
        if (at % (128 * 100) == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    device.timeline.detach();
    p.finishDisk();
    REQUIRE(p.drop < 0);
    REQUIRE_FALSE(p.failed.load());
    REQUIRE(recoverTake(s, w.dir, p.journal));
    auto clip = s.clips.back();
    REQUIRE(clip.offset == 384);
    auto reader = waveReader(w.dir.getChildFile(s.assets.back().path));
    REQUIRE(reader);
    AudioBuffer<float> buf(1, 3);
    for (auto expected : attacks) {
        REQUIRE(reader->read(&buf, 0, 3, expected + clip.offset - 1, true, false));
        REQUIRE(buf.getSample(0, 1) == 0.5f);
        REQUIRE(buf.getSample(0, 0) == 0);
        REQUIRE(buf.getSample(0, 2) == 0);
    }
}
TEST_CASE("A-14 stalled disk buffers five seconds and marks a fifteen-second overrun") {
    Workspace w;
    for (int seconds : {5, 15}) {
        auto s = example();
        s.grid.rate = 8000;
        Frame start = 8400;
        Performance p(s, {}, start, true, false, 0);
        REQUIRE(p.arm(s, w.dir, {"In 1", "In 2"}, 0, false));
        std::vector<float> block(800);
        const float* ptr[]{nullptr, block.data()};
        for (int i = 0; i < seconds * 10; ++i) {
            std::fill(block.begin(), block.end(), (float)i);
            p.process(ptr, 2, nullptr, 0, 800);
        }
        CHECK(p.finished.load() == (seconds == 15));
        if (seconds == 15)
            CHECK(p.drop == start + 80000);
        else
            CHECK(p.drop < 0);
        REQUIRE(p.drain());
        p.finishDisk();
        if (p.drop >= 0)
            s.dropouts.push_back(p.drop.load());
        REQUIRE(recoverTake(s, w.dir, p.journal));
        CHECK(s.clips.back().length == std::min(seconds, 10) * 8000);
        auto reader = waveReader(w.dir.getChildFile(s.assets.back().path));
        REQUIRE(reader);
        AudioBuffer<float> sample(1, 1);
        for (int i = 0; i < std::min(seconds, 10) * 10; ++i) {
            REQUIRE(reader->read(&sample, 0, 1, i * 800, true, false));
            CHECK(sample.getSample(0, 0) == (float)i);
        }
        bool damaged = false;
        REQUIRE(Song::load(w.dir, damaged)->dropouts == s.dropouts);
    }
}
TEST_CASE("Sync diagnostic measures both ends of a ten minute virtual cable") {
    Workspace w;
    auto s = example();
    s.grid.rate = 8000;
    s.grid.bpm = 120;
    Performance p(s, {}, 0, true, false, 0);
    p.syncPulse = true;
    const int delay = 177;
    p.end = Frame(s.grid.rate) * 600 + delay;
    REQUIRE(p.arm(s, w.dir, {"In 1", "In 2"}, delay, false));
    std::array<float, 177> wire{};
    std::array<float, 80> input{}, output{};
    const float* in[]{nullptr, input.data()};
    float* out[]{output.data()};
    for (Frame pos = 0; pos < p.end; pos += 80) {
        output.fill(0);
        for (int i = 0; i < 80; ++i)
            input[i] = -0.4f * wire[(pos + i) % delay];
        p.process(in, 2, out, 1, 80);
        for (int i = 0; i < 80; ++i)
            wire[(pos + i) % delay] = output[i];
        if (pos % 8000 == 0)
            REQUIRE(p.drain());
    }
    REQUIRE(p.finished.load());
    REQUIRE(p.drain());
    p.finishDisk();
    REQUIRE(recoverTake(s, w.dir, p.journal));
    auto reader = waveReader(w.dir.getChildFile(s.assets.back().path));
    REQUIRE(reader);
    auto aligned = syncOffsets(*reader, s.grid, delay);
    REQUIRE(aligned);
    CHECK(aligned->first == 0);
    CHECK(aligned->second == 0);
    auto offset = syncOffsets(*reader, s.grid, delay - 4);
    REQUIRE(offset);
    CHECK(offset->first == 4);
    CHECK(offset->second == 4);
}
} // namespace

TEST_CASE("A-21 sample continuity through both edges of opposite-phase takes", "[realtime]") {
    Workspace w;
    auto s = example();
    s.tracks.resize(1);
    s.tracks.front().kind = "jine";
    const int length = 32000, start = 5371, stop = 23543;
    auto track = s.tracks.front().id;
    for (int id = 0; id < 2; ++id) {
        String path = "Zvuk/" + String(id) + ".wav";
        File file = w.dir.getChildFile(path);
        REQUIRE(file.getParentDirectory().createDirectory().wasOk());
        auto writer = waveWriter(file, s.grid.rate, 1);
        REQUIRE(writer);
        AudioBuffer<float> source(1, length);
        for (int i = 0; i < length; ++i)
            source.setSample(0, i, (id ? -.7f : .7f) * std::cos(float(i) * 0.004f));
        REQUIRE(writer->writeFromAudioSampleBuffer(source, 0, length));
        s.assets.push_back({String(id), path, length, 1});
    }
    s.clips = {{"base", track, "0", 0, 0, length}};
    REQUIRE(s.addAttempt({"pass", track, "1", 0, 0, length}, start, stop, 123));
    AudioLibrary library;
    REQUIRE(library.load(s, w.dir));
    Playback playback(s, library);
    std::vector<float> output(length);
    for (int at = 0; at < length; at += 157) {
        float* samples[]{output.data() + at};
        playback.mix(samples, 1, std::min(157, length - at), at);
    }
    float maxStep = 0;
    for (int i = 1; i < length; ++i) {
        float original = .7f * std::cos(float(i) * 0.004f);
        float weight = std::clamp(float(std::min(i - start, stop - 1 - i)) / 480.f, 0.f, 1.f);
        if (std::abs(output[i] - original * (1 - 2 * weight)) > 0.00001f)
            FAIL("Crossfade differs at sample " << i);
        maxStep = std::max(maxStep, std::abs(output[i] - output[i - 1]));
    }
    CHECK(maxStep < .006f);
    playback.selectAttempt(s.groups.front().id, 0);
    std::array<float, 13> next{};
    float* channels[]{next.data()};
    playback.mix(channels, 1, 13, start + 3000);
    CHECK(std::abs(next[0] - .7f * std::cos(float(start + 3000) * .004f)) < .00001f);
    // Change selection in continuous playback, including a second request mid-fade.
    int position = start + 3013;
    for (int selected : {1, 0, 1, 1, 1}) {
        playback.selectAttempt(s.groups.front().id, selected);
        float previous = next.back();
        next.fill(0);
        playback.mix(channels, 1, (int)next.size(), position);
        CHECK(std::abs(previous - next.front()) < .006f);
        position += (int)next.size();
    }
    REQUIRE(s.save(w.dir));
    bool bad = false;
    REQUIRE(Song::load(w.dir, bad) == s);
    REQUIRE_FALSE(bad);
}

TEST_CASE("A-02 cropped occurrence fades and moves the following WAV without changing its bytes") {
    Workspace w;
    auto s = example();
    s.bars = 20;
    s.grid.bpm = 240;
    Frame length = s.barFrame(20);
    auto file = w.dir.getChildFile("Zvuk/source.wav");
    REQUIRE(file.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(file, s.grid.rate, 1);
    REQUIRE(writer);
    AudioBuffer<float> bar(1, (int)s.barFrame(1));
    for (int n = 0; n < 20; ++n) {
        std::fill_n(bar.getWritePointer(0), bar.getNumSamples(), float(n + 1) / 32);
        REQUIRE(writer->writeFromAudioSampleBuffer(bar, 0, bar.getNumSamples()));
    }
    writer.reset();
    MemoryBlock before, after;
    REQUIRE(file.loadFileAsData(before));
    s.assets = {{"a", "Zvuk/source.wav", length, 1}};
    s.clips = {{"clip", "dr", "a", 0, 0, length}};
    REQUIRE(s.splitSection(8));
    REQUIRE(s.splitSection(16));
    REQUIRE(s.repeatSection(1, 2));
    REQUIRE(s.resizeOccurrence(2, 6));
    AudioLibrary lib;
    REQUIRE(lib.load(s, w.dir));
    Playback mix(s, lib);
    std::array<float, 1024> samples{};
    float* channels[]{samples.data()};
    mix.mix(channels, 1, 1024, s.barFrame(22) - 512);
    CHECK(samples.front() == 14.f / 32);
    CHECK(samples.back() == 17.f / 32);
    CHECK(samples[511] == 0.f);
    CHECK(samples[512] == 0.f);
    for (size_t i = 1; i < samples.size(); ++i)
        if (std::abs(samples[i] - samples[i - 1]) > .003f)
            FAIL("Cropped edge discontinuity at " << i);
    REQUIRE(file.loadFileAsData(after));
    CHECK(after == before);
}

TEST_CASE("Recover a capture crossing a section boundary into source-aligned clips") {
    Workspace w;
    auto song = example();
    song.bars = 16;
    REQUIRE(song.splitSection(8));
    auto id = Uuid().toString();
    auto path = w.dir.getChildFile("Zvuk/" + id + ".wav");
    REQUIRE(path.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(path, song.grid.rate, 1);
    REQUIRE(writer);
    AudioBuffer<float> input(1, 2400);
    input.clear();
    REQUIRE(writer->writeFromAudioSampleBuffer(input, 0, input.getNumSamples()));
    writer.reset();
    auto journal = w.dir.getChildFile("Zachrana/capture.json");
    REQUIRE(journal.getParentDirectory().createDirectory().wasOk());
    auto start = song.barFrame(8) - 600;
    auto json = object({{"poloha", start},
                        {"kompenzace", 33},
                        {"sampleRate", song.grid.rate},
                        {"vyskyt", song.order[0].id},
                        {"dil", song.order[0].section},
                        {"stopy", Array<var>{object({{"stopa", "dr"}, {"id", id}})}}});
    REQUIRE(journal.replaceWithText(JSON::toString(json)));
    REQUIRE(recoverTake(song, w.dir, journal));
    REQUIRE(song.clips.size() == 2);
    CHECK(song.clips[0].length == 600);
    CHECK(song.clips[0].offset == 33);
    CHECK(song.clips[1].position == song.barFrame(8));
    CHECK(song.clips[1].length == 1767);
    CHECK(song.clips[1].offset == 633);
    CHECK(song.clips[0].asset == song.clips[1].asset);
    bool damaged = false;
    REQUIRE(Song::load(w.dir, damaged) == song);
    REQUIRE_FALSE(damaged);
    REQUIRE(journal.replaceWithText(JSON::toString(json)));
    REQUIRE(recoverTake(song, w.dir, journal));
    CHECK(song.clips.size() == 2);
}

TEST_CASE("Mix export preserves ranges, channel power, overload locations and deterministic dither") {
    Workspace w;
    Song s;
    s.title = "Mix";
    s.grid = {96000, 4, 4, 120};
    s.bars = 2;
    s.tracks = {{"a", "Guitar", "jine"}};
    auto file = w.dir.getChildFile("Zvuk/input.wav");
    REQUIRE(file.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(file, 96000, 1);
    REQUIRE(writer);
    AudioBuffer<float> signal(1, 1920);
    for (int i = 0; i < 1920; ++i)
        signal.setSample(0, i, .9f);
    REQUIRE(writer->writeFromAudioSampleBuffer(signal, 0, 1920));
    writer.reset();
    auto start = s.barFrame(1);
    s.assets = {{"w", "Zvuk/input.wav", 1920, 1}};
    s.clips = {{"c", "a", "w", start, 0, 1920}};
    AudioLibrary library;
    REQUIRE(library.load(s, w.dir));
    Playback live(s, library);
    AudioBuffer<float> block(2, 512);
    block.clear();
    live.mix(block.getArrayOfWritePointers(), 2, 512, start + 500);
    auto peak = live.meter();
    CHECK(std::abs(peak.left - .9 / std::sqrt(2.)) < 1e-6);
    CHECK(peak.left == peak.right);
    s.tracks[0].pan = -1;
    s.masterDb = 6;
    live.updateMix(s);
    block.clear();
    live.mix(block.getArrayOfWritePointers(), 2, 512, start + 1100);
    CHECK(live.meter().right == 0);
    CHECK(live.meter().over == start + 1100);
    for (auto format : {Delivery::wave24, Delivery::wave16}) {
        auto dest = w.dir.getChildFile("range.wav");
        auto rendered = exportMix(s, library, dest, format, start, start + 9600);
        REQUIRE(rendered.ok);
        CHECK(rendered.meter.maximum > 1.5f);
        REQUIRE(rendered.meter.over >= start);
        CHECK(rendered.meter.over < start + 1920);
        auto r = waveReader(dest);
        REQUIRE(r);
        CHECK(r->lengthInSamples == (format == Delivery::wave24 ? 4800 : 4410));
        CHECK(r->bitsPerSample == (format == Delivery::wave24 ? 24 : 16));
        CHECK_FALSE(r->usesFloatingPointData);
        AudioBuffer<float> tail(2, 256);
        REQUIRE(r->read(&tail, 0, 256, 3200, true, true));
        if (format == Delivery::wave16) {
            CHECK(tail.getMagnitude(0, 256) > 0);
            CHECK(tail.getMagnitude(0, 256) <= 1.f / 32768);
        } else
            CHECK(tail.getMagnitude(0, 256) == 0);
        r.reset();
        MemoryBlock a, b;
        REQUIRE(dest.loadFileAsData(a));
        REQUIRE(exportMix(s, library, dest, format, start, start + 9600).ok);
        REQUIRE(dest.loadFileAsData(b));
        CHECK(a == b);
    }
}
TEST_CASE("M-03 detector recognizes the monitor metronome and rejects the delivered song") {
    Workspace w;
    Song s;
    s.title = "Click";
    s.grid = {48000, 4, 4, 160};
    s.bars = 1;
    auto file = w.dir.getChildFile("test.wav");
    REQUIRE(exportMix(s, {}, file, Delivery::wave24).ok);
    REQUIRE(containsClick(file, s.grid) == false);
    Performance p(s, {}, 0, false, true, 0);
    AudioBuffer<float> capture(2, (int)p.end);
    capture.clear();
    p.process(nullptr, 0, capture.getArrayOfWritePointers(), 2, capture.getNumSamples());
    auto injected = w.dir.getChildFile("headphones.wav");
    auto writer = waveWriter(injected, 48000, 2);
    REQUIRE(writer);
    REQUIRE(writer->writeFromAudioSampleBuffer(capture, 0, capture.getNumSamples()));
    writer.reset();
    REQUIRE(containsClick(injected, s.grid) == true);
}
TEST_CASE("Legacy mono material keeps its pre-pan level and capture marks the input before mixing") {
    Workspace w;
    Song s = example();
    s.bars = 1;
    s.tracks[0].pan = 0;
    s.assets = {{"a", "Zvuk/sample.wav", 1024, 1}};
    s.clips = {{"c", "dr", "a", 0, 0, 1024}};
    auto legacy = s.json();
    for (auto& track : *legacy.getDynamicObject()->getProperty("stopy").getArray())
        track.getProperty("mix", {}).getDynamicObject()->removeProperty("gainDb");
    bool bad = false;
    auto read = Song::parse(legacy, bad);
    REQUIRE(read);
    CHECK_FALSE(bad);
    CHECK(std::abs(read->tracks.front().gainDb - 3.0103f) < .00001f);
    CHECK(read->tracks.back().gainDb == 0.f);
    s.tracks[1].gainDb = -96.f;
    Performance p(s, {}, 1024, true, false, 0);
    REQUIRE(p.arm(s, w.dir, {"In 1", "In 2"}, 12, false));
    std::array<float, 64> input{}, left{}, right{};
    input[27] = .988f;
    input[28] = .989f;
    const float* in[]{nullptr, input.data()};
    float* out[]{left.data(), right.data()};
    p.process(in, 2, out, 2, 64);
    p.finishDisk();
    p.storeInputMarks(s);
    CHECK(s.tracks[0].inputOver == -1);
    CHECK(s.tracks[1].inputOver == 1024 + 28 - 12);
    auto reloaded = Song::parse(s.json(), bad);
    REQUIRE(reloaded);
    CHECK(reloaded->tracks == s.tracks);
}
