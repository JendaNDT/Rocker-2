#include "Studio.h"
#include "Strings.h"
#include <cmath>

namespace rocker {
using namespace juce;
namespace s = style;
namespace {
using Rect = Rectangle<int>;
Rect row(Rect& area, int height = s::button) {
    auto r = area.removeFromTop(height);
    area.removeFromTop(s::gap);
    return r;
}
String decimal(double n, int places = 1) { return String(n, places).replaceCharacter('.', ','); }
String ms(int n, int rate) { return decimal(1000. * n / rate) + cs::latencyUnit; }
bool wireless(const String& endpoint) {
    return endpoint.containsIgnoreCase("bluetooth") || endpoint.containsIgnoreCase("airpods") ||
           endpoint.containsIgnoreCase("hands-free") || endpoint.containsIgnoreCase("a2dp");
}
class Fader final : public Slider {
  public:
    Fader(Theme& t, const String& title, String suffix) : skin(t), unit(std::move(suffix)) {
        setName(title);
        setSliderStyle(LinearHorizontal);
        setTextBoxStyle(NoTextBox, false, 0, 0);
        setWantsKeyboardFocus(true);
    }
    void paint(Graphics& g) override {
        auto area = getLocalBounds().reduced(s::gap, 0);
        auto value = area.removeFromRight(s::faderReadout);
        g.setColour(s::chalk);
        g.setFont(skin.font(s::body, true));
        g.drawText(textFromValueFunction ? textFromValueFunction(getValue())
                                         : String((int)getValue()) + unit,
                   value, Justification::centredRight, false);
        auto bar = area.withSizeKeepingCentre(area.getWidth() - s::inset, s::grid);
        g.setColour(s::line);
        g.fillRect(bar);
        const int x = bar.getX() + roundToInt(bar.getWidth() * valueToProportionOfLength(getValue()));
        g.setColour(s::guitar);
        g.fillRect(bar.withRight(x));
        g.setColour(s::chalk);
        g.fillEllipse((float)(x - s::gap / 2), (float)(bar.getCentreY() - s::gap / 2), s::gap, s::gap);
        if (hasKeyboardFocus(true)) {
            g.setColour(s::selection);
            g.drawRect(getLocalBounds(), s::hairline);
        }
    }
    void mouseDown(const MouseEvent& e) override {
        grabKeyboardFocus();
        setFromX(e.x);
    }
    void mouseDrag(const MouseEvent& e) override { setFromX(e.x); }
    bool keyPressed(const KeyPress& k) override {
        if (k == KeyPress::leftKey || k == KeyPress::rightKey) {
            setValue(getValue() + (k == KeyPress::leftKey ? -1 : 1), sendNotificationSync);
            return true;
        }
        return Slider::keyPressed(k);
    }

