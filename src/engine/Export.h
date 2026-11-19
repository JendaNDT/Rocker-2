#pragma once
#include "Timeline.h"

namespace rocker {
enum class Delivery { wave24, wave16, mp3, m4a };
struct MixFile {
    juce::File file;
    Playback::Meter meter;
    bool ok = false;
    Frame begin = 0;
};
// A range ends at its exclusive boundary. Whole-song delivery includes the effect tail.
MixFile exportMix(const Song&, const AudioLibrary&, const juce::File&, Delivery, Frame begin = 0,
                  Frame end = -1);
bool tagMp3Length(const juce::File&, Frame samples);
bool mp3Available();
bool encodeMp3(const juce::File& pcm, const juce::File& destination);
std::optional<Frame> decodedMp3Frames(const juce::File&);
std::optional<bool> containsClick(const juce::File&, const BeatGrid&, Frame start = 0);
} // namespace rocker
