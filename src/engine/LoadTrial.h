#pragma once
#include "song/Song.h"

namespace rocker {
// Build the ten-minute M-05 arrangement outside the callback.
bool makeLoadTrial(Song&, const juce::File& directory, int rate, int guitarCount);
} // namespace rocker
