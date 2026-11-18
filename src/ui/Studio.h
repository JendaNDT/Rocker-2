#pragma once
#include "storage/Settings.h"
#include "Tokens.h"
#include <functional>

namespace rocker {
enum class Page { home, device, tone, inputs, live, amp, tuner, diagnostics, customTuning };
class Theme final : public juce::LookAndFeel_V4 {
  public:
    Theme();
    juce::Font font(float size, bool mono = false, bool bold = false) const;
    juce::Font getPopupMenuFont() override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

  private:
    juce::Typeface::Ptr regular, heavy, digits;
};
class Studio final : public juce::Component, private juce::Timer, private juce::AsyncUpdater {
  public:
    Studio(DeviceSystem&, juce::File);
    ~Studio() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    Page currentPage() const { return page; }
    juce::StringArray layoutErrors() const;
    const Theme& theme() const { return skin; }

  private:
    using Rect = juce::Rectangle<int>;
    using Handler = std::function<void()>;
    struct Line {
        juce::String value;
        Rect bounds;
        juce::Font font;
        juce::Colour ink;
        bool right;
    };
    Theme skin;
    DeviceSystem& devices;
    juce::File settingsFile;
    Settings settings;
    Connection link;
    Page page;
    juce::OwnedArray<juce::Component> controls;
    std::vector<Line> textLines;
    std::vector<Rect> panels;
    juce::Viewport viewport;
    juce::Component rows;
    juce::String message;
    int answer = -1;
    bool played = false, laptop = false;
    std::vector<std::uint32_t> activityBeforePluck;
    int detectionTicks = 0;
    void identifyGuitar();
    void handleAsyncUpdate() override;
    void timerCallback() override;
    void rebuild();
    void show(Page);
    void persist();
    void finish();
    void refresh(bool);
    void status(juce::String);
    void makeHome(Rect);
    void makeWizard(Rect);
    void devicePage(Rect);
    void tonePage(Rect);
    void inputPage(Rect);
    void text(juce::String, Rect, int size = style::body, juce::Colour colour = style::chalk,
              bool mono = false, bool bold = false, bool right = false);
    juce::Button* button(const juce::String&, Rect, Handler, int look = 0, juce::String key = {});
    void combo(const juce::String&, Rect, const juce::StringArray&, int selected,
               std::function<void(int)> change, bool mono = false);
    void choose(DeviceChoice);
    Page tunerReturn = Page::home;
    int measurementInput = 0, guitarSource = 0, primaryInput = 0, secondaryInput = 1, presetIndex = 1;
    bool hearSecond = false;
    AmpParameters sound = presets[1];
    std::vector<Tuning> tunings;
    int tuningIndex = 0, targetString = -1, nearestString = 0, stableString = -1;
    double stableTime = 0, readingHz = 0, cents = 0;
    double lastVoiced = 0, needleCents = 0;
    std::vector<bool> confirmed;
    juce::String customName, customNotes;
    Rect needleArea, inputBar, outputBar, routeSymbol;
    enum class Readout { inputLevel, level, cpu, pitch, hint, confirmed };
    std::map<Readout, std::size_t> dynamicLines;
    void livePages(Rect);
    void playPanel(Rect);
    void ampPanel(Rect);
    void tunerPanel(Rect);
    void diagnosticPanel(Rect);
    void customPanel(Rect);
    void pollLive();
    void resetRoute();
    void resetTuning();
    void changeMonitor(int);
    void toggleTuner();
    int heardInput() const;
    juce::StringArray inputNames() const;
    int listeningDelaySamples() const;
    juce::String latencyLabel() const;
    void dynamicText(Readout, juce::String, Rect, int, bool);
    void setReadout(Readout, const juce::String&);
    void fader(const juce::String&, Rect, int, int, int, const juce::String&, std::function<void(int)>);
};
} // namespace rocker
