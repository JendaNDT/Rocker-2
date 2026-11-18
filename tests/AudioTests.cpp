#include "FakeDevice.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <numbers>
using namespace rocker;
using test::FakeDevice;
TEST_CASE("Meters read sample peaks, hold for 1.5 s and latch clipping per input") {
    FakeDevice fake;
    Connection link(fake); link.rescan({});
    auto feed=[&](float a,float b) {fake.process({{a},{b}},1);};
    feed(-0.5f,0);
    CHECK(decibels(link.audio.read(0).peak)==Catch::Approx(-6.0206));
    CHECK(std::isinf(decibels(link.audio.read(1).peak)));
    fake.process({},71999);
    CHECK(link.audio.read(0).held==0.5f);
    feed(0,0);
    CHECK(link.audio.read(0).held==0);
    feed(0.9885f,0);
    CHECK_FALSE(link.audio.read(0).clipped);
    feed(-std::pow(10.0f,-0.1f/20.0f),0);
    fake.process({},100000);
    CHECK(link.audio.read(0).clipped);
    CHECK_FALSE(link.audio.read(1).clipped);
    link.audio.clearClip(0);
    CHECK_FALSE(link.audio.read(0).clipped);
}
TEST_CASE("Tone matches the 440 Hz reference across every segment and block boundary") {
    for (int rate : {44100,48000,96000}) {
        FakeDevice fake; fake.rate=rate; fake.outputs=4;
        Connection link(fake); link.rescan({}); link.audio.playTone();
        double maxError=0;
        for (int pos=0;pos<3*rate+257;pos+=257) {
            auto out=fake.process({},257);
            for (int j=0;j<257;++j) for (int c=0;c<4;++c) {
                const int t=pos+j;
                const bool on=t<3*rate && ((c==0 && (t<rate || t>=2*rate)) || (c==1 && t>=rate));
                const double expected=on?std::pow(10.,-18./20.)*std::sin(2*std::numbers::pi*440*t/rate):0;
                maxError=std::max(maxError,std::abs(expected-out[c][j]));
            }
        }
        CHECK(maxError<1e-7);
        link.audio.playTone(); fake.process({},100);
        link.audio.stopTone();
        CHECK(fake.process({},1)[0][0]==0);
    }
}
