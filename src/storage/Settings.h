#pragma once
#include "engine/Devices.h"
#include <map>
namespace rocker {
struct Settings {
    DeviceChoice device;
    bool visited = false;
    juce::String laptopInput;
    std::map<juce::String, juce::String> aliases;
    static juce::File location();
    static Settings load(const juce::File&);
    bool save(const juce::File&) const;
    static juce::String inputKey(const DeviceChoice&, int channel);
};
} // namespace rocker