  private:
    Theme& skin;
    String unit;
    void setFromX(int x) {
        auto area = getLocalBounds()
                        .reduced(s::gap, 0)
                        .withTrimmedRight(s::faderReadout)
                        .reduced(s::inset / 2, 0);
        setValue(proportionOfLengthToValue(
                     jlimit(0., 1., double(x - area.getX()) / std::max(1, area.getWidth()))),
                 sendNotificationSync);
    }
};
} // namespace
void Studio::resetRoute() {
    measurementInput = 0;
    guitarSource = 0;
    primaryInput = 0;
    secondaryInput = 1;
    hearSecond = false;
    for (int i = 0; i < link.caps.inputs.size(); ++i) {
        auto it = settings.aliases.find(Settings::inputKey(link.current, i));
        if (it != settings.aliases.end() && it->second.equalsIgnoreCase(cs::guitar)) {
            primaryInput = i;
            break;
        }
    }
    secondaryInput = primaryInput == 0 ? 1 : 0;
    link.audio.ideaPort = primaryInput;
    link.audio.setInsurance(settings.insurance);
    link.audio.live.selectInput(primaryInput);
    link.audio.live.setAmp(sound);
}
int Studio::heardInput() const {
    return guitarSource == 3 && hearSecond ? secondaryInput : primaryInput;
}
StringArray Studio::inputNames() const {
    StringArray result;
    for (int i = 0; i < link.caps.inputs.size(); ++i) {
        auto name = cs::channel(i);
        auto it = settings.aliases.find(Settings::inputKey(link.current, i));
        if (it != settings.aliases.end() && it->second.isNotEmpty())
            name += "  /  " + it->second;
        result.add(name);
    }
    return result;
}
void Studio::changeMonitor(int mode) {
    const bool ampAllowed = (guitarSource == 0 || (guitarSource == 3 && !hearSecond)) && !laptop;
    if (mode == (int)Monitor::amp && !ampAllowed)
        mode = (int)Monitor::off;
    link.audio.live.selectInput(heardInput());
    link.audio.live.monitor = static_cast<Monitor>(mode);
    triggerAsyncUpdate();
}
void Studio::toggleTuner() { show(page == Page::tuner ? tunerReturn : Page::tuner); }
void Studio::resetTuning() {
    confirmed.assign(tunings[tuningIndex].notes.size(), false);
    nearestString = 0;
    stableString = -1;
    stableTime = 0;
    lastVoiced = 0;
    readingHz = cents = 0;
    if (targetString >= (int)confirmed.size())
        targetString = -1;
}
int Studio::listeningDelaySamples() const {
    if (!link.online)
        return -1;
    const auto saved = settings.latency.find(Settings::calibrationKey(link.current));
    int delay = link.caps.inputLatency + link.caps.outputLatency;
    if (saved != settings.latency.end())
        delay = saved->second.samples;
    else if (delay <= 0)
        return -1;
    return delay + (link.audio.live.monitor == Monitor::amp ? link.audio.live.ampDelay() : 0);
}
String Studio::latencyLabel() const {
    const int samples = listeningDelaySamples();
    if (samples < 0)
        return cs::unknown;
    const bool measured = settings.latency.contains(Settings::calibrationKey(link.current));
    return (measured ? cs::measured : cs::estimated) + "  " + ms(samples, link.current.rate) + "  /  " +
           cs::latencyRating(1000. * samples / link.current.rate) +
           (link.audio.live.monitor == Monitor::amp ? "  (" + cs::ampDelay + ")" : "");
}
void Studio::dynamicText(Readout id, String value, Rect r, int size, bool mono) {
    dynamicLines[id] = textLines.size();
    text(value, r, size, s::chalk, mono);
}
void Studio::setReadout(Readout id, const String& value) {
    auto it = dynamicLines.find(id);
    if (it == dynamicLines.end())
        return;
    auto& item = textLines[it->second];
    if (item.value != value) {
        item.value = value;
        repaint(item.bounds);
    }
}
void Studio::pollLive() {
    auto& live = link.audio.live;
    if (auto limit = live.takeThreshold();
        limit && (page == Page::amp || (page == Page::project && soundDeckOpen))) {
        if (page == Page::project) {
            auto& a = song.tracks[view.selected].amp;
            a.values.gate = static_cast<std::uint8_t>(*limit);
            a.modified = true;
            applyTrackSound(false);
        } else {
            sound.gate = static_cast<std::uint8_t>(*limit);
            live.setAmp(sound, false);
        }
        status(cs::gateSet);
    }
    if (page == Page::diagnostics) {
        if (auto result = live.takeMeasurement()) {
            if (result->samples < 0)
                status(cs::noReturn);
            else {
                settings.latency[Settings::calibrationKey(link.current)] = {
                    result->samples, link.caps.inputLatency + link.caps.outputLatency};
                status(cs::cableGain);
                persist();
            }
        }
        setReadout(Readout::cpu, decimal(live.cpuPercent.load()) + cs::percentUnit);
    }
    if (page == Page::live) {
        auto in = link.audio.read(heardInput());
        setReadout(Readout::inputLevel,
                   in.clipped ? cs::clip
                              : (in.peak > 0 ? decimal(decibels(in.peak)) + cs::dbUnit : cs::silence));
        repaint(inputBar);
        repaint(outputBar);
        float peak = live.outputPeak.load();
        setReadout(Readout::level, peak > 0 ? decimal(decibels(peak)) + cs::dbUnit : cs::silence);
    }
    if (page != Page::tuner)
        return;
    auto pitch = live.takePitch();
    const double now = Time::getMillisecondCounterHiRes();
    if (pitch && *pitch > 0) {
        nearestString = targetString >= 0 ? targetString : tunings[tuningIndex].nearest(*pitch);
        const int note = tunings[tuningIndex].notes[nearestString];
        cents = Pitch::cents(*pitch, note);
        needleCents =
            (readingHz == 0 || now - lastVoiced > 600) ? cents : 0.45 * needleCents + 0.55 * cents;
        readingHz = *pitch;
        lastVoiced = now;
        if (std::abs(cents) <= 5) {
            if (stableString != nearestString)
                stableTime = 0;
            stableString = nearestString;
            stableTime += live.pitchFrameSeconds();
            if (stableTime >= 0.4 && !confirmed[nearestString]) {
                confirmed[nearestString] = true;
                live.cue(880, true);
            }
        } else {
            stableString = -1;
            stableTime = 0;
        }
        const auto offset =
            std::abs(cents) < 0.05 ? String(u8"±0,0") : (cents > 0 ? "+" : "") + decimal(cents);
        setReadout(Readout::pitch, Tuning::noteName(note) + "   " + decimal(readingHz) + " Hz   " +
                                       offset + cs::centUnit);
        setReadout(Readout::hint,
                   std::abs(cents) <= 5 ? cs::inTune : (cents < 0 ? cs::flat : cs::sharp));
        StringArray done;
        for (size_t i = 0; i < confirmed.size(); ++i)
            done.add((confirmed[i] ? String(u8"✓ ") : String{}) +
                     Tuning::noteName(tunings[tuningIndex].notes[i]));
        setReadout(Readout::confirmed, done.joinIntoString("   "));
        repaint(needleArea);
    } else {
        if (pitch) {
            stableString = -1;
            stableTime = 0;
        }
        if (now - lastVoiced > 600 && readingHz > 0) {
            readingHz = 0;
            setReadout(Readout::pitch, cs::emptyDash);
            setReadout(Readout::hint, cs::waiting);
            repaint(needleArea);
        }
    }
}
void Studio::livePages(Rect area) {
    auto footer = area.removeFromBottom(s::footer).reduced(0, (s::footer - s::button) / 2);
    const std::array<Page, 6> pages{Page::home, Page::device, Page::live,
                                    Page::amp,  Page::tuner,  Page::diagnostics};
    const std::array<String, 6> labels{cs::home,    cs::openSetup, cs::livePage,
                                       cs::ampPage, cs::tuner,     cs::diagnostics};
    const int w = (footer.getWidth() - 5 * s::gap) / 6;
    for (int i = 0; i < 6; ++i) {
        button(
            labels[i], footer.removeFromLeft(w), [this, p = pages[i]] { show(p); },
            pages[i] == page ? 1 : 0);
        footer.removeFromLeft(s::gap);
    }
    if (projectDir != File{} && song.title.isNotEmpty())
        button(cs::useProject, area.removeFromBottom(s::button).withWidth(s::nameWidth),
               [this] { show(Page::project); });
    area.reduce(0, s::gutter);
    const String title = page == Page::live           ? cs::hear
                         : page == Page::amp          ? cs::ampPage
                         : page == Page::tuner        ? cs::tuner
                         : page == Page::customTuning ? cs::custom
                                                      : cs::rig;
    text(title, row(area, s::recordButton), s::title, s::chalk, false, true);
    auto notice = area.removeFromBottom(s::transport);
    text(message, notice, s::body, s::warning);
    panels.push_back(area);
    area.reduce(s::gutter, s::inset);
    switch (page) {
    case Page::live:
        playPanel(area);
        break;
    case Page::amp:
        ampPanel(area);
        break;
    case Page::tuner:
        tunerPanel(area);
        break;
    case Page::customTuning:
        customPanel(area);
        break;
    case Page::diagnostics:
        diagnosticPanel(area);
        break;
    default:
        break;
    }
}
void Studio::playPanel(Rect area) {
    auto left = area.removeFromLeft((area.getWidth() - s::gutter) / 2);
    area.removeFromLeft(s::gutter);
    auto names = inputNames();
    if (!names.isEmpty()) {
        auto sources = cs::sources;
        if (names.size() < 2)
            sources.remove(3);
        combo(cs::source, row(left), sources, guitarSource, [this](int i) {
            guitarSource = i;
            hearSecond = false;
            changeMonitor(0);
        });
        combo(cs::directIn, row(left), names, primaryInput, [this](int i) {
            if (i == secondaryInput)
                secondaryInput = primaryInput;
            primaryInput = i;
            changeMonitor(0);
        });
        if (guitarSource == 3) {
            auto available = names;
            available.remove(primaryInput);
            combo(cs::ampIn, row(left), available, secondaryInput - (secondaryInput > primaryInput),
                  [this](int i) {
                      secondaryInput = i + (i >= primaryInput);
                      changeMonitor(0);
                  });
            combo(cs::listeningTo, row(left), {cs::directIn, cs::ampIn}, hearSecond ? 1 : 0,
                  [this](int i) {
                      hearSecond = i != 0;
                      changeMonitor(0);
                  });
        }
    }
    if (link.caps.outputChannels > 0 && !names.isEmpty()) {
        auto modes = cs::monitors;
        if (laptop || guitarSource == 1 || guitarSource == 2 || hearSecond)
            modes.remove(2);
        combo(cs::listen, row(left), modes, (int)link.audio.live.monitor.load(),
              [this](int i) { changeMonitor(i); });
    } else
        text(cs::needIO, row(left), s::body, s::warning);
    text(cs::headphones.toUpperCase(), row(area), s::section, s::muted, false, true);
    auto mode = link.audio.live.monitor.load();
    text(mode == Monitor::off
             ? cs::monitorOff
             : (mode == Monitor::clean ? cs::drySignal
                                       : cs::wetSignal + "  /  " + cs::presetNames[presetIndex] +
                                             (sound == presets[presetIndex] ? "" : "*")),
         row(area), s::heading);
    auto meter = [&](Readout id, String name, Rect& bar) {
        auto r = row(area);
        text(name, r.removeFromLeft(s::levelWidth), s::caption, s::muted);
        auto digits = r.removeFromRight(s::levelWidth);
        bar = r.reduced(s::gap, 0);
        dynamicText(id, cs::silence, digits, s::body, true);
    };
    meter(Readout::inputLevel, cs::channel(heardInput()), inputBar);
    meter(Readout::level, cs::outputLevel, outputBar);
    text(cs::delay, row(area), s::caption, s::muted);
    text(latencyLabel(), row(area), s::body);
    // Full-width routing advice below the two columns.
    auto advice = Rect(left.getX(), std::max(left.getY(), area.getY()) + s::gap,
                       area.getRight() - left.getX(), s::button);
    routeSymbol = advice.removeFromLeft(guitarSource == 3 ? 2 * s::icon + s::gap : s::icon)
                      .withWidth(s::icon)
                      .withSizeKeepingCentre(s::icon, s::icon);
    advice.removeFromLeft(s::gap);
    text(laptop ? cs::fallback : (guitarSource == 0 ? cs::directHelp : cs::comboHelp), advice, s::body,
         s::muted);
    if (wireless(link.current.output))
        text(cs::bluetooth, advice.translated(0, s::transport), s::body, s::warning);
    else if (listeningDelaySamples() > link.current.rate * 0.015)
        text(cs::directOption, advice.translated(0, s::transport), s::body, s::warning);
}
void Studio::fader(const String& title, Rect area, int low, int high, int initial, const String& unit,
                   std::function<void(int)> apply) {
    text(title, area.removeFromLeft(std::min(s::levelWidth, area.getWidth() / 3)), s::body, s::muted);
    auto* slider = new Fader(skin, title, unit);
    controls.add(slider);
    addAndMakeVisible(slider);
    slider->setBounds(area);
    slider->setRange(low, high, 1);
    slider->setValue(initial, dontSendNotification);
    slider->onValueChange = [this, slider, apply] {
        apply(roundToInt(slider->getValue()));
        if (page == Page::amp)
            link.audio.live.setAmp(sound, false);
    };
}
void Studio::mixSlider(const String& title, Rect bounds, float initial, bool pan,
                       std::function<void(float)> set) {
    auto* slider = new Fader(skin, title, {});
    controls.add(slider);
    addAndMakeVisible(slider);
    slider->setBounds(bounds);
    slider->setRange(pan ? -1. : -96., pan ? 1. : 6., pan ? .01 : .1);
    if (!pan)
        slider->setSkewFactorFromMidPoint(-12.);
    slider->setDoubleClickReturnValue(true, 0.);
    slider->setValue(initial, dontSendNotification);
    slider->textFromValueFunction = [pan](double v) {
        if (pan)
            return std::abs(v) < .005
                       ? cs::centre
                       : (v < 0 ? cs::leftShort : cs::rightShort) + String(roundToInt(std::abs(v) * 100));
        return v <= -96 ? cs::silentGain : decimal(v) + cs::dbUnit;
    };
    slider->onValueChange = [this, slider, set] {
        set(float(slider->getValue()));
        if (performance)
            performance->playback.updateMix(song);
        pendingSoundSave = Time::getMillisecondCounterHiRes() + 400;
        repaint();
    };
}
void Studio::ampPanel(Rect area) {
    auto top = row(area);
    combo(cs::preset, top.removeFromLeft(top.getWidth() / 2), cs::presetNames, presetIndex,
          [this](int i) {
              presetIndex = i;
              sound = presets[i];
              link.audio.live.setAmp(sound);
              triggerAsyncUpdate();
          });
    top.removeFromLeft(s::gutter);
    const bool sourceAllowsAmp = guitarSource == 0 || (guitarSource == 3 && !hearSecond);
    const bool allowed =
        !laptop && sourceAllowsAmp && !link.caps.inputs.isEmpty() && link.caps.outputChannels > 0;
    if (allowed)
        button(link.audio.live.monitor == Monitor::amp ? cs::soundOff : cs::soundOn,
               top.removeFromLeft(s::nameWidth),
               [this] { changeMonitor(link.audio.live.monitor == Monitor::amp ? 0 : 2); });
    else
        text(laptop ? cs::fallback : cs::comboHelp, top, s::caption, s::muted);
    area.removeFromTop(s::inset);
    auto left = area.removeFromLeft((area.getWidth() - s::gutter) / 2);
    area.removeFromLeft(s::gutter);
    fader(cs::gate, row(left), -84, -24, -sound.gate, cs::dbUnit, [this](int v) { sound.gate = -v; });
    if (!link.caps.inputs.isEmpty())
        button(cs::noise, row(left).withWidth(s::nameWidth), [this] {
            link.audio.live.learnNoise();
            status(cs::quiet);
        });
    combo(cs::shape, row(left), cs::shapes, sound.shape, [this](int v) {
        sound.shape = v;
        link.audio.live.setAmp(sound, false);
    });
    fader(cs::drive, row(left), 0, 100, sound.drive, cs::percentUnit,
          [this](int v) { sound.drive = v; });
    fader(cs::echo, row(left), 0, 100, sound.echo, cs::percentUnit, [this](int v) { sound.echo = v; });
    fader(cs::room, row(area), 0, 100, sound.room, cs::percentUnit, [this](int v) { sound.room = v; });
    text(cs::cabinet, row(left), s::caption, s::muted);
    fader(cs::bass, row(area), 0, 100, sound.bass, cs::percentUnit, [this](int v) { sound.bass = v; });
    fader(cs::middle, row(area), 0, 100, sound.mid, cs::percentUnit, [this](int v) { sound.mid = v; });
    fader(cs::treble, row(area), 0, 100, sound.treble, cs::percentUnit,
          [this](int v) { sound.treble = v; });
    fader(cs::presence, row(area), 0, 100, sound.presence, cs::percentUnit,
          [this](int v) { sound.presence = v; });
    fader(cs::volume, row(area), 0, 100, sound.volume, cs::percentUnit,
          [this](int v) { sound.volume = v; });
}
void Studio::openTrackSound(int index) {
    if (index != view.selected) {
        projectMonitor = 0;
        view.selected = index;
        configureSongMonitor();
        view.save(projectDir);
    }
    soundDeckOpen = true;
    namingPreset = false;
    triggerAsyncUpdate();
}

void Studio::applyTrackSound(bool recall) {
    auto& amp = song.tracks[view.selected].amp;
    if (performance)
        performance->playback.setSound(view.selected, amp, recall);
    link.audio.live.setAmp(amp.values, recall);
    if (projectMonitor == 2)
        link.audio.live.monitor = amp.enabled ? Monitor::amp : Monitor::clean;
    if (soundBadge != nullptr) {
        auto label = amp.enabled ? amp.name : cs::bypass;
        auto font = skin.font(s::body, false, true);
        while (label.isNotEmpty() && GlyphArrangement::getStringWidth(font, label + "*") >
                                         soundBadge->getWidth() - 2 * s::inset)
            label = label.dropLastCharacters(1);
        soundBadge->setName(label + (amp.enabled && amp.modified ? "*" : ""));
        soundBadge->repaint();
    }
    if (recall)
        saveSong();
    else
        pendingSoundSave = Time::getMillisecondCounterHiRes() + 1200;
}

void Studio::trackSoundDeck(Rect area) {
    auto& track = song.tracks[view.selected];
    auto& amp = track.amp;
    auto header = row(area);
    StringArray names = cs::presetNames;
    for (const auto& preset : settings.guitarPresets)
        names.add(preset.name);
    if (namingPreset) {
        button(cs::back, header.removeFromRight(s::levelWidth), [this] {
            namingPreset = false;
            triggerAsyncUpdate();
        });
        header.removeFromRight(s::gap);
        button(cs::storePreset, header.removeFromRight(s::nameWidth), [this, names] {
            auto title = presetDraft.trim();
            if (title.isEmpty() || names.contains(title, true)) {
                status(cs::nameTaken);
                return;
            }
            auto preset = song.tracks[view.selected].amp;
            preset.name = title;
            preset.enabled = true;
            preset.modified = false;
            settings.guitarPresets.push_back(preset);
            if (!settings.save(settingsFile)) {
                settings.guitarPresets.pop_back();
                status(cs::writeError);
                return;
            }
            song.tracks[view.selected].amp = preset;
            namingPreset = false;
            applyTrackSound(true);
            triggerAsyncUpdate();
        });
        field(cs::presetTitle, header, presetDraft);
    } else {
        text(cs::ampPage, header.removeFromLeft(s::clipWidth), s::heading);
        button(cs::useProject, header.removeFromRight(s::clipWidth), [this] {
            soundDeckOpen = false;
            triggerAsyncUpdate();
        });
        header.removeFromRight(s::gap);
        button(cs::savePreset, header.removeFromRight(s::nameWidth), [this] {
            namingPreset = true;
            presetDraft = {};
            triggerAsyncUpdate();
        });
        // An embedded copy can survive removal of its source from the preset library.
        names.addIfNotAlreadyThere(amp.name);
        combo(cs::preset, header.removeFromLeft(s::clipWidth * 2), names, names.indexOf(amp.name),
              [this](int i) {
                  auto& stored = song.tracks[view.selected].amp;
                  if (i < (int)presets.size())
                      stored = {cs::presetNames[i], presets[i], true, false};
                  else if (i - presets.size() < settings.guitarPresets.size())
                      stored = settings.guitarPresets[i - presets.size()];
                  stored.enabled = true;
                  applyTrackSound(true);
                  triggerAsyncUpdate();
              });
        header.removeFromLeft(s::gap);
        button(amp.enabled ? cs::bypass : cs::useAmp, header.withWidth(s::clipWidth), [this] {
            auto& a = song.tracks[view.selected].amp;
            a.enabled = !a.enabled;
            applyTrackSound(true);
            triggerAsyncUpdate();
        });
    }
    const int width = (area.getWidth() - 3 * s::gap) / 4;
    auto column = [&] {
        auto part = area.removeFromLeft(width);
        area.removeFromLeft(s::gap);
        return part;
    };
    auto input = column(), eq = column(), cab = column();
    auto control = [this, &amp](Rect& col, const String& name, std::uint8_t AmpParameters::* member) {
        const bool gate = member == &AmpParameters::gate;
        fader(name, row(col), gate ? -84 : 0, gate ? -24 : 100, (gate ? -1 : 1) * (amp.values.*member),
              gate ? cs::dbUnit : cs::percentUnit, [this, member, gate](int value) {
                  auto& a = song.tracks[view.selected].amp;
                  a.values.*member = static_cast<std::uint8_t>(gate ? -value : value);
                  a.modified = true;
                  applyTrackSound(false);
              });
    };
    control(input, cs::gate, &AmpParameters::gate);
    if (link.caps.inputs.contains(track.inputs[0]))
        button(cs::noise, row(input), [this] {
            link.audio.live.learnNoise();
            status(cs::quiet);
        });
    combo(cs::shape, row(input), cs::shapes, amp.values.shape, [this](int value) {
        auto& a = song.tracks[view.selected].amp;
        a.values.shape = value;
        a.modified = true;
        applyTrackSound(false);
    });
    control(input, cs::drive, &AmpParameters::drive);
    control(eq, cs::bass, &AmpParameters::bass);
    control(eq, cs::middle, &AmpParameters::mid);
    control(eq, cs::treble, &AmpParameters::treble);
    control(eq, cs::presence, &AmpParameters::presence);
    text(cs::speaker, row(cab), s::section, s::muted);
    text(cs::builtInCab, row(cab));
    control(cab, cs::echo, &AmpParameters::echo);
    control(area, cs::room, &AmpParameters::room);
    control(area, cs::volume, &AmpParameters::volume);
    if (!laptop && link.caps.inputs.contains(track.inputs[0]) && link.caps.outputChannels > 0)
        button(projectMonitor == 2 ? cs::soundOff : cs::soundOn, row(area), [this] {
            projectMonitor = projectMonitor == 2 ? 0 : 2;
            configureSongMonitor();
            triggerAsyncUpdate();
        });
}

void Studio::tunerPanel(Rect area) {
    auto options = row(area);
    StringArray names;
    for (const auto& tuning : tunings)
        names.add(tuning.name);
    combo(cs::tuning, options.removeFromLeft(options.getWidth() / 2), names, tuningIndex, [this](int i) {
        tuningIndex = i;
        targetString = -1;
        settings.tuningName = tunings[i].name;
        if (tunerReturn == Page::project) {
            song.tuning = settings.tuningName;
            saveSong();
        }
        persist();
        resetTuning();
        link.audio.live.setTuner(true);
        triggerAsyncUpdate();
    });
    options.removeFromLeft(s::gutter);
    button(cs::custom, options.removeFromLeft(s::nameWidth), [this] { show(Page::customTuning); });
    auto target = row(area);
    StringArray strings{cs::automatic};
    const auto& notes = tunings[tuningIndex].notes;
    for (size_t i = 0; i < notes.size(); ++i)
        strings.add(String((int)(notes.size() - i)) + "  /  " + Tuning::noteName(notes[i]));
    combo(cs::string, target.removeFromLeft(target.getWidth() / 2), strings, targetString + 1,
          [this](int i) {
              targetString = i - 1;
              stableString = -1;
              stableTime = 0;
              readingHz = 0;
              link.audio.live.setTuner(true);
              triggerAsyncUpdate();
          });
    target.removeFromLeft(s::gutter);
    if (link.caps.outputChannels > 0) {
        auto* reference = button(cs::ref, target.removeFromLeft(s::nameWidth), [] {});
        reference->onStateChange = [this, reference, playing = false]() mutable {
            if (reference->isDown() == playing)
                return;
            playing = reference->isDown();
            if (playing) {
                int i = targetString >= 0 ? targetString : nearestString;
                link.audio.live.cue(Pitch::frequency(tunings[tuningIndex].notes[i]));
            } else
                link.audio.live.endCue();
        };
    }
    if (!link.caps.inputs.isEmpty())
        combo(cs::input, row(area), inputNames(), link.audio.live.input.load(), [this](int i) {
            link.audio.live.selectInput(i);
            resetTuning();
            triggerAsyncUpdate();
        });
    dynamicText(Readout::pitch, cs::emptyDash, row(area, s::recordButton), s::title, true);
    needleArea = row(area).reduced(s::inset, 0);
    dynamicText(Readout::hint, link.caps.inputs.isEmpty() ? cs::noInputs : cs::waiting, row(area),
                s::body, false);
    text(cs::tunedStrings, row(area), s::section, s::muted, false, true);
    StringArray labels;
    for (auto note : notes)
        labels.add(Tuning::noteName(note));
    dynamicText(Readout::confirmed, labels.joinIntoString("   "), row(area), s::body, true);
}
void Studio::customPanel(Rect area) {
    auto field = [&](const String& label, String& value) {
        auto r = row(area);
        text(label, r.removeFromLeft(s::labelWidth), s::body, s::muted);
        auto* editor = new TextEditor;
        controls.add(editor);
        addAndMakeVisible(editor);
        editor->setBounds(r.withWidth(s::fieldWidth));
        editor->setName(label);
        editor->setFont(skin.font(s::body));
        editor->setInputRestrictions(64);
        editor->setText(value, false);
        editor->onTextChange = [editor, &value] { value = editor->getText(); };
    };
    field(cs::tuningName, customName);
    field(cs::userNotes, customNotes);
    text(cs::notesHint, row(area), s::body, s::muted);
    button(
        cs::saveTuning, row(area).withWidth(s::nameWidth),
        [this] {
            auto name = customName.trim().substring(0, 48);
            auto notes = Tuning::parse(customNotes);
            if (name.isEmpty() || notes.empty() || cs::tuningNames.contains(name, true)) {
                status(cs::badTuning);
                return;
            }
            auto found = std::find_if(settings.tunings.begin(), settings.tunings.end(),
                                      [&](const auto& t) { return t.name.equalsIgnoreCase(name); });
            Tuning tuning{name, notes};
            if (found == settings.tunings.end())
                settings.tunings.push_back(tuning);
            else
                *found = tuning;
            tunings = Tuning::factory();
            tunings.insert(tunings.end(), settings.tunings.begin(), settings.tunings.end());
            for (int i = 0; i < (int)tunings.size(); ++i)
                if (tunings[i].name == name)
                    tuningIndex = i;
            settings.tuningName = name;
            if (tunerReturn == Page::project) {
                song.tuning = settings.tuningName;
                saveSong();
            }
            targetString = -1;
            show(Page::tuner);
            message = cs::customSaved;
            persist();
        },
        1);
}
void Studio::diagnosticPanel(Rect area) {
    auto advice = area.removeFromBottom(s::button * 2);
    text(cs::cableHelp, advice.removeFromTop(s::button), s::caption, s::muted);
    text(wireless(link.current.output) ? cs::bluetooth : cs::cableGain, advice, s::caption, s::warning);
    auto left = area.removeFromLeft((area.getWidth() - s::gutter) / 2);
    area.removeFromLeft(s::gutter);
    auto driver = row(left);
    text(cs::driver, driver.removeFromLeft(s::levelWidth), s::section, s::muted, false, true);
    text(link.online ? cs::drivers[(int)link.current.driver] : cs::disconnected, driver, s::body);
    text(cs::io + "   " + String(link.caps.inputs.size()) + " / " + String(link.caps.outputChannels),
         row(left));
    text(link.online ? cs::frequency(link.current.rate) + "   /   " + cs::bufferSize(link.current.buffer)
                     : cs::emptyDash,
         row(left), s::body, s::muted, true);
    text(SystemStats::getCpuModel().substring(0, 48) + "   /   " + String(SystemStats::getNumCpus()),
         row(left));
    auto cpu = row(left);
    text(cs::load, cpu.removeFromLeft(s::labelWidth), s::caption, s::muted);
    dynamicText(Readout::cpu, {}, cpu, s::body, true);
    const int reported = link.caps.inputLatency + link.caps.outputLatency;
    text(cs::estimated, row(area), s::section, s::muted, false, true);
    text(reported > 0 ? ms(reported, link.current.rate) + "   /   " + String(reported) + cs::samples
                      : cs::unknown,
         row(area), s::heading);
    const auto saved = settings.latency.find(Settings::calibrationKey(link.current));
    text(cs::measured +
             (saved != settings.latency.end() && saved->second.reported > 0
                  ? "    /    " + cs::delta + " " + String(saved->second.correction()) + cs::samples
                  : String{}),
         row(area), s::body, s::muted, false, true);
    text(saved != settings.latency.end() ? ms(saved->second.samples, link.current.rate) + "   /   " +
                                               String(saved->second.samples) + cs::samples
                                         : cs::notMeasured,
         row(area), s::body);

    if (activeMidi.isNotEmpty() && !link.caps.inputs.isEmpty())
        button(cs::midiCompare, row(left).withWidth(s::labelWidth), [this] { compareMidi(); });
    button(cs::checkClick, row(left).withWidth(s::labelWidth), [this] {
        findClick();
    })->setEnabled(!clickCheck.valid());
    if (testingLoad || diagnosing) {
        button(cs::stopSong, row(area).withWidth(s::nameWidth), [this] { stopProject(); });
        return;
    }
    if (link.online && link.caps.outputChannels > 0 &&
        link.audio.live.measurement() == Measurement::idle) {
        auto perf = row(left);
        StringArray counts;
        for (int n = 1; n <= 8; ++n)
            counts.add(String(n));
        button(cs::loadTrial, perf.removeFromRight(s::nameWidth), [this] { startLoadTrial(); });
        combo(cs::ampCount, perf, counts, trialAmps - 1, [this](int i) { trialAmps = i + 1; });
    }
    if (link.online && link.caps.outputChannels > 0 && !link.caps.inputs.isEmpty() &&
        link.audio.live.measurement() == Measurement::idle) {
        combo(cs::input, row(area), inputNames(), measurementInput, [this](int i) {
            measurementInput = i;
            link.audio.live.selectInput(i);
        });
        auto actions = row(area);
        button(cs::syncTitle, actions.removeFromRight(s::nameWidth), [this] { syncTest(); });
        button(cs::measure, actions.withWidth(s::nameWidth), [this] {
            if (link.audio.live.startMeasurement())
                status(cs::measuring);
        });
    }
}
} // namespace rocker
