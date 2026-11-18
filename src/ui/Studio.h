#pragma once
#include "storage/Settings.h"
#include "Tokens.h"
#include <functional>

namespace rocker {
enum class Page { home, device, tone, inputs };
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
    void button(const juce::String&, Rect, Handler, int look = 0, juce::String key = {});
    void combo(const juce::String&, Rect, const juce::StringArray&, int selected,
               std::function<void(int)> change, bool mono = false);
    void choose(DeviceChoice);
};
} // namespace rocker
