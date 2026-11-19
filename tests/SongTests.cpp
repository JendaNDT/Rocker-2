#include "FakeDevice.h"
#include "song/Song.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
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
    s.tracks.front().pan=-1; // Unity on the measured left channel for timing and edge tests.
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
        REQUIRE(renderSong(s, lib, file));
        auto reader = waveReader(file);
        REQUIRE(reader);
        AudioBuffer<float> rendered(2, 500);
        REQUIRE(reader->read(&rendered, 0, 500, 0, true, true));
        CHECK(rendered.getSample(0, 329) == 0.75f);
        CHECK(rendered.getSample(1,329)==0.f);
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
