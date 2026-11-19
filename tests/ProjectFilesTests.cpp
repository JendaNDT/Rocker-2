#include "storage/ProjectFiles.h"
#include "engine/Devices.h"
#include <catch2/catch_test_macros.hpp>
namespace {
using namespace rocker;
using namespace juce;
struct Temp {
    File dir = File::getSpecialLocation(File::tempDirectory).getChildFile("rocker-" + Uuid().toString());
    Temp() { REQUIRE(dir.createDirectory().wasOk()); }
    ~Temp() { dir.deleteRecursively(); }
};
TEST_CASE("A-10 A-11 Czech long paths and all song files survive a ZIP64 package") {
    Temp temp;
    File root = temp.dir;
    for (int n = 0; n < 5; ++n)
        root = root.getChildFile(String(u8"Příliš žluťoučký kůň ") + String(n) +
                                 String::repeatedString("ab", 9));
    auto dir = root.getChildFile(String(u8"Noční riff – ř ů ě"));
    Song s;
    s.title = dir.getFileName();
    s.tracks = {{"g", "Guitar", "kytara", {"In 1"}}, {"m", "Kit", "midi"}};
    const int length = 8000;
    s.assets = {{"w", "Zvuk/a.wav", length, 1}, {"m", "Zvuk/b.mid", length, 1}};
    s.clips = {{"c", "g", "w", 0, 0, length}, {"d", "m", "m", 0, 0, length}};
    REQUIRE(dir.getChildFile("Zvuk").createDirectory().wasOk());
    auto writer = waveWriter(dir.getChildFile(s.assets[0].path), 48000, 1);
    REQUIRE(writer);
    AudioBuffer<float> samples(1, length);
    samples.clear();
    samples.setSample(0, 25, .5);
    REQUIRE(writer->writeFromAudioSampleBuffer(samples, 0, length));
    writer.reset();
    std::vector<Hit> hits{{123, 38, 96}};
    REQUIRE(saveMidi(dir.getChildFile(s.assets[1].path), 48000, length, hits));
    REQUIRE(s.save(dir));
    REQUIRE(s.archive(dir));
    ViewState v;
    v.cursor = 471;
    REQUIRE(v.save(dir));
    auto pack = temp.dir.getChildFile("song.rocker2");
    REQUIRE(packSong(dir, pack));
    auto dest = unpackSong(pack, temp.dir.getChildFile("second"));
    REQUIRE(dest);
    CHECK(dest->getFileName() == s.title);
    for (auto& f : dir.findChildFiles(File::findFiles, true)) {
        auto copy = dest->getChildFile(f.getRelativePathFrom(dir));
        CHECK(copy.existsAsFile());
        CHECK(copy.hasIdenticalContentTo(f));
    }
    bool bad = false;
    auto restored = Song::load(*dest, bad);
    REQUIRE(restored);
    CHECK_FALSE(bad);
    CHECK(*restored == s);
    CHECK(ViewState::load(*dest).cursor == 471);
    AudioLibrary lib;
    REQUIRE(lib.load(*restored, *dest));
    Playback music(*restored, lib);
    float l[256]{}, r[256]{};
    float* out[]{l, r};
    music.mix(out, 2, 256, 0);
    CHECK(l[25] > 0);
    auto another = unpackSong(pack, temp.dir.getChildFile("second"));
    REQUIRE(another);
    CHECK(*another != *dest);
    auto broken = temp.dir.getChildFile("broken.rocker2");
    REQUIRE(broken.replaceWithText("broken"));
    CHECK_FALSE(unpackSong(broken, temp.dir.getChildFile("second")));
    CHECK(temp.dir.getChildFile("second").findChildFiles(File::findDirectories, false).size() == 2);
}
TEST_CASE("Explicit cleanup protects current clips, undo, archives and recovery") {
    Temp temp;
    auto& dir = temp.dir;
    Song s;
    s.title = "Cleanup";
    s.tracks = {{"t", "Tape", "jine"}};
    auto audio = dir.getChildFile("Zvuk");
    REQUIRE(audio.createDirectory().wasOk());
    for (auto name : {"used", "undo", "orphan"}) {
        auto a = Asset{String(name), "Zvuk/" + String(name) + ".wav", 100, 1};
        s.assets.push_back(a);
        REQUIRE(dir.getChildFile(a.path).replaceWithText("fixture"));
    }
    s.clips = {{"c", "t", "used", 0, 0, 100}, {"u", "t", "undo", 100, 0, 100}};
    EditHistory h;
    h.reset(s);
    s.clips.pop_back();
    h.remember(s, "edit");
    auto files = unusedAudio(s, dir, h);
    REQUIRE(files.size() == 1);
    CHECK(files[0].getFileName() == "orphan.wav");
    auto rescue = dir.getChildFile("Zachrana");
    REQUIRE(rescue.createDirectory().wasOk());
    auto log = rescue.getChildFile("take.json");
    REQUIRE(log.replaceWithText("{}"));
    CHECK(unusedAudio(s, dir, h).isEmpty());
    REQUIRE(log.deleteFile());
    CHECK(trashAudio(s, dir, h,
                     [&](const File& f) { return f.moveFileTo(temp.dir.getChildFile("bin.wav")); }) == 1);
    CHECK(audio.getChildFile("used.wav").existsAsFile());
    CHECK(audio.getChildFile("undo.wav").existsAsFile());
    CHECK(s.assets.size() == 2);
}
TEST_CASE("A-13 free space warning and a controlled stop preserve every sample already accepted") {
    Temp temp;
    Song s;
    s.title = "Space";
    s.tracks = {{"t", "Input", "bici", {"port"}, true}};
    Frame free = 4LL * 3600 * s.grid.rate * 4;
    // Four channels' worth of disk bandwidth crosses the 30-minute warning before the reserve.
    for (int i = 1; i < 4; ++i)
        s.tracks.push_back({String(i), "Input", "bici", {"port"}, true});
    Performance p(s, {}, 0, true, false, 0);
    p.diskSpace = [&](auto&) { return free; };
    REQUIRE(p.arm(s, temp.dir, {"port"}, 0, false));
    CHECK_FALSE(p.diskWarning);
    free = 800000000;
    std::array<float, 1024> data{};
    const float* in[]{data.data()};
    Frame frame = 0;
    auto feed = [&](int blocks) {
        for (int i = 0; i < blocks; ++i) {
            for (size_t j = 0; j < data.size(); ++j)
                data[j] = float((frame + Frame(j)) % 8192) / 8192.f;
            p.process(in, 1, nullptr, 0, (int)data.size());
            frame += data.size();
            REQUIRE(p.drain());
        }
    };
    feed(100);
    REQUIRE(p.diskWarning);
    CHECK_FALSE(p.finished);
    free = 490000000;
    while (!p.finished && frame < 4LL * s.grid.rate + 8096)
        feed(1);
    REQUIRE(p.diskStop);
    REQUIRE(p.finished);
    auto end = p.cursor.load();
    p.process(in, 1, nullptr, 0, 1024);
    CHECK(p.cursor.load() == end);
    p.finishDisk();
    REQUIRE(recoverTake(s, temp.dir, p.journal));
    REQUIRE(s.assets.size() == 4);
    for (auto& a : s.assets) {
        auto reader = waveReader(temp.dir.getChildFile(a.path));
        REQUIRE(reader);
        REQUIRE(reader->lengthInSamples == end);
        AudioBuffer<float> captured(1, 2048);
        int mismatches = 0;
        for (Frame at = 0; at < end; at += 2048) {
            auto n = int(std::min<Frame>(2048, end - at));
            REQUIRE(reader->read(&captured, 0, n, at, true, false));
            for (int i = 0; i < n; ++i)
                mismatches += captured.getSample(0, i) != float((at + i) % 8192) / 8192.f;
        }
        CHECK(mismatches == 0);
    }
}
TEST_CASE("OneDrive and unrelated Windows clocks are recognized without affecting CoreAudio or ASIO") {
    Temp temp;
    CHECK(syncedFolder(temp.dir.getChildFile("OneDrive - Kapela").getChildFile("Skladby")));
    CHECK_FALSE(syncedFolder(temp.dir.getChildFile("Rocker 2").getChildFile("Skladby")));
    DeviceChoice d{Driver::shared, "Internal microphone", "USB output"};
    CHECK(separateWindowsClocks(d));
    d.driver = Driver::core;
    CHECK_FALSE(separateWindowsClocks(d));
    d.driver = Driver::asio;
    CHECK_FALSE(separateWindowsClocks(d));
}
} // namespace
