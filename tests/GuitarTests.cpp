#include "engine/Export.h"
#include "engine/Audio.h"
#include "engine/LoadTrial.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <iostream>

namespace {
using namespace juce;
using namespace rocker;

void playbackBlock(Audio& device, float* const* out, int count)
#if defined(ROCKER_RT_AUDIT)
    [[clang::nonblocking]]
#endif
{
    device.process(nullptr, 0, out, 2, count);
}

struct Session {
    File dir =
        File::getSpecialLocation(File::tempDirectory).getChildFile("rocker-amp-" + Uuid().toString());
    Song song;
    AudioLibrary library;
    AudioBuffer<float> source{2, 9600};
    Session() {
        song.title = "A05";
        song.grid = {48000, 1, 4, 300};
        song.bars = 1;
        song.tracks = {{"dr", "Drums", "bici"}, {"g1", "Guitar", "kytara"}, {"g2", "Lead", "kytara"}};
        song.tracks[1].amp = {"Crunch", presets[1], true, false};
        song.tracks[2].amp = {"Lead", presets[2], true, false};
        auto file = dir.getChildFile("Zvuk/riff.wav");
        REQUIRE(file.getParentDirectory().createDirectory().wasOk());
        for (int i = 0; i < source.getNumSamples(); ++i) {
            source.setSample(0, i, i % 2400 < 1500 ? 0.12f * std::sin(i * 0.051f) : 0.f);
            source.setSample(1, i, i % 1900 < 1100 ? 0.18f * std::cos(i * 0.087f) : 0.f);
        }
        auto writer = waveWriter(file, 48000, 2);
        REQUIRE(writer);
        REQUIRE(writer->writeFromAudioSampleBuffer(source, 0, source.getNumSamples()));
        writer.reset();
        song.assets = {{"a", "Zvuk/riff.wav", 9600, 2}};
        song.clips = {{"d", "dr", "a", 52, 360, 110},
                      {"c1", "g1", "a", 0, 0, 9600},
                      {"c2", "g2", "a", 137, 800, 8200}};
        REQUIRE(library.load(song, dir));
    }
    ~Session() {
        library.data.clear();
        dir.deleteRecursively();
    }
    AudioBuffer<float> play(int block) {
        Audio device;
        device.prepare(0, 48000);
        Performance p(song, library, 0, false, false, 0);
        device.timeline.attach(p);
        AudioBuffer<float> result(2, (int)p.end);
        for (int pos = 0; pos < result.getNumSamples(); pos += block) {
            float* out[]{result.getWritePointer(0, pos), result.getWritePointer(1, pos)};
            playbackBlock(device, out, std::min(block, result.getNumSamples() - pos));
        }
        device.timeline.detach();
        REQUIRE(p.finished.load());
        return result;
    }
};

float difference(const AudioBuffer<float>& a, const AudioBuffer<float>& b) {
    REQUIRE(a.getNumSamples() == b.getNumSamples());
    float peak = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < a.getNumSamples(); ++i) {
            const float delta = std::abs(a.getSample(ch, i) - b.getSample(ch, i));
            if (!std::isfinite(delta))
                return std::numeric_limits<float>::infinity();
            peak = std::max(peak, delta);
        }
    return peak;
}
TEST_CASE("A-05 two independent stereo guitar amps and drums match the offline WAV including tails",
          "[realtime]") {
    Session s;
    auto file = s.dir.getChildFile("mix.wav");
    MemoryBlock first;
    s.song.tracks[1].pan=.37f;
    s.song.tracks[2].gainDb=-2.f;
    s.song.masterDb=-1.f;
    REQUIRE(exportMix(s.song, s.library, file, Delivery::wave24).ok);
    REQUIRE(file.loadFileAsData(first));
    REQUIRE(exportMix(s.song, s.library, file, Delivery::wave24).ok);
    MemoryBlock second;
    REQUIRE(file.loadFileAsData(second));
    REQUIRE(first == second);
    auto reader = waveReader(file);
    REQUIRE(reader);
    REQUIRE(reader->lengthInSamples == s.song.end() + 2 * s.song.grid.rate);
    AudioBuffer<float> render(2, (int)reader->lengthInSamples);
    REQUIRE(reader->read(&render, 0, render.getNumSamples(), 0, true, true));
    REQUIRE(render.getMagnitude(9600, 24000) > 0.0001f);
    for (int n : {64, 128, 512, 1024})
        REQUIRE(difference(s.play(n), render) <= 0.0000005f);
}
TEST_CASE(
    "A-20 recalls and embedded user settings reproduce guitar sound independently of the library") {
    Session s;
    auto original = s.play(128);
    s.song.tracks[1].amp.values = presets[3];
    REQUIRE(difference(original, s.play(128)) > 0.01f);
    s.song.tracks[1].amp.values = presets[1];
    REQUIRE(difference(original, s.play(128)) == 0);
    auto& setting = s.song.tracks[1].amp;
    setting.name = String(u8"Moje ozvěna");
    setting.values.echo = 72;
    setting.values.room = 54;
    setting.modified = true;
    auto edited = s.play(128);
    REQUIRE(difference(edited, original) > 0.001f);
    REQUIRE(s.song.save(s.dir));
    bool damaged;
    auto loaded = Song::load(s.dir, damaged);
    REQUIRE(loaded);
    REQUIRE_FALSE(damaged);
    REQUIRE(loaded->tracks[1].amp.values == setting.values);
    REQUIRE(loaded->tracks[1].amp.name == setting.name);
    REQUIRE(loaded->tracks[1].amp.modified);
    s.song = *loaded;
    REQUIRE(difference(edited, s.play(128)) == 0);
}
TEST_CASE("Playback advances guitar by the cabinet delay and preserves the first source samples") {
    Session s;
    auto& track = s.song.tracks[1];
    track.solo = true;
    const ScopedNoDenormals guard;
    auto played = s.play(64);
    for (int ch = 0; ch < 2; ++ch) {
        Amp reference;
        reference.prepare(s.song.grid.rate);
        reference.configure(track.amp.values);
        reference.reset();
        float error = 0;
        for (int i = 0; i < s.source.getNumSamples(); ++i) {
            float output = reference.process(s.source.getSample(ch, i));
            if (i >= reference.delaySamples())
                error = std::max(error,
                                 std::abs(played.getSample(ch, i - reference.delaySamples()) - output));
        }
        REQUIRE(error <= 0.000001f);
    }
}
TEST_CASE("Documented amp units and older dry guitars load without a preset library") {
    REQUIRE_FALSE(GuitarSound::read({}).enabled);
    auto saved = GuitarSound::read(object({{"typ", "distortion"}, {"brana", -62}, {"zkresleni", 0.48},
                                           {"basy", 0.1}, {"jas", 0.0}, {"dozvuk", 0.12}}));
    REQUIRE(saved.enabled);
    CHECK(saved.values.shape == 1);
    CHECK(saved.values.gate == 62);
    CHECK(saved.values.drive == 48);
    CHECK(saved.values.bass == 55);
    CHECK(saved.values.presence == 50);
    CHECK(saved.values.room == 12);
    for (const auto& preset : presets) {
        saved.values = preset;
        REQUIRE(GuitarSound::read(saved.json()).values == preset);
    }
}
TEST_CASE("A-20 switches the active track on the next block and recalls Crunch exactly") {
    Session s;
    auto& track = s.song.tracks[1];
    track.solo = true;
    s.song.clips.clear();
    constexpr int length = 4096;
    for (int i = 0; i < 3; ++i)
        s.song.clips.push_back({String(i), track.id, "a", i * length, 0, length});
    Playback player(s.song, s.library);
    AudioBuffer<float> blocks[3];
    for (int part = 0; part < 3; ++part) {
        track.amp.values = presets[part == 1 ? 3 : 1];
        player.setSound(1, track.amp, true);
        player.setSound(1, track.amp, false); // A slider notification must preserve the pending recall.
        auto& block = blocks[part];
        block.setSize(2, length);
        block.clear();
        for (int i = 0; i < length; i += 128) {
            float* out[]{block.getWritePointer(0, i), block.getWritePointer(1, i)};
            player.mix(out, 2, 128, part * length + i);
        }
    }
    // Compare the repeated phrase before the lookahead enters the next clip.
    for (auto& block : blocks)
        block.setSize(2, length - 256, true);
    REQUIRE(difference(blocks[0], blocks[1]) > 0.01f);
    REQUIRE(difference(blocks[0], blocks[2]) == 0);
}
TEST_CASE("M-05 trial covers ten minutes of sixteen tracks with eight complete amp chains") {
    Session s;
    Song trial;
    auto path = s.dir.getChildFile("trial");
    REQUIRE(makeLoadTrial(trial, path, 8000, 8));
    REQUIRE(trial.end() == 600 * 8000);
    REQUIRE(trial.tracks.size() == 16);
    CHECK(std::count_if(trial.tracks.begin(), trial.tracks.end(),
                        [](auto& t) { return t.amp.enabled; }) == 8);
    AudioLibrary lib;
    REQUIRE(lib.load(trial, path));
    Performance p(trial, lib, 600 * 8000 - 2048, false, false, 0);
    p.end = trial.end();
    AudioBuffer<float> out(2, 2048);
    out.clear();
    p.process(nullptr, 0, out.getArrayOfWritePointers(), 2, 2048);
    REQUIRE(out.getMagnitude(0, 2048) > 0.001f);
    REQUIRE(p.finished.load());
}

TEST_CASE("Time the M-05 mix without an audio device", "[.benchmark]") {
    Session s;
    Song trial;
    const int rate = 48000, block = 128, frames = rate * 600;
    auto folder = s.dir.getChildFile("benchmark");
    REQUIRE(makeLoadTrial(trial, folder, rate, 8));
    AudioLibrary lib;
    REQUIRE(lib.load(trial, folder));
    Performance player(trial, lib, 0, false, false, 0);
    player.end = frames;
    Audio device;
    device.prepare(0, rate);
    device.timeline.attach(player);
    AudioBuffer<float> output(2, block);
    auto start = std::chrono::steady_clock::now();
    for (int at = 0; at < frames; at += block)
        playbackBlock(device, output.getArrayOfWritePointers(), std::min(block, frames - at));
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    device.timeline.detach();
    REQUIRE(player.finished.load());
    std::cout << "M-05 offline: " << frames << " frames, " << seconds << " s, mean " << seconds / 6
              << "% of realtime\n";
}

} // namespace
