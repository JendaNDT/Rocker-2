#pragma once
#include "song/Song.h"
#include <atomic>
#include <array>
#include <span>

namespace rocker {
struct Hit {
    Frame frame = 0;
    int note = 0, velocity = 0;
    bool operator==(const Hit&) const = default;
};
class HitQueue {
  public:
    bool push(Hit h) noexcept;
    bool peek(Hit& h) const noexcept;
    void pop() noexcept;

  private:
    static constexpr unsigned capacity = 8192;
    std::array<Hit, capacity> data{};
    std::atomic<unsigned> write{0}, read{0};
};
class DrumPlayer {
  public:
    void prepare(int rate);
    void reset() noexcept;
    void trigger(int kit, int note, int velocity) noexcept;
    float sample() noexcept;
    static constexpr std::array<int, 8> pads{36, 38, 42, 46, 45, 48, 49, 51};

  private:
    struct Voice {
        const float* wave = nullptr;
        int remaining = 0;
        float gain = 0;
    };
    std::array<Voice, 32> voices{};
    std::array<std::array<std::vector<float>, 8>, 2> samples;
    unsigned next = 0;
};
class DrumInput {
  public:
    void prepare(int rate);
    bool push(Hit) noexcept; // Frame on the device's continuous sample clock.
    bool native(double seconds, int note, int velocity) noexcept;
    std::span<const Hit> block(int frames, double startSeconds) noexcept;
    void render(float* const*, int, int, std::span<const Hit>) noexcept;
    Frame position = 0;
    std::atomic<int> kit{0}, lastNote{-1};
    std::atomic<bool> monitor{false}, overflow{false};
    std::array<std::atomic<int>, 128> mapping{};

  private:
    HitQueue queue;
    DrumPlayer player;
    std::array<Hit, 512> hits;
    int rate = 48000;
};
bool saveMidi(const juce::File&, int rate, Frame length, std::span<const Hit>);
std::optional<std::vector<Hit>> readMidi(const juce::File&, int rate);
std::optional<int> compareSnare(std::span<const Hit>, juce::AudioFormatReader&, Frame midiOffset,
                                Frame audioOffset);
bool renderMidi(const juce::File& midi, const juce::File& wave, int kit, int rate, Frame length);
} // namespace rocker
