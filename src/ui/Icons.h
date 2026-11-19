#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace rocker {
enum class Icon {
    none, home, ideas, playMode, editMode, mixMode, metronome, tuner, device,
    play, stop, start, distant, hideInspector, showInspector, minus, plus,
    ok, up, down, exportMix
};

void paintIcon(juce::Graphics&, Icon, juce::Colour, juce::Rectangle<float>);
} // namespace rocker
