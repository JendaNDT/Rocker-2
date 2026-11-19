#include "Export.h"
#include "storage/AtomicFile.h"
#include <numbers>

namespace rocker {
using namespace juce;
namespace {
// Fractional-delay low-pass filter. The same phase is used for both channels.
constexpr int radius = 32, phases = 1024;
using Kernel = std::array<float, radius * 2>;
std::vector<Kernel> kernels(double cutoff) {
    std::vector<Kernel> table(phases);
    constexpr double pi = std::numbers::pi;
    for (int phase = 0; phase < phases; ++phase) {
        double sum = 0;
        for (int k = 0; k < radius * 2; ++k) {
            const double x = k - radius + 1 - double(phase) / phases;
            const double window = 0.42 + 0.5 * std::cos(pi*x/radius) + 0.08 * std::cos(2*pi*x/radius);
            sum += table[phase][k] = float(window * (x == 0 ? cutoff : std::sin(pi*cutoff*x)/(pi*x)));
        }
        for (auto& value : table[phase]) value /= float(sum);
    }
    return table;
}
struct Noise {
    uint32_t state = 42893;
    double next() {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return double(state) / 4294967296.;
    }
};
bool convert(AudioFormatReader& input, const File& path, int rate, int bits, Frame begin, Playback::Meter& meter) {
    std::unique_ptr<OutputStream> stream = path.createOutputStream();
    if (!stream) return false;
    auto writer = WavAudioFormat().createWriterFor(stream, AudioFormatWriterOptions{}.withSampleRate(rate)
        .withNumChannels(2).withBitsPerSample(bits));
    if (!writer) return false;
    const int sourceRate = roundToInt(input.sampleRate);
    const double ratio = double(sourceRate) / rate;
    const auto table = rate == sourceRate ? std::vector<Kernel>{} : kernels(std::min(1., 1./ratio));
    constexpr int block = 512;
    AudioBuffer<float> source(2, int(std::ceil(block*ratio))+radius*2+2);
    int pcm[2][block];
    const int* channels[]{pcm[0], pcm[1]};
    Noise noise;
    const int scale = 1 << (bits-1);
    const Frame frames = std::llround(input.lengthInSamples / ratio);
    for (Frame pos=0; pos<frames; pos+=block) {
        const int count = int(std::min<Frame>(block, frames-pos));
        const Frame first = pos*sourceRate/rate - radius + 1;
        if (!input.read(&source, 0, source.getNumSamples(), first, true, true)) return false;
        for (int i=0; i<count; ++i) {
            const Frame time = (pos+i)*sourceRate;
            int centre = int(time/rate-first);
            int phase = int((time%rate)*phases/rate);
            for (int ch=0; ch<2; ++ch) {
                const float* data=source.getReadPointer(ch);
                double sample=0;
                if (table.empty()) sample=data[centre];
                else for (int k=0; k<radius*2; ++k) sample += data[centre-radius+1+k]*table[phase][k];
                float level=float(std::abs(sample));
                if (level > meter.maximum) { meter.maximum=level; meter.loudest=begin+time/rate; }
                if (level > 1 && meter.over < 0) meter.over=begin+time/rate;
                const double a=bits==16 ? noise.next() : 0.;
                const double b=bits==16 ? noise.next() : 0.;
                auto quantized=std::llround(std::clamp(sample,-1.,1.)*scale+a-b);
                pcm[ch][i] = int(std::clamp(quantized,-(long long)scale,(long long)scale-1)) * (1 << (32-bits));
            }
        }
        if (!writer->write(channels,count)) return false;
    }
    return writer->flush();
}
}
MixFile exportMix(const Song& song,const AudioLibrary& lib,const File& dest, Delivery format,Frame begin,Frame end) {
    MixFile result{dest};
    if (dest.getParentDirectory().createDirectory().failed() || (format==Delivery::mp3 && !mp3Available())) return result;
    TemporaryFile stage(".wav"), pcm(".wav"), output(dest);
    if (!renderSong(song,lib,stage.getFile(),begin,end)) return result;
    auto input=waveReader(stage.getFile());
    if (!input) return result;
    const bool mp3 = format==Delivery::mp3;
    if (!convert(*input,mp3 ? pcm.getFile() : output.getFile(),format==Delivery::wave24 ? 48000:44100,
                 format==Delivery::wave24 ? 24:16,begin,result.meter)) return result;
    if (mp3 && !encodeMp3(pcm.getFile(),output.getFile())) return result;
    result.ok=commitTemporary(output);
    return result;
}
std::optional<bool> containsClick(const File& file, const BeatGrid& grid, Frame start) {
    auto input=waveReader(file);
    if(!input || input->numChannels==0) return {};
    BeatGrid clock=grid;
    clock.rate=roundToInt(input->sampleRate);
    Frame origin=std::llround(double(start)*clock.rate/grid.rate);
    int n=clock.rate/80;
    AudioBuffer<float> samples(2,n);
    int hits=0;
    for (Frame beat=clock.beatAt(origin);clock.beat(beat)-origin+n<=input->lengthInSamples;++beat) {
        auto pos=clock.beat(beat)-origin;
        if(pos<0) continue;
        if(!input->read(&samples,0,n,pos,true,true)) return {};
        double match=0,energy=0,reference=0;
        for(int i=0;i<n;++i) {
            double expected=std::exp(-i/(clock.rate*.0025))*std::cos(2*std::numbers::pi*(beat%clock.beats==0 ? 1600:1000)*i/clock.rate);
            double actual=.5*(samples.getSample(0,i)+samples.getSample(1,i));
            match+=expected*actual; energy+=actual*actual; reference+=expected*expected;
        }
        if(match>.035*reference && match*match>.65*reference*energy && ++hits>=2) return true;
    }
    return false;
}
}
