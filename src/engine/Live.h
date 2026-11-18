#pragma once
#include "dsp/Amp.h"
#include "dsp/Pitch.h"
#include <atomic>
#include <bit>
#include <optional>
#include <vector>

namespace rocker {
enum class Monitor { off, clean, amp };
enum class Measurement { idle, requested, recording, ready };
struct LatencyResult {
    int samples;
    float confidence;
};
class Live {
  public:
    void prepare(int sampleRate);
    void render(const float* const* in, int ni, float* const* out, int no, int frames) noexcept;
    void setAmp(AmpParameters p, bool recall = true) {
        parameters.store(std::bit_cast<std::uint64_t>(p));
        if (recall)
            ++recallCount;
    }
    void selectInput(int index);
    void setTuner(bool active);
    void cue(double frequency, bool confirmation = false);
    void endCue() { cueCommand = -2; }
    void silence();
    void learnNoise();
    std::optional<int> takeThreshold();
    std::optional<double> takePitch();
    double pitchFrameSeconds() const { return pitchFrame.size() * stride / static_cast<double>(rate); }
    bool startMeasurement();
    std::optional<LatencyResult> takeMeasurement();
    Measurement measurement() const { return phase.load(); }
    int ampDelay() const { return amp.delaySamples(); }
    std::atomic<Monitor> monitor{Monitor::off};
    std::atomic<int> input{0};
    std::atomic<float> outputPeak{0}, cpuPercent{0};

  private:
    int rate = 48000, stride = 4;
    Amp amp;
    std::atomic<std::uint64_t> parameters{std::bit_cast<std::uint64_t>(presets[1])};
    std::uint64_t applied = 0;
    std::atomic<unsigned> recallCount{0};
    unsigned lastRecall = 0;
    Monitor previousMode = Monitor::off;
    std::atomic<int> cueCommand{0};
    bool reference = false, releasing = false;
    int cueLeft = 0, cueLength = 0;
    double angle = 0, step = 0;
    std::atomic<bool> tuning{false};
    std::atomic<unsigned> generation{0};
    unsigned writingGeneration = 0, publishedGeneration = 0;
    std::array<float, 1024> pitchFrame{}, mailbox{};
    std::atomic<bool> pitchReady{false};
    int samplesInGroup = 0, pitchIndex = 0;
    float groupSum = 0;
    void collectPitch(float x) noexcept;
    std::atomic<int> noiseCommand{0}, learned{-1};
    int noiseLeft = 0;
    float noisePeak = 0;
    std::atomic<Measurement> phase{Measurement::idle};
    std::atomic<bool> cancel{false};
    std::array<float, 508> probe{};
    std::vector<float> response;
    int cursor = 0;
};
} // namespace rocker
