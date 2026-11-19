#include "Loudness.h"
namespace rocker {
std::optional<LoudnessReading> measureLoudness(juce::AudioFormatReader&) { return {}; }
bool writeLimitedMix(juce::AudioFormatReader&, const juce::File&, double, double) { return false; }
}
