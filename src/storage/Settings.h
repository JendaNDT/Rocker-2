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
    juce::String libraryPath, clockOverride;
    int pedalKey = 0;
    int clickVolume = 100, clickSound = 0;
    struct MidiSetup {
        bool coupled = false, enabled = true;
        std::map<int, int> pads;
    };
    juce::String midiDevice;
    std::map<juce::String, MidiSetup> midiSetups;
    std::map<juce::String, int> midiCorrections;
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
