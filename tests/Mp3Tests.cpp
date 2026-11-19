#include "engine/Export.h"
#include <catch2/catch_test_macros.hpp>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_STDIO
#define MINIMP3_NO_SIMD
#include <minimp3_ex.h>

TEST_CASE("A-12 native MP3 at 320 kbps delivers the song length and both ends of the audio") {
    using namespace juce;
    using namespace rocker;
    if (!mp3Available())
        SKIP("Systém nemá stereo kodér MP3 320 kb/s. Poslech po vydání ověří M-13.");
    auto root =
        File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("Rocker2-mp3", "", false);
    struct Cleanup {
        File file;
        ~Cleanup() { file.deleteRecursively(); }
    } cleanup{root};
    Song song;
    song.title = "A12";
    song.grid = {44100, 4, 4, 120};
    song.bars = 1;
    song.tracks = {{"s", "Tone", "jine"}};
    song.tracks[0].pan = -1;
    for (int length : {86400, 86617}) {
        auto path = root.getChildFile("Zvuk/reference.wav");
        REQUIRE(path.getParentDirectory().createDirectory().wasOk());
        AudioBuffer<float> signal(1, length);
        for (int i = 0; i < length; ++i) {
            bool sounding = i < 4096 || i >= length - 4096;
            signal.setSample(
                0, i, sounding ? .4f * std::sin(i * 2 * MathConstants<double>::pi * 440 / 44100) : 0.f);
        }
        path.deleteFile();
        auto writer = waveWriter(path, 44100, 1);
        REQUIRE(writer);
        REQUIRE(writer->writeFromAudioSampleBuffer(signal, 0, length));
        writer.reset();
        song.assets = {{"a", "Zvuk/reference.wav", length, 1}};
        song.clips = {{"c", "s", "a", 0, 0, length}};
        AudioLibrary sources;
        REQUIRE(sources.load(song, root));
        auto file = root.getChildFile("mix.mp3");
        REQUIRE(exportMix(song, sources, file, Delivery::mp3, 0, length).ok);
        auto native = decodedMp3Frames(file);
        REQUIRE(native);
        CHECK(*native >= length - 1152);
        MemoryBlock bytes;
        REQUIRE(file.loadFileAsData(bytes));
        mp3dec_t decoder{};
        mp3dec_file_info_t info{};
        REQUIRE(mp3dec_load_buf(&decoder, (const uint8_t*)bytes.getData(), bytes.getSize(), &info,
                                nullptr, nullptr) == 0);
        std::unique_ptr<mp3d_sample_t, decltype(&std::free)> samples(info.buffer, std::free);
        REQUIRE(samples);
        REQUIRE(info.channels == 2);
        CHECK(info.hz == 44100);
        CHECK(info.avg_bitrate_kbps == 320);
        REQUIRE(std::abs(Frame(info.samples / 2) - length) <= 1);
        auto rms = [&](int from, int to, int ch) {
            double energy = 0;
            for (int i = from; i < to; ++i) {
                double v = samples.get()[2 * i + ch] / 32768.;
                energy += v * v;
            }
            return std::sqrt(energy / (to - from));
        };
        CHECK(rms(32, 544, 0) > .12);
        CHECK(rms(length - 544, length - 32, 0) > .12);
        CHECK(rms(length / 2, length / 2 + 512, 0) < .001);
        CHECK(rms(32, 544, 1) < .001);
    }
}

#if JUCE_MAC
TEST_CASE("A-28 native AAC delivers both edges and the exact editorial length") {
    using namespace juce;
    using namespace rocker;
    auto root = File::getSpecialLocation(File::tempDirectory).getChildFile("Rocker2-AAC-" + Uuid().toString());
    struct Cleanup {
        File dir;
        ~Cleanup() { dir.deleteRecursively(); }
    } cleanup{root};
    auto wav = root.getChildFile("Zvuk/reference.wav");
    REQUIRE(wav.getParentDirectory().createDirectory().wasOk());
    constexpr int rate = 44100;
    constexpr int length = rate * 2 - 31;
    auto writer = waveWriter(wav, rate, 1);
    REQUIRE(writer);
    AudioBuffer<float> tone(1, length);
    tone.clear();
    for (int i = 0; i < length; ++i)
        if (i < 2048 || i >= length - 2048)
            tone.setSample(0, i, float(.4 * std::sin(i * MathConstants<double>::twoPi * 440 / rate)));
    REQUIRE(writer->writeFromAudioSampleBuffer(tone, 0, length));
    writer.reset();
    Song song;
    song.title = "A28";
    song.grid.rate = rate;
    song.bars = 1;
    song.tracks = {{"t", "Tone", "jine"}};
    song.tracks.front().pan = -1;
    song.assets = {{"a", "Zvuk/reference.wav", length, 1}};
    song.clips = {{"c", "t", "a", 0, 0, length}};
    AudioLibrary sources;
    REQUIRE(sources.load(song, root));
    auto file = root.getChildFile("Univerzální poslech.m4a");
    auto encoded = exportMix(song, sources, file, Delivery::m4a, 0, length);
    INFO(encoded.detail.toStdString());
    REQUIRE(encoded.ok);
    FileInputStream bytes(file);
    REQUIRE(bytes.openedOk());
    REQUIRE(bytes.setPosition(4));
    std::array<char, 4> tag{};
    REQUIRE(bytes.read(tag.data(), 4) == 4);
    CHECK(String::fromUTF8(tag.data(), 4) == "ftyp");
    AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> decoded(formats.createReaderFor(file));
    REQUIRE(decoded);
    CHECK(decoded->sampleRate == rate);
    CHECK(decoded->numChannels == 2);
    REQUIRE(decoded->lengthInSamples == length);
    AudioBuffer<float> left(2, 512);
    for (auto offset : {64, length - 576}) {
        REQUIRE(decoded->read(&left, 0, 512, offset, true, true));
        CHECK(left.getRMSLevel(0, 0, 512) > .1);
        CHECK(left.getRMSLevel(1, 0, 512) < .02);
    }
    REQUIRE(decoded->read(&left, 0, 512, length / 2, true, true));
    CHECK(left.getRMSLevel(0, 0, 512) < .001);
}
#endif
