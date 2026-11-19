#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <optional>

namespace rocker {
struct LoudnessReading {
    double integrated;
    double peak;
};
std::optional<LoudnessReading> measureLoudness(juce::AudioFormatReader&);
bool writeLimitedMix(juce::AudioFormatReader&, const juce::File&, double gain, double ceiling);
} // namespace rocker
