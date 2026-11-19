#pragma once
#include "engine/Devices.h"
#include <vector>

namespace rocker::test {
class FakeDevice final : public DeviceSystem {
  public:
    using Block = std::vector<std::vector<float>>;
    int channels = 2, outputs = 2, rate = 48000;
    bool separate = false;
    bool core = false, asio = true, laptop = false, attached = true;
    juce::Array<juce::MidiDeviceInfo> midi;
    juce::String selectedMidi;
    juce::Array<juce::MidiDeviceInfo> midiDevices() override { return midi; }
    bool openMidi(const juce::String& id, Audio&) override {
        selectedMidi.clear();
        for (auto& p : midi)
            if (p.identifier == id) {
                selectedMidi = id;
                return true;
            }
        return false;
    }
    Audio* active = nullptr;
    std::vector<Ports> scan() override {
        if (!attached)
            return {};
        if (core)
            return {{Driver::core, {"Studio USB"}, {"Studio USB"}, true}};
        std::vector<Ports> list;
        if (asio)
            list.push_back({Driver::asio, {"Studio USB"}, {"Studio USB"}, false});
        if (laptop)
            list.push_back({Driver::shared, {"Internal microphone"}, {"Headphones"}, true});
        else
            for (auto d : {Driver::exclusive, Driver::lowLatency, Driver::shared})
                list.push_back({d, {"Studio USB"}, {"Studio USB"}, true});
        return list;
    }
    bool open(DeviceChoice& d, Capabilities& caps, Audio& audio) override {
        if (!attached)
            return false;
        caps.rates = {static_cast<double>(rate)};
        caps.buffers = {64, 128, 256};
        caps.outputChannels = outputs;
        caps.separateClocks = separate;
        caps.inputLatency = 121;
        caps.outputLatency = 252;
        d.rate = rate;
        if (!caps.buffers.contains(d.buffer))
            d.buffer = 128;
        for (int i = 0; i < channels; ++i)
            caps.inputs.add("In " + juce::String(i + 1));
        audio.prepare(channels, rate);
        active = &audio;
        return true;
    }
    void close() override { active = nullptr; }
    void disconnect() {
        attached = false;
        if (changed)
            changed();
    }
    Block process(const Block& inputs, int frames) {
        std::vector<const float*> in;
        for (auto& channel : inputs)
            in.push_back(channel.data());
        Block result(outputs, std::vector<float>(frames, 2.0f));
        std::vector<float*> out;
        for (auto& channel : result)
            out.push_back(channel.data());
        active->process(in.data(), (int)in.size(), out.data(), outputs, frames);
        return result;
    }
};
} // namespace rocker::test
