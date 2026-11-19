#include "engine/Audio.h"
#include "engine/Export.h"
#include <catch2/catch_test_macros.hpp>
using namespace rocker;
using namespace juce;
namespace {
struct Temp {
    File dir = File::getSpecialLocation(File::tempDirectory).getChildFile(Uuid().toString());
    Temp() { REQUIRE(dir.createDirectory().wasOk()); }
    ~Temp() { dir.deleteRecursively(); }
};
} // namespace
TEST_CASE("A-22 timestamped drum strikes stay on the beat through ten minutes") {
    Temp temp;
    Song song;
    song.title = "MIDI";
    song.grid.bpm = 137;
    song.tracks = {{"m", "MIDI", "midi", {}, true}};
    Performance p(song, {}, 0, true, false, 0);
    REQUIRE(p.arm(song, temp.dir, {}, 0, false, 173));
    DrumInput input;
    input.prepare(song.grid.rate);
    Frame beat = 0;
    const auto end = Frame(song.grid.rate) * 600;
    for (Frame at = 0; at < end;) {
        int n = int(std::min<Frame>(701, end - at));
        while (song.grid.beat(beat) + 173 < at + n) {
            REQUIRE(input.push({song.grid.beat(beat++) + 173, 38, 99}));
        }
        auto events = input.block(n, double(at) / song.grid.rate);
        p.hits(events, at, false);
        p.process(nullptr, 0, nullptr, 0, n);
        input.render(nullptr, 0, n, events);
        at += n;
        if (at % 7010 == 0)
            REQUIRE(p.drain());
    }
    p.finishDisk();
    REQUIRE(recoverTake(song, temp.dir, p.journal));
    REQUIRE(song.assets.size() == 1);
    REQUIRE(song.clips.size() == 1);
    REQUIRE(song.clips[0].offset == 173);
    auto notes = readMidi(temp.dir.getChildFile(song.assets[0].path), song.grid.rate);
    REQUIRE(notes);
    REQUIRE(notes->size() == (size_t)beat);
    for (Frame i = 0; i < beat; ++i) {
        CHECK(std::abs((*notes)[i].frame - 173 - song.grid.beat(i)) <= 1);
        CHECK((*notes)[i].velocity == 99);
    }
}
TEST_CASE("A-23 MIDI cuts, attempts and kit changes survive deterministic WAV export") {
    Temp temp;
    Song s;
    s.title = "MIDI";
    s.tracks = {{"m", "MIDI", "midi"}};
    constexpr int length = 48000;
    const std::vector<Hit> hits{{0, 36, 100}, {12000, 42, 76}, {24000, 38, 127}, {36000, 49, 75}};
    s.assets = {{"h", "Zvuk/h.mid", length, 1}};
    s.clips = {{"c", "m", "h", 0, 0, length}};
    REQUIRE(saveMidi(temp.dir.getChildFile("Zvuk/h.mid"), 48000, length, hits));
    REQUIRE(s.addAttempt({"p", "m", "h", 0, 0, length}, 12000, 24000, 1));
    REQUIRE(s.save(temp.dir));
    bool bad = false;
    auto loaded = Song::load(temp.dir, bad);
    REQUIRE(loaded);
    CHECK_FALSE(bad);
    AudioLibrary library;
    std::vector<MemoryBlock> files;
    for (int kit : {0, 1, 0}) {
        loaded->tracks[0].kit = kit;
        REQUIRE(library.load(*loaded, temp.dir));
        auto out = temp.dir.getChildFile(String((int)files.size()) + ".wav");
        REQUIRE(renderSong(*loaded, library, out, 0, length));
        MemoryBlock bytes;
        REQUIRE(out.loadFileAsData(bytes));
        files.push_back(std::move(bytes));
        auto reader = waveReader(out);
        REQUIRE(reader);
        Range<float> peak[2];
        reader->readMaxLevels(0, length, peak, 2);
        CHECK(peak[0].getEnd() > .01f);
    }
    CHECK(files[0] != files[1]);
    CHECK(files[0] == files[2]);
    auto restored = readMidi(temp.dir.getChildFile("Zvuk/h.mid"), 48000);
    REQUIRE(restored);
    CHECK(*restored == hits);
}
TEST_CASE("M-14 compares sixteen independent snare transients in the audio and MIDI take") {
    Temp temp;
    auto file = temp.dir.getChildFile("drums.wav");
    auto writer = waveWriter(file, 48000, 1);
    REQUIRE(writer);
    AudioBuffer<float> audio(1, 96000);
    audio.clear();
    std::vector<Hit> notes;
    for (int i = 0; i < 16; ++i) {
        int at = 5000 + i * 5600;
        audio.setSample(0, at + 372, .7f);
        notes.push_back({at + 173 + 49, 38, 99});
    }
    REQUIRE(writer->writeFromAudioSampleBuffer(audio, 0, 96000));
    writer.reset();
    auto reader = waveReader(file);
    REQUIRE(reader);
    CHECK(compareSnare(notes, *reader, 173, 372) == 49);
    notes.pop_back();
    CHECK_FALSE(compareSnare(notes, *reader, 173, 372));
}
