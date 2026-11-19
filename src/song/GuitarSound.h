#pragma once
#include "dsp/Amp.h"
#include <juce_core/juce_core.h>

namespace rocker {
struct GuitarSound {
    bool operator==(const GuitarSound&) const = default;
    juce::String name = "Crunch";
    AmpParameters values = presets[1];
    bool enabled = false, modified = false;

    juce::var json() const;
    static GuitarSound read(const juce::var&);
};
} // namespace rocker
