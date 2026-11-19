#include "Studio.h"
#include "Strings.h"
#include <BinaryData.h>
namespace rocker {
namespace s = style;
using namespace juce;
namespace {
using Rect = Rectangle<int>;
float width(const Font& f, const String& t) { return GlyphArrangement::getStringWidth(f, t); }
String shortened(const Font& font, String text, int available) {
    if (width(font, text) <= available)
        return text;
    while (text.isNotEmpty() && width(font, text + String(u8"…")) > available)
        text = text.dropLastCharacters(1);
    return text + String(u8"…");
}
bool builtIn(const String& name) {
    return name.containsIgnoreCase("built-in") || name.containsIgnoreCase("internal mic") ||
           name.containsIgnoreCase("macbook") || name.containsIgnoreCase("realtek") ||
           name.containsIgnoreCase("microphone array") ||
           name.containsIgnoreCase(String(u8"mikrofonní pole"));
}
class Action final : public Button {
  public:
    Action(const String& title, int kind, String key, Icon symbol, String visible, Theme& theme)
        : Button(title), mode(kind), shortcut(std::move(key)), icon(symbol),
          label(std::move(visible)), skin(theme) {
        setWantsKeyboardFocus(true);
        if (icon != Icon::none)
            setTooltip(title);
    }
    bool isRound() const { return mode >= roundIdle && mode <= roundRecording; }
    bool isRail() const { return mode == railIdle || mode == railCurrent; }
    String shownTitle() const { return label.isEmpty() ? getName() : label; }
    Rect titleArea() const {
        if (isRail())
            return getLocalBounds().reduced(s::grid, 0).removeFromBottom(s::inset);
        if (isRound())
            return getLocalBounds();
        if (mode == armActive || mode == muteActive || mode == soloActive || getWidth() <= s::armWidth)
            return getLocalBounds().reduced(s::grid, 0);
        auto area = getLocalBounds().reduced(s::inset, 0);
        if (mode == recording || mode == newSong || mode == tunerButton)
            area.removeFromLeft(s::icon + s::gap);
        if (shortcut.isNotEmpty())
            area.removeFromRight(s::keyWidth);
        return area;
    }
    Font titleFont() const {
        if (isRail())
            return skin.font(s::railLabel, false, true);
        if (mode == armActive || mode == muteActive || mode == soloActive || getWidth() <= s::armWidth)
            return skin.font(s::section, false, true);
        return skin.font(mode == recording || mode == newSong ? s::heading : s::body,
                         getProperties().contains("number"), true);
    }
    void paintButton(Graphics& g, bool over, bool down) override {
        if (!isEnabled())
            g.beginTransparencyLayer(s::disabledOpacity);
        auto bounds = getLocalBounds().toFloat().reduced(s::hairline);
        if (isRail() || isRound()) {
            const auto ink = mode == roundPlay ? s::background : isRail() && !over && mode != railCurrent
                                                               ? s::muted : s::chalk;
            if (isRail()) {
                if (mode == railCurrent || over || down) {
                    g.setColour(s::raised);
                    g.fillRoundedRectangle(bounds, s::panelRadius);
                }
                auto top = getLocalBounds().withTrimmedBottom(s::inset - s::grid);
                paintIcon(g, icon, ink, top.withSizeKeepingCentre(s::icon, s::icon).toFloat());
                g.setColour(ink);
                g.setFont(titleFont());
                g.drawText(shownTitle(), titleArea(), Justification::centred, false);
                if (shortcut.isNotEmpty()) {
                    g.setFont(skin.font(s::railLabel, true));
                    g.drawText(shortcut, getLocalBounds().reduced(s::grid)
                                             .removeFromTop(s::caption), Justification::topRight);
                }
                if (mode == railCurrent) {
                    g.setColour(s::chalk);
                    g.fillRoundedRectangle(bounds.withWidth(s::railIndicator)
                                               .reduced(0, s::gap), s::stroke);
                }
            } else {
                g.setColour(mode == roundPlay ? s::chalk : mode == roundRecording ? s::recording
                                                               : over || down ? s::raised : s::panel);
                g.fillEllipse(bounds);
                g.setColour(mode == roundRecord || mode == roundRecording ? s::record : s::barGrid);
                g.drawEllipse(bounds, s::stroke);
                if (mode == roundRecord) {
                    g.setColour(s::record);
                    g.fillEllipse(bounds.withSizeKeepingCentre(s::icon, s::icon));
                } else {
                    paintIcon(g, icon, ink, bounds.withSizeKeepingCentre(s::icon, s::icon));
                }
            }
            if (hasKeyboardFocus(true)) {
                g.setColour(s::selection);
                g.drawRoundedRectangle(bounds, s::panelRadius, s::stroke);
            }
            if (!isEnabled())
                g.endTransparencyLayer();
            return;
        }
        const bool active = mode == primary || mode == selected;
        const bool toggle = mode == pillOff || mode == pillOn;
        const auto radius = toggle ? s::transportPillRadius : s::radius;
        g.setColour(mode == armActive ? s::record.withAlpha(s::armedOpacity)
                    : mode == muteActive ? s::warning : mode == soloActive ? s::selection
                    : mode == clipButton ? s::clipping
                    : active           ? s::chalk
                                       : (down || over ? s::barGrid : s::raised));
        g.fillRoundedRectangle(bounds, radius);
        g.setColour(hasKeyboardFocus(true) ? s::selection : mode == armActive ? s::record : s::line);
        g.drawRoundedRectangle(bounds, radius, hasKeyboardFocus(true) ? s::stroke : s::hairline);
        const auto titleInk = active || mode == clipButton || mode == muteActive || mode == soloActive
                              ? s::background : mode == armActive ? s::armedInk : s::chalk;
        g.setColour(titleInk);
        auto area = getLocalBounds().reduced(s::inset, 0);
        auto symbol = area.withWidth(s::icon).withSizeKeepingCentre(s::icon, s::icon).toFloat();
        if (mode == recording) {
            g.setColour(s::record);
            g.fillEllipse(symbol.reduced(s::stroke));
            g.setColour(s::chalk);
        } else if (mode == newSong) {
            g.drawLine(symbol.getX(), symbol.getCentreY(), symbol.getRight(), symbol.getCentreY(),
                       s::stroke);
            g.drawLine(symbol.getCentreX(), symbol.getY(), symbol.getCentreX(), symbol.getBottom(),
                       s::stroke);
        } else if (mode == tunerButton) {
            auto fork = symbol.reduced(s::grid, 0);
            g.drawLine(fork.getX(), fork.getY(), fork.getX(), fork.getCentreY(), s::stroke);
            g.drawLine(fork.getRight(), fork.getY(), fork.getRight(), fork.getCentreY(), s::stroke);
            g.drawLine(fork.getX(), fork.getCentreY(), fork.getRight(), fork.getCentreY(), s::stroke);
            g.drawLine(fork.getCentreX(), fork.getCentreY(), fork.getCentreX(), fork.getBottom(),
                       s::stroke);
        }
        auto titleBounds = titleArea();
        if (toggle) {
            auto led = titleBounds.removeFromLeft(s::inset)
                           .withSizeKeepingCentre(s::transportLed, s::transportLed);
            g.setColour(mode == pillOn ? s::ready : s::barGrid);
            g.fillEllipse(led.toFloat());
        }
        if (icon != Icon::none) {
            auto iconBounds = shownTitle() == " " ? getLocalBounds()
                                                  : titleBounds.removeFromLeft(s::icon + s::gap);
            paintIcon(g, icon, titleInk, iconBounds.withSizeKeepingCentre(s::icon, s::icon).toFloat());
        }
        g.setColour(titleInk);
        g.setFont(titleFont());
        g.drawText(shownTitle(), titleBounds,
                   mode == recording || mode == newSong ? Justification::centredLeft
                                                        : Justification::centred,
                   false);
        if (shortcut.isNotEmpty()) {
            g.setFont(skin.font(s::caption, true));
            g.setColour(s::muted);
            g.drawText(shortcut, area.removeFromRight(s::keyWidth), Justification::centredRight, false);
        }
        if (!isEnabled())
            g.endTransparencyLayer();
    }

