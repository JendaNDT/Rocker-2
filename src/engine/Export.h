#pragma once
#include "Timeline.h"
#include "Loudness.h"

namespace rocker {
enum class Delivery { wave24, wave16, mp3, m4a, stems };
struct MixFile {
    juce::File file;
    Playback::Meter meter;
    bool ok = false;
    Frame begin = 0;
    juce::String detail;
    std::optional<LoudnessReading> loudness;
};
// A range ends at its exclusive boundary. Whole-song delivery includes the effect tail.
MixFile exportMix(const Song&, const AudioLibrary&, const juce::File&, Delivery, Frame begin = 0,
                  Frame end = -1, bool louder = false);
MixFile exportStems(const Song&, const AudioLibrary&, const juce::File& destination,
                    const juce::String& drumsName = "Drums");
bool tagMp3Length(const juce::File&, Frame samples);
bool mp3Available();
bool encodeAac(const juce::File& pcm, const juce::File& destination, juce::String& detail);
bool deliveryAvailable(Delivery);
bool encodeMp3(const juce::File& pcm, const juce::File& destination);
std::optional<Frame> decodedMp3Frames(const juce::File&);
std::optional<bool> containsClick(const juce::File&, const BeatGrid&, Frame start = 0);
} // namespace rocker
