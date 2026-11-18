#include "Devices.h"
#include <algorithm>

namespace rocker {
namespace {
juce::String choose(const juce::StringArray& list, const juce::String& wanted, int fallback) {
    if (list.contains(wanted)) return wanted;
    return list[juce::jlimit(0, std::max(0, list.size() - 1), fallback)];
}
class Native final : public DeviceSystem, private juce::AudioIODeviceCallback,
                     private juce::AudioIODeviceType::Listener, private juce::AsyncUpdater {
    using Type = juce::AudioIODeviceType;
    struct Entry { Driver driver; std::unique_ptr<Type> type; };
    std::vector<Entry> types;
    std::unique_ptr<juce::AudioIODevice> device;
    Audio* audio = nullptr;
    std::atomic<bool> closing{false};
    void add(Driver driver, Type* type) {
        if (type) {
            types.push_back({driver, std::unique_ptr<Type>(type)});
            type->addListener(this);
        }
    }
public:
    Native() {
#if JUCE_WINDOWS
        add(Driver::asio, Type::createAudioIODeviceType_ASIO());
        add(Driver::exclusive, Type::createAudioIODeviceType_WASAPI(juce::WASAPIDeviceMode::exclusive));
        add(Driver::lowLatency, Type::createAudioIODeviceType_WASAPI(juce::WASAPIDeviceMode::sharedLowLatency));
        add(Driver::shared, Type::createAudioIODeviceType_WASAPI(juce::WASAPIDeviceMode::shared));
#elif JUCE_MAC
        add(Driver::core, Type::createAudioIODeviceType_CoreAudio());
#else
        add(Driver::alsa, Type::createAudioIODeviceType_ALSA());
#endif
    }
    ~Native() override {
        for (auto& e : types) e.type->removeListener(this);
        close();
        cancelPendingUpdate();
    }
    std::vector<Ports> scan() override {
        std::vector<Ports> result;
        for (auto& e : types) {
            e.type->scanForDevices();
            auto in = e.type->getDeviceNames(true), out = e.type->getDeviceNames(false);
            if (!in.isEmpty() || !out.isEmpty())
                result.push_back({e.driver, in, out, e.type->hasSeparateInputsAndOutputs(),
                                  e.type->getDefaultDeviceIndex(true), e.type->getDefaultDeviceIndex(false)});
        }
        return result;
    }
    void close() override {
        closing = true;
        if (device) { device->stop(); device->close(); device.reset(); }
        audio = nullptr;
        closing = false;
    }
    bool open(DeviceChoice& choice, Capabilities& caps, Audio& processor) override {
        close();
        for (auto& e : types) {
            if (e.driver != choice.driver) continue;
            device.reset(e.type->createDevice(choice.output, choice.input));
            if (!device) break;
            caps.rates = device->getAvailableSampleRates();
            caps.buffers = device->getAvailableBufferSizesAvailable();
            if (!caps.rates.contains(choice.rate))
                choice.rate = caps.rates.contains(48000) ? 48000 : static_cast<int>(caps.rates.getFirst());
            if (!caps.buffers.contains(choice.buffer)) choice.buffer = device->getDefaultBufferSize();
            juce::BigInteger ins, outs;
            ins.setRange(0, device->getInputChannelNames().size(), true);
            outs.setRange(0, std::min(2, device->getOutputChannelNames().size()), true);
            if (device->open(ins, outs, choice.rate, choice.buffer).isNotEmpty()) break;
            const auto names = device->getInputChannelNames();
            const auto active = device->getActiveInputChannels();
            caps.inputs.clear();
            for (int i = 0; i < names.size(); ++i) if (active[i]) caps.inputs.add(names[i]);
            caps.outputChannels = device->getActiveOutputChannels().countNumberOfSetBits();
            choice.rate = juce::roundToInt(device->getCurrentSampleRate());
            choice.buffer = device->getCurrentBufferSizeSamples();
            processor.prepare(caps.inputs.size(), choice.rate);
            audio = &processor;
            device->start(this);
            if (device->isPlaying()) return true;
            break;
        }
        close();
        return false;
    }
private:
    void audioDeviceIOCallbackWithContext(const float* const* in, int ni, float* const* out, int no,
                                          int n, const juce::AudioIODeviceCallbackContext&) override {
        audio->process(in, ni, out, no, n);
    }
    void audioDeviceAboutToStart(juce::AudioIODevice*) override {}
    void audioDeviceStopped() override { if (!closing.load()) triggerAsyncUpdate(); }
    void audioDeviceError(const juce::String&) override { triggerAsyncUpdate(); }
    void audioDeviceListChanged() override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { if (changed) changed(); }
};
}
std::unique_ptr<DeviceSystem> nativeDevices() { return std::make_unique<Native>(); }
bool Connection::select(DeviceChoice preferred) {
    system.close();
    online = false;
    caps = {};
    current = preferred;
    for (auto& p : ports) {
        if (p.driver != preferred.driver) continue;
        current.input = choose(p.inputs, preferred.input, p.defaultInput);
        current.output = choose(p.outputs, preferred.output, p.defaultOutput);
        if (!p.separate && p.outputs.contains(current.input)) current.output = current.input;
        online = system.open(current, caps, audio);
        if (!online) caps = {};
        break;
    }
    if (!online) audio.prepare(0, 48000);
    return online;
}
void Connection::rescan(DeviceChoice preferred) {
    ports = system.scan();
    if (select(preferred)) return;
    for (auto& p : ports) {
        if (p.driver != preferred.driver && select({p.driver})) return;
    }
}
}