  private:
    int mode;
    String shortcut;
    Icon icon;
    String label;
    Theme& skin;
};
class InputRow final : public Component {
  public:
    InputRow(Theme& theme, Connection& connection, int index, const String& name,
             std::function<void(String)> rename)
        : skin(theme), link(connection), channel(index),
          clip(cs::clip, clipButton, {}, Icon::none, {}, theme) {
        addAndMakeVisible(editor);
        editor.setName(cs::channel(index));
        editor.setFont(skin.font(s::body));
        editor.setTextToShowWhenEmpty(cs::nameHint, s::muted);
        editor.setText(name, false);
        editor.onTextChange = [this, rename] { rename(editor.getText()); };
        addChildComponent(clip);
        clip.onClick = [this] {
            link.audio.clearClip(channel);
            update();
        };
        update();
    }
    void update() {
        reading = link.audio.read(channel);
        clip.setVisible(reading.clipped);
        repaint();
    }
    void resized() override {
        auto area = getLocalBounds()
                        .reduced(s::inset, 0)
                        .withSizeKeepingCentre(getWidth() - 2 * s::inset, s::button);
        area.removeFromLeft(s::levelWidth);
        editor.setBounds(area.removeFromLeft(s::nameWidth));
        clip.setBounds(area.removeFromRight(s::clipWidth));
    }
    void paint(Graphics& g) override {
        g.fillAll(s::panel);
        auto area = getLocalBounds().reduced(s::inset, 0);
        g.setFont(skin.font(s::body, true));
        g.setColour(s::chalk);
        g.drawText(cs::channel(channel), area.removeFromLeft(s::levelWidth), Justification::centredLeft);
        area.removeFromLeft(s::nameWidth + s::gutter);
        area.removeFromRight(s::clipWidth + s::gap);
        auto value = area.removeFromRight(s::levelWidth);
        const String label = reading.held > 0
                                 ? String(decibels(reading.held), 1).replaceCharacter('.', ',') + cs::db
                                 : String(cs::silence);
        g.drawText(label, value, Justification::centred, false);
        auto meter = area.withSizeKeepingCentre(area.getWidth(), s::gap);
        g.setColour(s::raised);
        g.fillRect(meter);
        auto extent = [&](float level) {
            return roundToInt(meter.getWidth() * jlimit(0.0f, 1.0f, (decibels(level) + 60.0f) / 60.0f));
        };
        g.setColour(reading.clipped ? s::clipping : s::ready);
        g.fillRect(meter.withWidth(extent(reading.peak)));
        if (reading.held > 0) {
            g.setColour(s::chalk);
            g.fillRect(meter.withX(meter.getX() + std::max(0, extent(reading.held) - s::stroke))
                           .withWidth(s::stroke));
        }
        g.setColour(s::line);
        g.fillRect(getLocalBounds().removeFromBottom(s::hairline));
    }
    bool fits() const {
        return width(skin.font(s::body, true), cs::channel(channel)) <= s::levelWidth &&
               width(skin.font(s::body, true), "-144.0 dBFS") <= s::levelWidth &&
               width(clip.titleFont(), clip.getName()) <= clip.titleArea().getWidth() &&
               editor.getRight() + s::gutter + s::levelWidth < clip.getX();
    }

