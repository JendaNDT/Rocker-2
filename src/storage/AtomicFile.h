#pragma once
#include <juce_core/juce_core.h>

namespace rocker {
bool commitTemporary(const juce::TemporaryFile&);
bool moveNewDirectory(const juce::File& source, const juce::File& destination);
bool atomicJson(const juce::File&, const juce::var&);
} // namespace rocker
