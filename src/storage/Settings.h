#pragma once
#include "engine/Devices.h"
#include <map>
#include "song/GuitarSound.h"
#include "dsp/Tunings.h"
namespace rocker {
struct Calibration {
    int samples = 0, reported = 0;
    int correction() const { return samples - reported; }
};
struct DrumSetup {
    juce::StringArray ports, names;
    bool stereo = false, wide = true;
};
struct Settings {
    std::map<juce::String, DrumSetup> drumSetups;
    int pedalKey = 0;
    std::vector<GuitarSound> guitarPresets;
    DeviceChoice device;
    bool visited = false, insurance = true;
    juce::String laptopInput;
    std::map<juce::String, juce::String> aliases;
    std::vector<Tuning> tunings;
    juce::String tuningName, lastSong;
    std::map<juce::String, Calibration> latency;
    static juce::String calibrationKey(const DeviceChoice&);
    static juce::File location();
    static Settings load(const juce::File&);
    bool save(const juce::File&) const;
    static juce::String inputKey(const DeviceChoice&, int channel);
};
} // namespace rocker
