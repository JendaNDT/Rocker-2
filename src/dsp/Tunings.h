#pragma once
#include <juce_core/juce_core.h>
#include <vector>
namespace rocker {
struct Tuning {
    juce::String name;
    std::vector<int> notes;
    static std::vector<Tuning> factory();
    static std::vector<int> parse(const juce::String&);
    static juce::String noteName(int note);
    int nearest(double hz) const;
};
}
