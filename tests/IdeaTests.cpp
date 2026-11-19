#include "engine/Audio.h"
#include "engine/Import.h"
#include "storage/Ideas.h"
#include <catch2/catch_test_macros.hpp>
#include <numbers>

namespace {
using namespace rocker;
using namespace juce;
struct Temp {
    File dir =
        File::getSpecialLocation(File::tempDirectory).getChildFile("Rocker2-idea-" + Uuid().toString());
    Temp() { REQUIRE(dir.createDirectory().wasOk()); }
    ~Temp() { dir.deleteRecursively(); }
};
TEST_CASE("A-18 three minutes of input retain exactly the last two minutes and a new take has pre-roll") {
    Temp temp;
    constexpr int rate = 44100;
    auto input = std::make_shared<IdeaInput>(rate);
    std::array<float, 509> block;
    auto value = [](Frame i) { return float((i * 17) % 4093) / 4096.f; };
    Frame pos = 0;
    auto play = [&](Frame n) {
        Frame stop = pos + n;
        while (pos < stop) {
            int count = int(std::min<Frame>(block.size(), stop - pos));
            for (int i = 0; i < count; ++i)
                block[i] = value(pos + i);
            input->push(block.data(), count);
            pos += count;
        }
    };
    play(180 * rate);
    auto file = temp.dir.getChildFile("past.wav");
    IdeaRecording past(input, file, true);
    REQUIRE(past.valid());
    REQUIRE(past.finish());
    auto reader = waveReader(file);
    REQUIRE(reader);
    REQUIRE(reader->lengthInSamples == 120 * rate);
    AudioBuffer<float> actual(1, 2048);
    Frame errors = 0;
    for (Frame at = 0; at < reader->lengthInSamples; at += 2048) {
        int n = int(std::min<Frame>(2048, reader->lengthInSamples - at));
        REQUIRE(reader->read(&actual, 0, n, at, true, false));
        for (int i = 0; i < n; ++i)
            errors += actual.getSample(0, i) != value(60 * rate + at + i);
    }
    CHECK(errors == 0);
    Ideas ideas(temp.dir);
    Idea x;
    x.id = Uuid().toString();
    x.tuning = "Drop D";
    x.created = 12345;
    IdeaRecording take(input, ideas.wave(x.id), false);
    play(rate);
    take.requestStop();
    play(rate);
    REQUIRE(take.finish());
    REQUIRE(ideas.append(x));
    auto saved = ideas.all().front();
    REQUIRE(saved.length == 3 * rate);
    CHECK(saved.peaks.size() == 96);
    auto recorded = waveReader(ideas.wave(x.id));
    REQUIRE(recorded);
    REQUIRE(recorded->read(&actual, 0, 1, 0, true, false));
    CHECK(actual.getSample(0, 0) == value(178 * rate));
    saved.star = true;
    saved.note = String::fromUTF8("Sloka na zítra");
    saved.bpm = 120;
    saved.firstBeat = 12000;
    REQUIRE(ideas.update(saved));
    Ideas next(temp.dir);
    REQUIRE(next.all().size() == 1);
    CHECK(next.all()[0].star);
    CHECK(next.all()[0].note == saved.note);
    auto song = next.toSong(saved, temp.dir.getChildFile("Song"), "Idea", "Guitar");
    REQUIRE(song);
    CHECK(song->tuning == "Drop D");
    CHECK((song->clips.front().position + saved.firstBeat) % song->barFrame(1) == 0);
    MemoryBlock a, b;
    REQUIRE(ideas.wave(x.id).loadFileAsData(a));
    REQUIRE(temp.dir.getChildFile("Song/" + song->assets[0].path).loadFileAsData(b));
    CHECK(a == b);
    Audio device;
    device.prepare(1, rate);
    device.setInsurance(false);
    input = device.ideaInput;
    CHECK_FALSE(input->hasHistory());
    CHECK(input->earliest() == input->end());
    play(3 * rate);
    CHECK(input->end() - input->earliest() == 2 * rate);
    device.setInsurance(true);
    input = device.ideaInput;
    CHECK(input->hasHistory());
    CHECK(input->earliest() == input->end());
}
TEST_CASE("Eight taps anchor a free riff on its first downbeat") {
    std::vector<Frame> taps;
    for (int i = 0; i < 8; ++i)
        taps.push_back(1452 + i * 24000);
    auto beat = measureTaps(taps, 48000);
    REQUIRE(beat);
    CHECK(beat->bpm == 120);
    CHECK(beat->firstBeat == 1452);
    taps[4] = 0;
    CHECK_FALSE(measureTaps(taps, 48000));
}
TEST_CASE("A-19 independent mono and stereo assets retain pitch and duration through import") {
    Temp t;
    AudioFormatManager formats;
    formats.registerBasicFormats();
    for (const auto& suffix : StringArray{".wav", ".aiff", ".flac", ".ogg", ".opus", ".mp3"}) {
        CAPTURE(suffix.toStdString());
        int rate = 44100, frames = 44100;
        auto file = t.dir.getChildFile("440Hz" + suffix);
        if (suffix == ".mp3") {
            file = File(ROCKER_FIXTURES).getChildFile("a19-tone.mp3");
            // The original CC0 synthetic sine fixture contains 41 MPEG-1 audio frames.
            frames = 41 * 1152;
        } else {
            if (suffix == ".opus") {
                rate = 48000;
                frames = 48000;
            }
            auto* format = formats.findFormatForFileExtension(suffix);
            REQUIRE(format);
            std::unique_ptr<OutputStream> out = file.createOutputStream();
            auto writer = format->createWriterFor(out, AudioFormatWriterOptions{}
                                                           .withSampleRate(rate)
                                                           .withBitsPerSample(suffix == ".opus" ? 32 : 16)
                                                           .withNumChannels(2));
            REQUIRE(writer);
            AudioBuffer<float> signal(2, frames);
            for (int i = 0; i < frames; ++i)
                for (int c = 0; c < 2; ++c)
                    signal.setSample(
                        c, i, float(.3 * std::sin(2 * std::numbers::pi * (c ? 660 : 440) * i / rate)));
            REQUIRE(writer->writeFromAudioSampleBuffer(signal, 0, frames));
        }
        String detail;
        auto imported = importAudio(file, t.dir, 48000, &detail);
        INFO(detail.toStdString());
        REQUIRE(imported);
        auto output = waveReader(t.dir.getChildFile(imported->path));
        REQUIRE(output);
        CHECK(output->sampleRate == 48000);
        CHECK(output->usesFloatingPointData);
        CHECK(std::abs(output->lengthInSamples - std::llround(frames * 48000. / rate)) <= 1);
        AudioBuffer<float> result(2, 24000);
        REQUIRE(output->read(&result, 0, 24000, 10000, true, true));
        for (int c = 0; c < (suffix == ".mp3" ? 1 : 2); ++c) {
            int count = 0;
            for (int i = 1; i < 24000; ++i)
                if (result.getSample(c, i) > 0 && result.getSample(c, i - 1) <= 0)
                    ++count;
            CHECK(std::abs(count * 2 - (c ? 660 : 440)) <= 2);
        }
    }
    CHECK_FALSE(importAudio(t.dir.getChildFile("broken.mp3"), t.dir, 48000));
}
} // namespace

TEST_CASE("Idea recovery reindexes a readable interrupted WAV exactly once") {
    Temp temp;
    Ideas catalog(temp.dir);
    auto id = Uuid().toString();
    auto file = catalog.wave(id);
    REQUIRE(file.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(file.getSiblingFile(id + ".partial.wav"), 44100, 1);
    REQUIRE(writer);
    AudioBuffer<float> material(1, 2048);
    material.clear();
    material.setSample(0, 120, .8);
    REQUIRE(writer->writeFromAudioSampleBuffer(material, 0, 2048));
    writer.reset();
    REQUIRE(temp.dir.getChildFile("napady.json").replaceWithText("{"));
    catalog.recover();
    catalog.recover();
    REQUIRE(catalog.all().size() == 1);
    CHECK(catalog.all()[0].id == id);
    CHECK(catalog.all()[0].length == 2048);
    CHECK(file.existsAsFile());
    CHECK(temp.dir.findChildFiles(File::findFiles, false, "napady-zaloha*.json").size() == 1);
}
