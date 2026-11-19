#pragma once
#include "song/Song.h"
namespace rocker {
bool supportsImport(const juce::File&);
std::optional<Asset> importAudio(const juce::File& input, const juce::File& project, int rate, juce::String* detail = nullptr);
bool decodeSystemAudio(const juce::File&, const juce::File& temporaryWave);
} // namespace rocker
