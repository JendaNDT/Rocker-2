#include "engine/Audio.h"
#include "engine/LoadTrial.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace {
using namespace juce;
using namespace rocker;

struct Session {
    File dir = File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("rocker-amp", "", true);
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
        song.clips = {{"d", "dr", "a", 52, 360, 110}, {"c1", "g1", "a", 0, 0, 9600}, {"c2", "g2", "a", 137, 800, 8200}};
        REQUIRE(library.load(song, dir));
    }
    ~Session() { library.data.clear(); dir.deleteRecursively(); }
    AudioBuffer<float> play(int block) {
        Audio device;
        device.prepare(0, 48000);
        Performance p(song, library, 0, false, false, 0);
        device.timeline.attach(p);
        AudioBuffer<float> result(2, (int)p.end);
        for (int pos = 0; pos < result.getNumSamples(); pos += block) {
            float* out[]{result.getWritePointer(0, pos), result.getWritePointer(1, pos)};
            device.process(nullptr, 0, out, 2, std::min(block, result.getNumSamples() - pos));
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
            const float delta = std::abs(a.getSample(ch,i) - b.getSample(ch,i));
            if (!std::isfinite(delta)) return std::numeric_limits<float>::infinity();
            peak = std::max(peak, delta);
        }
    return peak;
}
TEST_CASE("A-05 two independent stereo guitar amps and drums match the offline WAV including tails") {
    Session s;
    auto file = s.dir.getChildFile("mix.wav");
    MemoryBlock first;
    REQUIRE(renderSong(s.song, s.library, file));
    REQUIRE(file.loadFileAsData(first));
    REQUIRE(renderSong(s.song, s.library, file));
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
TEST_CASE("A-20 recalls and embedded user settings reproduce guitar sound independently of the library") {
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
            float output = reference.process(s.source.getSample(ch,i));
            if (i >= reference.delaySamples())
                error = std::max(error, std::abs(played.getSample(ch,i - reference.delaySamples()) - output));
        }
        REQUIRE(error <= 0.000001f);
    }
}
TEST_CASE("M-05 trial covers ten minutes of sixteen tracks with eight complete amp chains") {
    Session s;
    Song trial;
    auto path = s.dir.getChildFile("trial");
    REQUIRE(makeLoadTrial(trial, path, 8000, 8));
    REQUIRE(trial.end() == 600 * 8000);
    REQUIRE(trial.tracks.size() == 16);
    CHECK(std::count_if(trial.tracks.begin(), trial.tracks.end(), [](auto& t){return t.amp.enabled;}) == 8);
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

}
