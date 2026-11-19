#pragma once
#include <juce_core/juce_core.h>

namespace rocker {
bool commitTemporary(const juce::TemporaryFile&);
bool atomicJson(const juce::File&, const juce::var&);
} // namespace rocker
