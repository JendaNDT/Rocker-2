#include "Amp.h"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace rocker {
namespace {
constexpr double pi=std::numbers::pi;
double sinc(double x) { return std::abs(x)<1e-10 ? 1.0 : std::sin(pi*x)/(pi*x); }
template<size_t N>
float fir(float x,std::array<float,N>& line,int& head,const std::array<float,N>& kernel) noexcept {
    line[head]=x;
    float out=0;
    int pos=head;
    for (float tap:kernel) {
        out+=tap*line[pos];
        if (--pos<0) pos=N-1;
    }
    if (++head==N) head=0;
    return out;
}
}
void Amp::prepare(int sampleRate) {
    rate=sampleRate;
    auto pole=[&](float hz) { return static_cast<float>(1-std::exp(-2*pi*hz/rate)); };
    lowPole=pole(220); midPole=pole(1450); airPole=pole(3200);
    attack=1-std::exp(-1.f/(rate*0.002f)); release=std::exp(-1.f/(rate*0.040f));
    float sum=0;
    for (int i=0;i<15;++i) {
        filter[i]=static_cast<float>(0.5*sinc((i-7)*0.5)*(0.54-0.46*std::cos(2*pi*i/14)));
        sum+=filter[i];
    }
    for (auto& tap:filter) tap/=sum;
    // Original windowed-sinc cabinet IR, designed at the actual device rate.
    // Band-limited speaker with a midrange peak and a softer upper midrange.
    const int half=std::max(8,static_cast<int>(std::round(rate*0.0015)));
    cabinet.resize(2*half+1); history.resize(cabinet.size());
    for (int i=-half;i<=half;++i) {
        auto lp=[&](double hz) { const auto f=2*hz/rate; return f*sinc(i*f); };
        const auto window=0.42+0.5*std::cos(pi*i/half)+0.08*std::cos(2*pi*i/half);
        cabinet[i+half]=static_cast<float>(window*(lp(4200)-lp(100)+0.32*(lp(1550)-lp(700))-0.25*(lp(3400)-lp(2300))));
    }
    configure(params); reset();
}
void Amp::configure(AmpParameters p) {
    params=p;
    gain=std::pow(10.f,p.drive*0.015f);
    level=0.75f*std::pow(p.volume/100.f,1.6f);
    threshold=std::pow(10.f,-p.gate/20.f);
    auto eq=[](int n) {return std::pow(10.f,(n-50)*0.012f);};
    bassGain=eq(p.bass); midGain=eq(p.mid); trebleGain=eq(p.treble); presenceGain=eq(p.presence)-1;
}
void Amp::reset() noexcept {
    up.fill(0); down.fill(0); std::fill(history.begin(),history.end(),0);
    upHead=downHead=head=0; env=gate=low=wide=air=0;
}
float Amp::oversample(float sample,int shape) noexcept {
    const float x=gain*fir(2*sample,up,upHead,filter);
    float wet=0;
    switch(shape) {
        case 0: wet=std::tanh(x); break;
        case 1: wet=std::tanh(x+0.22f)-0.21651806f; break;
        default: wet=std::clamp(x,-0.8f,0.8f); break;
    }
    return fir(wet,down,downHead,filter);
}
float Amp::process(float x) noexcept {
    env=std::max(std::abs(x),env*release);
    const float target=env>=threshold ? 1.f : 0.f;
    gate+=(target-gate)*(target>gate?attack:1-release);
    const float wet=oversample(x*gate,params.shape);
    oversample(0,params.shape);
    low+=lowPole*(wet-low); wide+=midPole*(wet-wide); air+=airPole*(wet-air);
    history[head]=bassGain*low+midGain*(wide-low)+trebleGain*(wet-wide)+presenceGain*(wet-air);
    float result=0; int pos=head;
    for (float tap:cabinet) {
        result+=tap*history[pos];
        if (--pos<0) pos=static_cast<int>(history.size())-1;
    }
    if (++head==static_cast<int>(history.size())) head=0;
    return std::clamp(result*level,-1.f,1.f);
}
}
