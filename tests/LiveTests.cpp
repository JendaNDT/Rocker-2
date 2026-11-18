#include "FakeDevice.h"
#include "dsp/Tunings.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>
#include <random>
using namespace rocker;
using test::FakeDevice;

TEST_CASE("A-16 tuner tracks B1 and E2 through E4 within the cent limits") {
    for (int rate : {44100, 48000, 96000}) {
        FakeDevice fake;
        fake.rate = rate;
        Connection link(fake);
        link.rescan({});
        auto& live = link.audio.live;
        for (bool noisy : {false, true})
            for (int note = 35; note <= 64; ++note) {
                const double hz = Pitch::frequency(note) * std::exp2(7.5 / 1200);
                live.setTuner(true);
                std::mt19937 rng(17);
                std::uniform_real_distribution<float> noise(-0.022f, 0.022f);
                double result = 0;
                for (int pos = 0; pos < rate / 4 && result == 0; pos += 257) {
                    FakeDevice::Block input(1, std::vector<float>(257));
                    for (int i = 0; i < 257; ++i) {
                        const auto phase = 2 * std::numbers::pi * hz * (pos + i) / rate;
                        input[0][i] = 0.36f * std::sin(phase);
                        if (noisy)
                            input[0][i] +=
                                0.32 * std::sin(2 * phase) + 0.15 * std::sin(3 * phase) + noise(rng);
                    }
                    fake.process(input, 257);
                    if (auto pitch = live.takePitch())
                        result = *pitch;
                }
                CAPTURE(rate, note, noisy, result);
                REQUIRE(result > 0);
                CHECK(std::abs(1200 * std::log2(result / hz)) <= (noisy ? 3. : 1.));
            }
    }
    std::array<float, 1024> silence{};
    CHECK(Pitch{}.detect(silence, 12000) == 0);
}
TEST_CASE("A-20 repeatable amp, cabinet, preset switching and block independence") {
    FakeDevice fake;
    Connection link(fake);
    link.rescan({});
    auto& live = link.audio.live;
    live.monitor = Monitor::amp;
    auto play = [&](int preset, int block) {
        live.setAmp(presets[preset]);
        std::vector<float> result;
        for (int pos = 0; pos < 8192; pos += block) {
            int n = std::min(block, 8192 - pos);
            FakeDevice::Block input(1, std::vector<float>(n));
            for (int i = 0; i < n; ++i)
                input[0][i] = 0.42 * std::sin((pos + i) * 0.019) + 0.18 * std::sin((pos + i) * 0.043);
            auto out = fake.process(input, n);
            REQUIRE(out[0] == out[1]);
            result.insert(result.end(), out[0].begin(), out[0].end());
        }
        return result;
    };
    auto crunch = play(1, 128);
    auto metal = play(3, 128);
    CHECK(*std::max_element(crunch.begin(), crunch.end()) > 0.05f);
    CHECK(metal != crunch);
    CHECK(play(1, 257) == crunch);
    auto clean = play(0, 64);
    auto lead = play(2, 256);
    CHECK(clean != lead);
    CHECK(lead != metal);
    CHECK(play(1, 1024) == crunch);
}
TEST_CASE("Live routing separates inputs, cues and noise measurement") {
    FakeDevice fake;
    fake.outputs = 4;
    Connection link(fake);
    link.rescan({});
    auto& live = link.audio.live;
    FakeDevice::Block input{{0.5f, 0.2f}, {-0.1f, -0.3f}};
    CHECK(fake.process(input, 2)[0] == std::vector<float>(2, 0));
    live.selectInput(1);
    live.monitor = Monitor::clean;
    auto out = fake.process(input, 2);
    CHECK(out[0] == input[1]);
    CHECK(out[1] == input[1]);
    CHECK(out[2] == std::vector<float>(2, 0));
    live.silence();
    live.cue(440);
    out = fake.process({}, 2400);
    CHECK(out[0] == out[1]);
    CHECK(*std::max_element(out[0].begin(), out[0].end()) > 0.09f);
    CHECK(link.audio.read(0).peak == 0);
    live.cue(880, true);
    fake.process({}, 6000);
    CHECK(fake.process({}, 16)[0] == std::vector<float>(16, 0));
    live.learnNoise();
    fake.process({std::vector<float>(95999, 0.9f), std::vector<float>(95999, 0.001f)}, 95999);
    CHECK_FALSE(live.takeThreshold());
    fake.process({{0}, {0}}, 1);
    REQUIRE(live.takeThreshold() == 54);
}
TEST_CASE("Cable measures an inverted, attenuated return and rejects an unplugged cable") {
    for (int rate : {44100, 48000, 96000}) {
        FakeDevice fake;
        fake.rate = rate;
        Connection link(fake);
        link.rescan({});
        auto& live = link.audio.live;
        live.selectInput(1);
        for (int trial = 0; trial < 5; ++trial) {
            live.monitor = Monitor::amp;
            REQUIRE(live.startMeasurement());
            const int delay = 373;
            std::vector<float> wire(delay, 0);
            for (int pos = 0; live.measurement() != Measurement::ready && pos < rate; pos += 64) {
                FakeDevice::Block in(2, std::vector<float>(64));
                for (int i = 0; i < 64; ++i)
                    in[1][i] = -0.3f * wire[(pos + i) % delay];
                auto out = fake.process(in, 64);
                for (int i = 0; i < 64; ++i)
                    wire[(pos + i) % delay] = out[0][i];
                REQUIRE(out[1] == std::vector<float>(64, 0));
            }
            auto result = live.takeMeasurement();
            REQUIRE(result);
            CHECK(result->samples == delay);
            CHECK(live.monitor == Monitor::off);
        }
        REQUIRE(live.startMeasurement());
        fake.process({}, rate);
        auto missing = live.takeMeasurement();
        REQUIRE(missing);
        CHECK(missing->samples < 0);
    }
}
TEST_CASE("Factory and custom tunings keep string order and reject malformed notes") {
    auto list = Tuning::factory();
    REQUIRE(list.size() == 9);
    CHECK(list[1].notes == Tuning::parse("D2 A2 D3 G3 B3 E4"));
    CHECK(list[3].notes.front() == 35);
    CHECK(Tuning::parse("Eb2 H3 F#4") == std::vector<int>{39, 59, 66});
    CHECK(Tuning::parse("E2 hi A2").empty());
    CHECK(Tuning::parse("A0").empty());
    CHECK(list[3].nearest(Pitch::frequency(35)) == 0);
}