  private:
    Theme& skin;
    Connection& link;
    int channel;
    Reading reading{};
    TextEditor editor;
    Action clip;
};
} // namespace
Theme::Theme() {
    regular = Typeface::createSystemTypefaceFor(BinaryData::Inter_ttf, BinaryData::Inter_ttfSize);
    heavy = Typeface::createSystemTypefaceFor(BinaryData::InterSemiBold_ttf,
                                              BinaryData::InterSemiBold_ttfSize);
    digits = Typeface::createSystemTypefaceFor(BinaryData::Mono_ttf, BinaryData::Mono_ttfSize);
    setColour(ComboBox::textColourId, s::chalk);
    setColour(PopupMenu::backgroundColourId, s::panel);
    setColour(PopupMenu::textColourId, s::chalk);
    setColour(PopupMenu::highlightedBackgroundColourId, s::raised);
    setColour(PopupMenu::highlightedTextColourId, s::chalk);
    setColour(TextEditor::backgroundColourId, s::raised);
    setColour(TextEditor::textColourId, s::chalk);
    setColour(TextEditor::outlineColourId, s::line);
    setColour(TextEditor::focusedOutlineColourId, s::selection);
    setColour(TextEditor::highlightColourId, s::selection.withAlpha(s::selectionOpacity));
    setColour(CaretComponent::caretColourId, s::chalk);
    setColour(ScrollBar::thumbColourId, s::faint);
}
Font Theme::font(float size, bool mono, bool bold) const {
    auto face = mono ? digits : (bold ? heavy : regular);
    return Font(FontOptions(face).withPointHeight(size).withFallbackEnabled(false));
}
Font Theme::getPopupMenuFont() { return font(s::body); }
Font Theme::getComboBoxFont(ComboBox& c) { return font(s::body, c.getProperties().contains("number")); }
void Theme::positionComboBoxText(ComboBox& c, Label& l) {
    l.setBounds(c.getLocalBounds().reduced(s::gap, 0).withTrimmedRight(s::icon));
    l.setFont(getComboBoxFont(c));
    l.setBorderSize({});
}
void Theme::drawComboBox(Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& c) {
    auto r = Rectangle<float>(0, 0, (float)w, (float)h).reduced(s::hairline);
    g.setColour(s::raised);
    g.fillRoundedRectangle(r, s::radius);
    g.setColour(c.hasKeyboardFocus(true) ? s::selection : s::line);
    g.drawRoundedRectangle(r, s::radius, s::hairline);
    Path p;
    auto x = w - s::inset;
    auto y = h / 2;
    p.startNewSubPath(x - s::grid, y - s::stroke);
    p.lineTo(x, y + s::stroke);
    p.lineTo(x + s::grid, y - s::stroke);
    g.setColour(s::chalk);
    g.strokePath(p, PathStrokeType(s::stroke));
}
Studio::Studio(DeviceSystem& sys, File path, File root)
    : devices(sys), settingsFile(std::move(path)), settings(Settings::load(settingsFile)), link(sys),
      page(settings.visited ? Page::home : Page::device) {
    setLookAndFeel(&skin);
    setWantsKeyboardFocus(true);
    viewport.setViewedComponent(&rows, false);
    viewport.setScrollBarsShown(true, false);
    addChildComponent(viewport);
    tunings = Tuning::factory();
    tunings.insert(tunings.end(), settings.tunings.begin(), settings.tunings.end());
    for (int i = 0; i < (int)tunings.size(); ++i)
        if (tunings[i].name == settings.tuningName)
            tuningIndex = i;
    libraryRoot = root == File{} ? settingsFile.getSiblingFile("Skladby") : root;
    ideas = std::make_unique<Ideas>(libraryRoot.getSiblingFile("Napady"));
    if (File::isAbsolutePath(settings.libraryPath))
        libraryRoot = File(settings.libraryPath);
    ideas->recover();
    if (!ideas->all().empty())
        chooseIdea(ideas->all().front());
    resetTuning();
    devices.midiChanged = [this] {
        refreshMidi();
        triggerAsyncUpdate();
    };
    devices.changed = [this] { refresh(true); };
    refresh(false);
    setSize(s::minWidth, s::minHeight);
    for (const auto& dir : libraryRoot.findChildFiles(File::findDirectories, false))
        if (!dir.getChildFile("Zachrana").findChildFiles(File::findFiles, false, "*.json").isEmpty()) {
            openProject(dir);
            break;
        }
    startTimerHz(30);
}
Studio::~Studio() {
    if (fileTask.valid())
        fileTask.wait();
    devices.midiChanged = {};
    stopTimer();
    stopMetronome();
    stopPreview();
    stopIdea();
    if (preparingPreview.valid()) {
        preparingPreview.wait();
        finishPreview();
    }
    if (savingIdea.valid())
        savingIdea.wait();
    if (importing.valid()) {
        stopProject();
        importing.wait();
        finishImport();
    }
    finishProject();
    cancelPendingUpdate();
    devices.changed = {};
    controls.clear();
    viewport.setViewedComponent(nullptr, false);
    setLookAndFeel(nullptr);
}
void Studio::persist() {
    if (!settings.save(settingsFile))
        message = cs::settingsError;
}
void Studio::refresh(bool notify) {
    answer = -1;
    played = false;
    stopMetronome();
    stopPreview();
    stopIdea();
    activityBeforePluck.clear();
    const bool recording=performance && performance->recording;
    if(performance) stopProject();
    const auto outcome=recording?message:String{};
    link.rescan(settings.device);
    laptop = link.online &&
             (builtIn(link.current.input) ||
              (settings.laptopInput.isNotEmpty() && settings.laptopInput == link.current.input));
    if (laptop && link.current.driver != Driver::core && link.current.driver != Driver::shared &&
        settings.device.input.isEmpty()) {
        for (const auto& p : link.ports) {
            if (p.driver != Driver::shared || !p.inputs.contains(link.current.input))
                continue;
            const auto previous = link.current;
            auto shared = previous;
            shared.driver = Driver::shared;
            if (!link.select(shared))
                link.select(previous);
            break;
        }
    }
    resetRoute();
    refreshMidi();
    if (notify) {
        message = outcome.isEmpty()?cs::unplugged:cs::unplugged+cs::divider+outcome;
        if (page != Page::home)
            page = Page::device;
    }
    triggerAsyncUpdate();
}
void Studio::choose(DeviceChoice d, bool refreshCalibration) {
    answer = -1;
    played = false;
    stopMetronome();
    stopPreview();
    stopIdea();
    const bool ok = link.select(d);
    resetRoute();
    refreshMidi();
    message = ok ? String{} : String(cs::failed);
    if (ok) {
        settings.device = link.current;
        if (refreshCalibration && link.current.rate == d.rate)
            settings.latency.erase(Settings::calibrationKey(link.current));
        persist();
    }
    laptop = builtIn(link.current.input) ||
             (link.current.input.isNotEmpty() && settings.laptopInput == link.current.input);
    triggerAsyncUpdate();
}
void Studio::finish() {
    show(Page::home);
    settings.visited = true;
    if (link.online)
        settings.device = link.current;
    message = cs::saved;
    persist();
}
void Studio::show(Page value) {
    if (value != Page::metronome)
        stopMetronome();
    if (value != Page::device)
        pedalLearning = false;
    if (value != Page::ideas) {
        pendingIdeaRemoval.clear();
        stopPreview();
        stopIdea();
    }
    if (performance)
        stopProject();
    if ((value == Page::amp || value == Page::live) && page != Page::amp && page != Page::live)
        link.audio.live.setAmp(sound);
    activityBeforePluck.clear();
    link.audio.stopTone();
    const bool keepMonitor =
        (page == Page::live || page == Page::amp) && (value == Page::live || value == Page::amp);
    if (!keepMonitor)
        link.audio.live.silence();
    if (value == Page::live || value == Page::amp || value == Page::diagnostics ||
        value == Page::tuner) {
        int port = value == Page::diagnostics ? measurementInput : heardInput();
        if (page == Page::project && (value == Page::amp || value == Page::tuner) &&
            view.selected < (int)song.tracks.size()) {
            int assigned = link.caps.inputs.indexOf(song.tracks[view.selected].inputs[0]);
            if (assigned >= 0)
                port = assigned;
        }
        link.audio.live.selectInput(port);
    }
    if (value == Page::tuner) {
        if (page != Page::customTuning && page != Page::tuner)
            tunerReturn = page;
        resetTuning();
        link.audio.live.setTuner(true);
    }
    page = value;
    if (value != Page::midi && learningPad >= 0) {
        learningPad = -1;
        refreshMidi();
    }
    link.audio.drums.monitor = value == Page::midi && activeMidi.isNotEmpty();
    if (value == Page::project)
        configureSongMonitor();
    if (value < Page::device || value > Page::inputs) {
        answer = -1;
        played = false;
    }
    message = {};
    triggerAsyncUpdate();
}
void Studio::status(String value) {
    message = std::move(value);
    triggerAsyncUpdate();
}
void Studio::timerCallback() {
    projectTick();
    pollMidi();
    ideaTick();
    if (page == Page::metronome && metronome)
        repaint(metroBeats);
    if ((page >= Page::live && page <= Page::customTuning) || (page == Page::project && soundDeckOpen))
        pollLive();
    if (page == Page::inputs) {
        identifyGuitar();
        for (auto* c : rows.getChildren())
            if (auto* row = dynamic_cast<InputRow*>(c))
                row->update();
    }
}
void Studio::identifyGuitar() {
    if (activityBeforePluck.empty())
        return;
    int active = -1;
    for (int i = 0; i < static_cast<int>(activityBeforePluck.size()); ++i) {
        if (link.audio.read(i).activity == activityBeforePluck[i])
            continue;
        if (active >= 0) {
            activityBeforePluck.clear();
            status(cs::ambiguous);
            return;
        }
        active = i;
    }
    // Observe three UI ticks after the first attack to catch simultaneous channels.
    if (active < 0 || ++detectionTicks < 3)
        return;
    activityBeforePluck.clear();
    auto& alias = settings.aliases[Settings::inputKey(link.current, active)];
    if (alias.isEmpty())
        alias = cs::guitar;
    status(cs::identified + cs::channel(active) + cs::existing);
    persist();
}
void Studio::handleAsyncUpdate() { rebuild(); }
void Studio::resized() { rebuild(); }
void Studio::paint(Graphics& g) {
    g.fillAll(s::background);
    const auto content = getLocalBounds().withTrimmedLeft(navigationBounds.getWidth());
    if (workspaceHeading.isEmpty()) {
        g.setColour(s::panel);
        g.fillRect(content.withHeight(s::header));
    }
    paintWorkspace(g);
    for (auto r : panels) {
        g.setColour(s::panel);
        g.fillRoundedRectangle(r.toFloat(), s::radius);
    }
    g.setColour(s::line);
    if (transportBounds.isEmpty() && workspaceHeading.isEmpty()) {
        g.fillRect(content.getX(), s::header, content.getWidth(), s::hairline);
        g.fillRect(content.getX(), getHeight() - s::footer, content.getWidth(), s::hairline);
    }
    if (page == Page::project)
        paintProject(g);
    if (page == Page::metronome)
        paintMetronome(g);
    if (page == Page::home || page == Page::ideas)
        paintIdeas(g);
    for (const auto& [shape, ink] : homePreview) {
        g.setColour(ink);
        g.fillRect(shape);
    }
    if (page == Page::live) {
        auto drawMeter = [&](Rect r, float value, float held, bool clipped) {
            auto track = r.withSizeKeepingCentre(r.getWidth(), s::grid * 2);
            auto position = [&](float amplitude) {
                return juce::roundToInt(track.getWidth() *
                                        std::clamp((decibels(amplitude) + 60.f) / 60.f, 0.f, 1.f));
            };
            g.setColour(s::line);
            g.fillRect(track);
            g.setColour(clipped ? s::clipping : s::ready);
            g.fillRect(track.withWidth(position(value)));
            if (held > 0) {
                g.setColour(s::chalk);
                g.fillRect(
                    track.withX(track.getX() + std::min(position(held), track.getWidth() - s::stroke))
                        .withWidth(s::stroke));
            }
        };
        auto in = link.audio.read(heardInput());
        drawMeter(inputBar, in.peak, in.held, in.clipped);
        drawMeter(outputBar, link.audio.live.outputPeak.load(), 0, false);
        auto icon = routeSymbol.toFloat();
        g.setColour(s::guitar);
        if (!icon.isEmpty()) {
            auto centre = icon.getCentre();
            if (guitarSource == 0 || guitarSource == 3) {
                auto plug = icon.reduced(s::grid).withTrimmedRight(s::grid);
                g.drawRect(plug, s::stroke);
                g.drawLine(plug.getRight(), centre.y, icon.getRight(), centre.y, s::stroke);
            }
            if (guitarSource > 0) {
                if (guitarSource == 3)
                    icon.translate(s::icon + s::gap, 0);
                g.drawRect(icon, s::stroke);
                g.drawEllipse(icon.reduced(s::grid), s::stroke);
            }
        }
    }
    if (page == Page::tuner)
        paintTuner(g);
    for (const auto& item : textLines) {
        g.setColour(item.ink);
        g.setFont(item.font);
        g.drawText(item.value, item.bounds,
                   item.right ? Justification::centredRight : Justification::centredLeft, false);
    }
}
void Studio::text(String value, Rect r, int size, Colour ink, bool mono, bool bold, bool right) {
    auto f = skin.font(size, mono, bold);
    if (size == s::section)
        f.setExtraKerningFactor(s::tracking);
    textLines.push_back({std::move(value), r, f, ink, right});
}
Button* Studio::button(const String& name, Rect r, Handler handler, int look, String key, Icon icon,
                       String visible) {
    auto* b = new Action(name, look, std::move(key), icon, std::move(visible), skin);
    controls.add(b);
    addAndMakeVisible(b);
    b->setBounds(r);
    b->onClick = std::move(handler);
    return b;
}
void Studio::combo(const String& title, Rect r, const StringArray& items, int selectedIndex,
                   std::function<void(int)> change, bool mono) {
    text(title, r.removeFromLeft(std::min(s::labelWidth, r.getWidth() * 2 / 5)), s::body, s::muted);
    auto* box = new ComboBox;
    controls.add(box);
    addAndMakeVisible(box);
    box->setName(title);
    box->setBounds(r.withWidth(std::min(s::fieldWidth, r.getWidth())));
    if (mono) {
        box->getProperties().set("number", true);
        box->lookAndFeelChanged();
    }
    const auto font = skin.getComboBoxFont(*box);
    for (int i = 0; i < items.size(); ++i)
        box->addItem(shortened(font, items[i], box->getWidth() - s::icon - 2 * s::gap), i + 1);
    box->setTextWhenNothingSelected(cs::noPort);
    box->setSelectedItemIndex(selectedIndex, dontSendNotification);
    box->setEnabled(items.size() > 1);
    box->setTooltip(items[selectedIndex]);
    box->onChange = [box, change] { change(box->getSelectedItemIndex()); };
}
void Studio::rebuild() {
    controls.clear();
    ideaWaves.clear();
    homePreview.clear();
    ideaTrace = {};
    soundBadge = nullptr;
    dynamicLines.clear();
    tunerTiles.clear();
    needleArea = {};
    inputBar = {};
    outputBar = {};
    routeSymbol = {};
    textLines.clear();
    panels.clear();
    viewport.setVisible(false);
    navigationBounds = workspaceHeading = inspectorBounds = transportBounds = {};
    if (getWidth() <= 0)
        return;
    auto workspace = getLocalBounds();
    if (page != Page::project || !view.distant)
        navigation(workspace.removeFromLeft(s::railWidth));
    if (page == Page::metronome && !fileTask.valid()) {
        metronomePage(workspace);
        repaint();
        return;
    }
    if (page == Page::tuner && !fileTask.valid()) {
        tunerPanel(workspace);
        repaint();
        return;
    }
    if (page >= Page::device && page <= Page::inputs && !fileTask.valid()) {
        makeWizard(workspace);
        repaint();
        return;
    }
    if (page == Page::home && !fileTask.valid()) {
        makeHome(workspace);
        repaint();
        return;
    }
    if (page == Page::project && !view.distant && !fileTask.valid()) {
        projectPage(workspace);
        repaint();
        return;
    }
    auto area = workspace.reduced(s::safeGutter, 0);
    area = area.withSizeKeepingCentre(std::min(area.getWidth(), s::maxWidth), area.getHeight());
    if (page == Page::midi) {
        midiPage(area);
        repaint();
        return;
    }
    if (page == Page::drums) {
        drumPage(area);
        repaint();
        return;
    }
    if (page == Page::ideas) {
        ideaPage(area);
        repaint();
        return;
    }
    if (fileTask.valid()) {
        text(cs::workingFiles, area, s::heading);
        repaint();
        return;
    }
    if (page == Page::remap) {
        remapPage(area);
        repaint();
        return;
    }
    if (page >= Page::newProject) {
        projectPage(area);
        repaint();
        return;
    }
    auto header = area.removeFromTop(s::header);
    auto brandFont = skin.font(s::counter, false, true);
    const int brandWidth = roundToInt(std::ceil(width(brandFont, cs::brand))) + s::gap;
    text(cs::brand, header.removeFromLeft(brandWidth), s::counter, s::chalk, false, true);
    text(cs::version, header.removeFromLeft(s::keyWidth), s::counter, s::muted, true);
    const auto endpoint = link.current.output.isEmpty() ? link.current.input : link.current.output;
    auto indicator = link.online ? String(cs::device) + String(u8"  ·  ") +
                                       cs::drivers[(int)link.current.driver] + "  /  " + endpoint
                                 : String(cs::disconnected);
    text(shortened(skin.font(s::caption), indicator, header.getWidth()), header, s::caption, s::muted,
         false, false, true);
    livePages(area);
    repaint();
}
void Studio::makeHome(Rect area) {
    workspaceHeading = area.removeFromTop(s::workspaceHeader);
    auto heading = workspaceHeading.reduced(s::inset, 0);
    text(cs::navHome, heading.removeFromLeft(s::nameWidth), s::workspaceTitle, s::chalk, false, true);
    deviceStatus(heading);

    area.reduce(s::inset, s::inset);
    auto right = area.removeFromRight(s::homeActions);
    area.removeFromRight(s::inset);
    auto actions = [&](int height) {
        auto bounds = right.removeFromTop(height);
        right.removeFromTop(s::gap);
        return bounds;
    };
    button(cs::record, actions(s::recordButton + s::gap), [this] { startIdea(false); }, recording, "R")
        ->setEnabled(!savingIdea.valid());
    text(cs::ideaBefore, actions(s::caption + s::grid), s::caption, s::muted);
    button(cs::keepHistory, actions(s::transport), [this] { startIdea(true); }, plain, "U")
        ->setEnabled(settings.insurance && !savingIdea.valid());
    auto commands = actions(s::transport);
    button(cs::song, commands.removeFromLeft((commands.getWidth() - s::grid) / 2),
           [this] { newProject(); }, plain, "N");
    commands.removeFromLeft(s::grid);
    button(cs::openPackage, commands, [this] { choosePack(true); })->setEnabled(!fileTask.valid());
    button(cs::livePage, actions(s::button), [this] { show(Page::live); }, plain, {}, Icon::none,
           cs::playWithoutSong);
    panels.push_back(right);
    auto ideaCard = right.reduced(s::inset);
    auto ideasTitle = ideaCard.removeFromTop(s::button);
    text(cs::ideas.toUpperCase(), ideasTitle.removeFromLeft(s::levelWidth), s::section,
         s::navigationInk, false, true);
    button(cs::library, ideasTitle, [this] { show(Page::ideas); });
    ideaCard.removeFromTop(s::gap);
    if (ideas->all().empty())
        text(cs::emptyIdeas, ideaCard.removeFromTop(s::button), s::caption, s::muted);
    else
        ideaHome(ideaCard);

    auto recent = libraryRoot.findChildFiles(File::findDirectories, false);
    const File last = settings.lastSong.isEmpty() ? File{} : File(settings.lastSong);
    if (last.isDirectory())
        recent.addIfNotAlreadyThere(last);
    for (int i = recent.size(); --i >= 0;)
        if (!recent[i].getChildFile("skladba.json").existsAsFile() && Song::latestVersion(recent[i]) == File{})
            recent.remove(i);
    std::stable_sort(recent.begin(), recent.end(), [&last](const auto& a, const auto& b) {
        if (a == b)
            return false;
        if (a == last)
            return true;
        if (b == last)
            return false;
        return a.getChildFile("skladba.json").getLastModificationTime() >
               b.getChildFile("skladba.json").getLastModificationTime();
    });
    if ((!last.isDirectory() || !recent.contains(last)) && !recent.isEmpty())
        settings.lastSong = recent.getFirst().getFullPathName();

    auto notice = area.removeFromBottom(s::button);
    text(limit(skin.font(s::caption), message, notice.getWidth()), notice, s::caption, s::warning);
    auto resume = area.removeFromTop(s::resumeHeight);
    panels.push_back(resume);
    resume.reduce(s::inset, s::inset);
    text(cs::resume.toUpperCase(), resume.removeFromTop(s::button), s::section,
         s::navigationInk, false, true);
    auto resumeTitle = resume.removeFromTop(s::transport);
    const File target = recent.isEmpty() ? File{} : recent.getFirst();
    auto* proceed = button(cs::resume, resumeTitle.removeFromRight(s::clipWidth),
                            [this, target] { openProject(target); }, primary);
    auto title = resumeTitle.withTrimmedRight(s::gap);
    bool damaged = false;
    auto current = target == File{} ? std::optional<Song>{} : Song::load(target, damaged);
    proceed->setEnabled(target != File{});
    const auto name = current ? current->title : target != File{} ? target.getFileName() : cs::emptySongs;
    text(limit(skin.font(s::counter, false, true), name, title.getWidth()), title, s::counter,
         s::chalk, false, true);
    if (current) {
        const auto viewState = ViewState::load(target);
        auto details = resume.removeFromTop(s::button);
        auto metadata = cs::bar + " " +
            String(current->grid.beatAt(viewState.cursor) / current->grid.beats + 1) + cs::divider +
            current->tuning + cs::divider + String(current->grid.bpm, 1) + " BPM" + cs::divider +
            String(current->grid.beats) + "/" + String(current->grid.unit);
        text(limit(skin.font(s::caption), metadata, details.getWidth()), details, s::caption, s::muted);
        auto strip = resume.removeFromTop(s::caption);
        const auto duration = std::max<Frame>(1, current->end());
        auto x = [&](Frame frame) {
            return strip.getX() + roundToInt(std::clamp(double(frame) / duration, 0., 1.) * strip.getWidth());
        };
        for (int i = 0; i < (int)current->order.size(); ++i) {
            const auto& occurrence = current->order[i];
            const auto start = current->startBar(i);
            const auto from = x(current->barFrame(start));
            const auto to = x(current->barFrame(start + occurrence.length));
            const auto key = current->sections.at(occurrence.section).colour;
            const auto ink = key == "uvod" ? s::intro : key == "refren" ? s::chorus
                           : key == "most" ? s::bridge : key == "solo" ? s::solo
                           : key == "zaver" ? s::ending : s::verse;
            if (to > from)
                homePreview.emplace_back(strip.withLeft(from).withRight(to).reduced(s::hairline, 0), ink);
        }
        auto waves = resume.removeFromTop(s::caption + s::grid);
        if (target == projectDir)
            for (const auto& clip : current->clips) {
                auto cached = library.data.find(clip.asset);
                const auto start = x(clip.position), end = x(clip.position + clip.length);
                if (cached == library.data.end() || end <= start)
                    continue;
                for (int px = start; px < end; ++px) {
                    const auto position = (px - start) * clip.length / (end - start);
                    const int count = int(std::clamp<Frame>(clip.length / (end - start), 1,
                                                           std::numeric_limits<int>::max()));
                    const int amplitude = roundToInt(cached->second->peak(clip.offset + position, count) *
                                                     waves.getHeight());
                    if (amplitude > 0)
                        homePreview.emplace_back(Rect(px, waves.getCentreY() - amplitude / 2,
                                                      s::hairline, amplitude), s::guitar);
                }
            }
        homePreview.emplace_back(Rect(x(viewState.cursor) - s::hairline, strip.getY(),
                                      s::stroke, strip.getHeight() + waves.getHeight()), s::playhead);
    }
    area.removeFromTop(s::inset);
    text(cs::recentSongs.toUpperCase(), area.removeFromTop(s::button), s::section, s::navigationInk,
         false, true);
    if (recent.isEmpty()) {
        text(cs::emptySongs, area.removeFromTop(s::button), s::body, s::muted);
        return;
    }
    const int visible = std::max(1, area.getHeight() / s::workspaceHeader);
    recentOffset = jlimit(0, std::max(0, recent.size() - visible), recentOffset);
    if (recent.size() > visible) {
        auto pagination = area.removeFromBottom(s::button);
        button(cs::prevTracks, pagination.removeFromLeft(s::clipWidth), [this, visible] {
            recentOffset -= std::max(1, visible - 1);
            triggerAsyncUpdate();
        })->setEnabled(recentOffset > 0);
        pagination.removeFromLeft(s::gap);
        button(cs::nextTracks, pagination.removeFromLeft(s::clipWidth), [this, visible] {
            recentOffset += std::max(1, visible - 1);
            triggerAsyncUpdate();
        })->setEnabled(recentOffset + visible < recent.size());
    }
    for (int i = recentOffset; i < recent.size() && area.getHeight() >= s::workspaceHeader; ++i) {
        const auto& dir = recent[i];
        auto line = area.removeFromTop(s::workspaceHeader).reduced(0, s::grid);
        auto project = Song::load(dir, damaged);
        auto label = project ? project->title : dir.getFileName();
        auto spot = line.removeFromLeft(line.getWidth() / 2);
        button(limit(skin.font(s::body, false, true), label, spot.getWidth() - 2 * s::inset), spot,
               [this, dir] { openProject(dir); })->setTooltip(label);
        line.removeFromLeft(s::inset);
        auto meta = dir.getChildFile("skladba.json").getLastModificationTime().formatted("%d. %m. %H:%M");
        if (project)
            meta = project->tuning + cs::divider + String(project->grid.bpm, 1) + " BPM" + cs::divider + meta;
        text(limit(skin.font(s::caption), meta, line.getWidth()), line, s::caption, s::muted);
    }
}
void Studio::makeWizard(Rect area) {
    workspaceHeading = area.removeFromTop(s::workspaceHeader);
    auto heading = workspaceHeading.reduced(s::inset, 0);
    text(cs::setup, heading.removeFromLeft(s::labelWidth), s::workspaceTitle, s::chalk, false, true);
    deviceStatus(heading);
    auto footer = area.removeFromBottom(s::workspaceFooter).reduced(s::inset, s::gap);
    inspectorBounds = area.removeFromRight(s::inspectorWidth);
    auto summary = inspectorBounds.reduced(s::inset);
    auto panelRow = [&](int height = s::inspectorRow) {
        auto bounds = summary.removeFromTop(height);
        summary.removeFromTop(s::inspectorGap);
        return bounds;
    };
    text(cs::setupSummary.toUpperCase(), panelRow(), s::section, s::navigationInk, false, true);
    auto item = [&](const String& title, const String& label, bool on) {
        text(title, panelRow(s::caption + s::grid), s::caption, s::muted);
        auto bounds = panelRow();
        text(limit(skin.font(s::body), label, bounds.getWidth()), bounds, s::body,
             on ? s::chalk : s::warning);
    };
    item(cs::device, link.online ? link.current.input : String(cs::disconnected), link.online);
    item(cs::formatLabel, link.online ? cs::frequency(link.current.rate) + cs::divider +
                                        cs::bufferSize(link.current.buffer) : cs::emptyDash, link.online);
    item(cs::delay, latencyLabel(), link.online);
    item(cs::titles[1], answer >= 0 ? cs::answers[answer] : cs::tonePending, answer == 0);
    item(cs::inputCount, String(link.caps.inputs.size()), !link.caps.inputs.isEmpty());
    button(settings.insurance ? cs::insuranceOn : cs::insuranceOff, panelRow(s::button),
           [this] { enableInsurance(); }, settings.insurance ? pillOn : pillOff);

    const bool last = page == Page::inputs;
    const auto advance = [this] {
        if (page == Page::inputs)
            finish();
        else
            show(static_cast<Page>((int)page + 1));
    };
    button(last ? cs::finish : cs::next, footer.removeFromRight(s::nameWidth), advance, primary);
    footer.removeFromRight(s::gap);
    button(cs::skip, footer.removeFromRight(s::levelWidth), advance);
    if (page != Page::device) {
        footer.removeFromRight(s::gap);
        button(cs::back, footer.removeFromRight(s::levelWidth),
               [this] { show(static_cast<Page>((int)page - 1)); });
    }
    if (page == Page::device && link.caps.separateClocks &&
        settings.clockOverride != Settings::calibrationKey(link.current)) {
        button(cs::clockOverride, footer.removeFromLeft(s::nameWidth), [this] {
            settings.clockOverride = Settings::calibrationKey(link.current);
            persist();
            triggerAsyncUpdate();
        });
        footer.removeFromLeft(s::gap);
    }
    if (page == Page::device) {
        button(cs::pedal, footer.removeFromLeft(s::clipWidth), [this] {
            pedalLearning = true;
            grabKeyboardFocus();
            status(cs::pedalLearn);
        });
        if (settings.pedalKey) {
            footer.removeFromLeft(s::gap);
            button(cs::pedalClear, footer.removeFromLeft(s::clipWidth), [this] {
                settings.pedalKey = 0;
                persist();
                status({});
            });
        }
    }
    area.reduce(s::inset, s::inset);
    auto steps = area.removeFromTop(s::workspaceHeader);
    const int width = (steps.getWidth() - 2 * s::grid) / 3;
    for (int i = 0; i < 3; ++i) {
        const auto destination = static_cast<Page>(int(Page::device) + i);
        button(cs::titles[i], steps.removeFromLeft(width), [this, destination] { show(destination); },
               destination == page ? primary : plain, String(i + 1));
        steps.removeFromLeft(s::grid);
    }
    area.removeFromTop(s::inset);
    auto notice = area.removeFromBottom(s::button);
    text(limit(skin.font(s::caption), message, notice.getWidth()), notice, s::caption, s::warning);
    switch (page) {
    case Page::device: devicePage(area); break;
    case Page::tone: tonePage(area); break;
    case Page::inputs: inputPage(area); break;
    default: break;
    }
}
void Studio::devicePage(Rect area) {
    auto row = [&] {
        auto r = area.removeFromTop(s::button);
        area.removeFromTop(s::gap);
        return r;
    };
    StringArray labels;
    int index = -1;
    for (auto& p : link.ports) {
        if (p.driver == link.current.driver)
            index = labels.size();
        labels.add(cs::drivers[(int)p.driver]);
    }
    combo(cs::driver, row(), labels, index, [this](int i) { choose({link.ports.at(i).driver}); });
    if (index >= 0) {
        const auto& p = link.ports[(size_t)index];
        if (!p.inputs.isEmpty())
            combo(p.separate ? cs::inputPort : cs::bothPort, row(), p.inputs,
                  p.inputs.indexOf(link.current.input), [this, names = p.inputs](int i) {
                      auto d = link.current;
                      d.input = names[i];
                      choose(d);
                  });
        if ((p.separate || p.inputs.isEmpty()) && !p.outputs.isEmpty())
            combo(cs::outputPort, row(), p.outputs, p.outputs.indexOf(link.current.output),
                  [this, names = p.outputs](int i) {
                      auto d = link.current;
                      d.output = names[i];
                      choose(d);
                  });
        if (link.online) {
            StringArray rates, buffers;
            for (auto n : link.caps.rates)
                rates.add(cs::frequency(n));
            for (auto n : link.caps.buffers)
                buffers.add(cs::bufferSize(n));
            combo(
                cs::rate, row(), rates, link.caps.rates.indexOf(link.current.rate),
                [this](int i) {
                    auto d = link.current;
                    d.rate = (int)link.caps.rates[i];
                    choose(d);
                },
                true);
            combo(
                cs::buffer, row(), buffers, link.caps.buffers.indexOf(link.current.buffer),
                [this](int i) {
                    auto d = link.current;
                    d.buffer = link.caps.buffers[i];
                    choose(d);
                },
                true);
        }
    }
    auto actions = row();
    button(cs::refresh, actions.removeFromLeft(s::nameWidth), [this] { refresh(true); });
    actions.removeFromLeft(s::gap);
    if (link.online && link.current.driver != Driver::asio && link.current.input.isNotEmpty())
        button(laptop ? cs::card : cs::laptop, actions.removeFromLeft(s::fieldWidth), [this] {
            bool enable = !laptop;
            if (enable)
                for (auto& p : link.ports)
                    if (p.driver == Driver::shared) {
                        DeviceChoice d = link.current;
                        d.driver = Driver::shared;
                        choose(d);
                        break;
                    }
            laptop = enable;
            settings.laptopInput = enable ? link.current.input : String{};
            persist();
            triggerAsyncUpdate();
        });
    text(!link.online               ? cs::connect
         : link.caps.separateClocks ? cs::clockConflict
                                    : (laptop ? cs::fallback : cs::hints[(int)link.current.driver]),
         area.removeFromTop(s::button), s::body, laptop || !link.online ? s::warning : s::muted);
    if (link.online)
        text(cs::bufferHelp, area.removeFromTop(s::button), s::caption, s::muted);
}
void Studio::tonePage(Rect area) {
    const auto outputs = link.caps.outputChannels;
    text(outputs == 0 ? cs::noOutput : (outputs == 1 ? cs::mono : cs::stereo),
         area.removeFromTop(s::transport));
    if (outputs == 0)
        return;
    auto action = area.removeFromTop(s::transport);
    button(
        played ? cs::again : cs::play, action.removeFromLeft(s::labelWidth),
        [this] {
            link.audio.playTone();
            played = true;
            answer = -1;
            triggerAsyncUpdate();
        },
        primary);
    text(cs::toneInfo, action.reduced(s::inset, 0), s::caption, s::muted, true);
    auto sequence = area.removeFromTop(s::workspaceHeader);
    const int width = (sequence.getWidth() - 2 * s::grid) / 3;
    for (const auto& label : {cs::toneLeft, cs::toneRight, cs::toneBoth}) {
        auto part = sequence.removeFromLeft(width);
        panels.push_back(part);
        text(label, part.withTrimmedLeft(s::inset), s::caption, s::muted, true);
        sequence.removeFromLeft(s::grid);
    }
    if (!played)
        return;
    area.removeFromTop(s::gutter);
    text(cs::question, area.removeFromTop(s::recordButton), s::heading, s::chalk, false, true);
    auto options = area.removeFromTop(s::button);
    for (int i = 0; i < 3; ++i) {
        button(
            cs::answers[i], options.removeFromLeft(s::clipWidth),
            [this, i] {
                answer = i;
                triggerAsyncUpdate();
            },
            i == answer ? selected : plain);
        options.removeFromLeft(s::gap);
    }
    if (answer >= 0)
        text(cs::advice[answer], area.removeFromTop(s::recordButton), s::body,
             answer == 0 ? s::ready : s::warning);
}
void Studio::inputPage(Rect area) {
    auto heading = area.removeFromTop(s::button);
    if (!midiPorts.isEmpty())
        button(cs::midiDrums, heading.removeFromRight(s::clipWidth), [this] { show(Page::midi); });
    text(cs::inputHelp, heading);
    auto options = area.removeFromTop(s::button);
    if (!link.caps.inputs.isEmpty()) {
        button(cs::drumTest, options.removeFromRight(s::clipWidth), [this] {
            resetDrums();
            show(Page::drums);
        });
        button(cs::findGuitar, options.removeFromRight(s::nameWidth), [this] {
            activityBeforePluck.clear();
            detectionTicks = 0;
            for (int i = 0; i < link.caps.inputs.size(); ++i)
                activityBeforePluck.push_back(link.audio.read(i).activity);
            status(cs::pluck);
        });
    }
    if (!link.caps.inputs.isEmpty()) {
        options.removeFromRight(s::gap);
        button(cs::source, options.removeFromRight(s::clipWidth), [this] {
            finish();
            show(Page::live);
        });
    }
    text(cs::clipHelp, area.removeFromTop(s::button), s::caption, s::muted);
    if (link.caps.inputs.isEmpty()) {
        text(cs::noInputs, area, s::body, s::warning);
        return;
    }
    viewport.setBounds(area);
    viewport.setVisible(true);
    const int count = link.caps.inputs.size();
    const int w = area.getWidth() - viewport.getScrollBarThickness();
    rows.setSize(w, count * s::row);
    for (int i = 0; i < count; ++i) {
        const auto key = Settings::inputKey(link.current, i);
        auto* row = new InputRow(skin, link, i, settings.aliases[key], [this, key](String name) {
            settings.aliases[key] = name;
            persist();
        });
        controls.add(row);
        rows.addAndMakeVisible(row);
        row->setBounds(0, i * s::row, w, s::row);
    }
}
bool Studio::keyStateChanged(bool down) {
    if (heldTransportKey && (!down || !KeyPress::isKeyCurrentlyDown(heldTransportKey)))
        heldTransportKey = 0;
    return false;
}
bool Studio::keyPressed(const KeyPress& key) {
    if (fileTask.valid())
        return true;
    const int code = key.getKeyCode();
    if (pedalLearning && page == Page::device) {
        pedalLearning = false;
        if (key != KeyPress::escapeKey && !key.getModifiers().isAnyModifierKeyDown()) {
            settings.pedalKey = code;
            persist();
            status(cs::pedalBound + " · " + key.getTextDescription());
        } else
            status({});
        return true;
    }
    const bool editable = dynamic_cast<TextEditor*>(Component::getCurrentlyFocusedComponent()) != nullptr;
    if (page == Page::metronome && !key.getModifiers().isAnyModifierKeyDown()) {
        if (key == KeyPress::spaceKey) {
            if (metroTapping)
                tapMetronome();
            else
                toggleMetronome();
            return true;
        }
        if (key == KeyPress::upKey || key == KeyPress::downKey) {
            changeMetroTempo(metroGrid.bpm + (key == KeyPress::upKey ? 1 : -1));
            return true;
        }
        if (key == KeyPress::escapeKey) {
            show(metroReturn);
            return true;
        }
    }
    if (!editable && !key.getModifiers().isAnyModifierKeyDown()) {
        if ((page == Page::drums || page == Page::midi) && key == KeyPress::escapeKey) {
            show(Page::inputs);
            return true;
        }
        if (page == Page::project && view.distant &&
            (key == KeyPress::escapeKey || key == KeyPress::F11Key)) {
            toggleDistant();
            return true;
        }
        const bool pedal = settings.pedalKey != 0 && code == settings.pedalKey;
        const bool keepAttempt =
            page == Page::project && view.editing && !view.distant &&
            (comparing || song.groupAt(selectedTrack(), selection().from, selection().to));
        const bool projectRecord =
            page == Page::project && (pedal || (code == KeyPress::returnKey && !keepAttempt));
        if (projectRecord || (page == Page::project && code == KeyPress::numberPad0) ||
            (pedal && (page == Page::home || page == Page::ideas))) {
            if (heldTransportKey == code && KeyPress::isKeyCurrentlyDown(code))
                return true;
            heldTransportKey = code;
            if (page == Page::project) {
                if (performance)
                    stopFromTransport();
                else
                    startProject(projectRecord);
            } else if (ideaRecording)
                stopIdea();
            else
                startIdea(false);
            return true;
        }
        if (page == Page::project && code == KeyPress::numberPadAdd) {
            chooseAttempt(false);
            return true;
        }
        if (page == Page::project && code == KeyPress::numberPadSubtract) {
            if (!performance) {
                auto bar = song.grid.beatAt(view.cursor) / song.grid.beats;
                view.cursor = song.barFrame((int)std::max<Frame>(0, bar - 1));
                view.save(projectDir);
                repaint();
            }
            return true;
        }
    }
    if (page == Page::ideas && !key.getModifiers().isAnyModifierKeyDown()) {
        if (dynamic_cast<TextEditor*>(Component::getCurrentlyFocusedComponent()) &&
            key != KeyPress::escapeKey)
            return false;
        auto code = CharacterFunctions::toUpperCase((juce_wchar)key.getKeyCode());
        if (code == 'R' || (key == KeyPress::spaceKey && ideaRecording)) {
            if (ideaRecording)
                stopIdea();
            else
                startIdea(false);
            return true;
        }
        if (code == 'U') {
            startIdea(true);
            return true;
        }
        if (key == KeyPress::spaceKey) {
            if (tapping)
                tapIdea();
            else
                playIdea(false);
            return true;
        }
    }
    if (page == Page::project && !namingPreset && editKey(key))
        return true;
    if (page == Page::project && namingPreset && soundDeckOpen) {
        if (key == KeyPress::escapeKey) {
            namingPreset = false;
            triggerAsyncUpdate();
            return true;
        }
        if (dynamic_cast<TextEditor*>(Component::getCurrentlyFocusedComponent()))
            return false;
    }
    if (page == Page::project) {
        if (!performance && (key == KeyPress::leftKey || key == KeyPress::rightKey)) {
            auto bar = song.grid.beatAt(view.cursor) / song.grid.beats;
            view.cursor =
                std::min(song.end(),
                         song.grid.beat(std::max<Frame>(0, bar + (key == KeyPress::leftKey ? -1 : 1)) *
                                        song.grid.beats));
            view.save(projectDir);
            repaint();
            return true;
        }
        if (key.getTextCharacter() == '+' || key.getTextCharacter() == '-') {
            view.zoom = jlimit(1, 64, key.getTextCharacter() == '+' ? view.zoom / 2 : view.zoom * 2);
            view.save(projectDir);
            repaint();
            return true;
        }
        if (key == KeyPress::F11Key) {
            toggleDistant();
            return true;
        }
        if (key == KeyPress::spaceKey || key.getKeyCode() == 'R') {
            if (performance)
                stopFromTransport();
            else
                startProject(key != KeyPress::spaceKey);
            return true;
        }
        if (key == KeyPress::homeKey && !performance) {
            view.cursor = 0;
            view.save(projectDir);
            repaint();
            return true;
        }
        if (key == KeyPress::escapeKey) {
            if (performance)
                stopProject();
            else if (view.distant)
                toggleDistant();
            else
                show(Page::home);
            return true;
        }
    }
    if (page == Page::newProject && key == KeyPress::returnKey) {
        for (auto* c : controls)
            if (auto* b = dynamic_cast<Button*>(c); b && b->getName() == cs::createSong) {
                b->onClick();
                return true;
            }
    }
    if (page != Page::customTuning && !key.getModifiers().isAnyModifierKeyDown() &&
        CharacterFunctions::toUpperCase((juce_wchar)key.getKeyCode()) == 'T' &&
        !dynamic_cast<TextEditor*>(Component::getCurrentlyFocusedComponent())) {
        toggleTuner();
        return true;
    }
    if (page >= Page::live && key == KeyPress::escapeKey) {
        show(page == Page::tuner ? tunerReturn
                                 : (page == Page::customTuning ? Page::tuner : Page::home));
        return true;
    }
    if (page != Page::home) {
        if (key == KeyPress::escapeKey) {
            finish();
            return true;
        }
        return false;
    }
    if (key == KeyPress::returnKey) {
        openProject(File(settings.lastSong));
        return true;
    }
    if (key.getModifiers().isAnyModifierKeyDown())
        return false;
    switch (CharacterFunctions::toUpperCase(static_cast<juce_wchar>(key.getKeyCode()))) {
    case 'R':
        startIdea(false);
        return true;
    case 'U':
        startIdea(true);
        return true;
    case 'N':
        newProject();
        return true;
    default:
        return false;
    }
}
StringArray Studio::layoutErrors() const {
    StringArray errors;
    for (auto& l : textLines)
        if (width(l.font, l.value) > l.bounds.getWidth() || l.font.getHeight() > l.bounds.getHeight() ||
            !getLocalBounds().contains(l.bounds))
            errors.add(l.value);
    for (auto* c : controls) {
        if (auto* a = dynamic_cast<Action*>(c))
            if ((!a->isRound() && width(a->titleFont(), a->shownTitle()) > a->titleArea().getWidth()) ||
                a->titleFont().getHeight() > a->getHeight() ||
                !getLocalBounds().contains(a->getBounds()))
                errors.add(a->getName());
        if (auto* b = dynamic_cast<ComboBox*>(c))
            if (width(const_cast<Theme&>(skin).getComboBoxFont(*b), b->getText()) >
                b->getWidth() - s::icon - 2 * s::gap)
                errors.add(b->getName());
        if (auto* row = dynamic_cast<InputRow*>(c))
            if (!row->fits())
                errors.add(cs::input);
    }
    appendFaderLayoutErrors(errors);
    return errors;
}
} // namespace rocker
