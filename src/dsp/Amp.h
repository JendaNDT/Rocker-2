#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace rocker {
// Percent controls. Gate stores the magnitude of a negative dBFS threshold.
struct AmpParameters {
    std::uint8_t shape, drive, bass, mid, treble, presence, volume, gate;
    bool operator==(const AmpParameters&) const = default;
};
inline constexpr std::array<AmpParameters,4> presets{{
    {0,8,52,50,56,34,68,68}, {0,48,56,62,54,47,60,60},
    {1,72,51,70,57,52,54,55}, {2,90,64,38,61,58,48,50}
}};
class Amp {
public:
    void prepare(int sampleRate);
    void configure(AmpParameters);
    void reset() noexcept;
    float process(float) noexcept;
    int delaySamples() const noexcept { return static_cast<int>(cabinet.size()/2)+3; }
private:
    using Branch = std::array<float,3>;
    Branch upEven{},upOdd{},downEven{},downOdd{};
    float previousOdd=0;
    int head=0,rate=48000;
    std::vector<float> cabinet, history;
    AmpParameters params=presets[0];
    float gain=1,level=1,threshold=0,env=0,gate=0,release=0,attack=0;
    float low=0,wide=0,air=0,lowPole=0,midPole=0,airPole=0;
    float bassGain=1,midGain=1,trebleGain=1,presenceGain=0;
};
}
