#pragma once
#include "Audio.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>
#include <vector>

namespace rocker {
enum class Driver { asio, exclusive, lowLatency, shared, core, alsa };
struct DeviceChoice {
    Driver driver = Driver::asio;
    juce::String input, output;
    int rate = 48000, buffer = 128;
    bool operator==(const DeviceChoice&) const = default;
};
struct Capabilities {
    juce::StringArray inputs, outputs;
    juce::Array<double> rates;
    juce::Array<int> buffers;
    int outputChannels = 0;
};
struct Ports {
    Driver driver;
    juce::StringArray inputs, outputs;
    bool separate;
    int defaultInput = 0, defaultOutput = 0;
};
class DeviceSystem {
  public:
    virtual ~DeviceSystem() = default;
    virtual std::vector<Ports> scan() = 0;
    virtual bool open(DeviceChoice&, Capabilities&, Audio&) = 0;
    virtual void close() = 0;
    std::function<void()> changed;
};
std::unique_ptr<DeviceSystem> nativeDevices();

// All device changes and reads of channel layout take place on the message thread.
class Connection {
  public:
    explicit Connection(DeviceSystem& s) : system(s) {}
    ~Connection() {
        system.changed = {};
        system.close();
    }
    void rescan(DeviceChoice preferred);
    bool select(DeviceChoice preferred);
    std::vector<Ports> ports;
    DeviceChoice current;
    Capabilities caps;
    Audio audio;
    bool online = false;

  private:
    DeviceSystem& system;
};
} // namespace rocker
