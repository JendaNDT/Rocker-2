#include "ui/Studio.h"
#include "ui/Strings.h"

namespace rocker {
class Window final : public juce::DocumentWindow {
  public:
    Window(DeviceSystem& devices) : DocumentWindow(cs::app, style::background, allButtons) {
        setUsingNativeTitleBar(true);
        setResizable(true, false);
        setResizeLimits(style::minWidth, style::minHeight, style::maxWindow, style::maxWindow);
        setContentOwned(new Studio(devices, Settings::location()), true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
        getContentComponent()->grabKeyboardFocus();
    }
    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};
class Application final : public juce::JUCEApplication {
  public:
    const juce::String getApplicationName() override { return cs::app; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void initialise(const juce::String&) override {
        devices = nativeDevices();
        window = std::make_unique<Window>(*devices);
    }
    void shutdown() override {
        window.reset();
        devices.reset();
    }

  private:
    std::unique_ptr<DeviceSystem> devices;
    std::unique_ptr<Window> window;
};
} // namespace rocker
START_JUCE_APPLICATION(rocker::Application)
