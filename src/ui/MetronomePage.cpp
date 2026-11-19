#include "Studio.h"
#include "Strings.h"
#include <numeric>

namespace rocker {
using namespace juce;
namespace s = style;

void Studio::openMetronome() {
    if (performance && performance->recording)
        return;
    metroReturn = page;
    metroForSong = page == Page::project;
    metroGrid = metroForSong ? song.grid : BeatGrid{48000, ideaBeats, ideaUnit, ideaTempo};
    if (link.online)
        metroGrid.rate = link.current.rate;
    metroGuide = metroForSong ? (view.click ? (view.rhythm ? 1 : 0) : 2)
                              : (ideaClick ? (ideaRhythm ? 1 : 0) : 2);
    if (metroReturn == Page::home)
        metroGuide = 0;
    metroCountIn = !metroForSong || view.countIn;
    metroTapping = false;
    metroTaps.clear();
    show(Page::metronome);
}

void Studio::stopMetronome() {
    if (metronome) {
        link.audio.timeline.detach();
        metronome.reset();
    }
}

void Studio::startMetronome() {
    if (!link.online || link.caps.outputChannels == 0)
        return;
    Song clockSong;
    clockSong.grid = metroGrid;
    metronome = std::make_unique<Performance>(
        clockSong, AudioLibrary{}, 0, false, metroGuide != 2, 0, metroGuide == 1,
        LoopSpan{0, metroGrid.beat(metroGrid.beats), 0});
    metronome->configureClick(settings.clickVolume, settings.clickSound);
    link.audio.timeline.attach(*metronome);
}

void Studio::toggleMetronome() {
    if (metronome)
        stopMetronome();
    else
        startMetronome();
    triggerAsyncUpdate();
}

void Studio::changeMetroTempo(double bpm, bool rebuild) {
    if (metroGrid.bpm == std::clamp(bpm, 30., 300.))
        return;
    const bool restart = bool(metronome);
    stopMetronome();
    metroGrid.bpm = std::clamp(bpm, 30., 300.);
    if (restart)
        startMetronome();
    if (rebuild)
        triggerAsyncUpdate();
    else
        repaint();
}

void Studio::tapMetronome() {
    const auto now = Time::getMillisecondCounterHiRes();
    const double multiplier = 4. / metroGrid.unit;
    if (!metroTaps.empty() && now - metroTaps.back() > 2400. * multiplier)
        metroTaps.clear();
    if (metroTaps.empty() || now - metroTaps.back() >= 150. * multiplier)
        metroTaps.push_back(now);
    if (metroTaps.size() == 8) {
        std::vector<double> gaps;
        for (size_t i = 1; i < metroTaps.size(); ++i)
            gaps.push_back(metroTaps[i] - metroTaps[i - 1]);
        std::sort(gaps.begin(), gaps.end());
        const auto mean = std::accumulate(gaps.begin() + 1, gaps.end() - 1, 0.) / (gaps.size() - 2);
        metroTapping = false;
        changeMetroTempo(std::round(2400000. / (mean * metroGrid.unit)) / 10.);
    }
    triggerAsyncUpdate();
}

void Studio::metronomePage(Rect workspace) {
    workspaceHeading = workspace.removeFromTop(s::workspaceHeader);
    text(cs::navMetro, workspaceHeading.reduced(s::inset, 0), s::workspaceTitle,
         s::chalk, false, true);
    inspectorBounds = workspace.removeFromRight(s::inspectorWidth);
    auto main = workspace.withSizeKeepingCentre(
        std::min(workspace.getWidth(), s::fieldWidth + s::transportSide),
        s::meterClock + 2 * s::gutter + s::workspaceRuler + 2 * s::button + s::gap + s::transport);
    metroBeats = main.removeFromTop(s::workspaceRuler);
    auto tempo = main.removeFromTop(s::meterClock + s::gutter);
    auto change = [&](const String& title, Rect bounds, int direction, Icon icon) {
        button(title, bounds.withSizeKeepingCentre(s::workspaceRuler, s::workspaceRuler),
               [this, direction] { changeMetroTempo(metroGrid.bpm + direction); },
               roundIdle, {}, icon);
    };
    change(cs::tempoLess, tempo.removeFromLeft(s::scaleWidth), -1, Icon::minus);
    change(cs::tempoMore, tempo.removeFromRight(s::scaleWidth), 1, Icon::plus);
    auto fields = main.removeFromTop(s::button);
    fader(cs::tempo, fields.withSizeKeepingCentre(s::fieldWidth, s::button), 30, 300,
          roundToInt(metroGrid.bpm), {}, [this](int v) { changeMetroTempo(v, false); });
    main.removeFromTop(s::gap);
    auto controls = main.removeFromTop(s::transport).withSizeKeepingCentre(
        2 * s::nameWidth + s::gap, s::transport);
    const bool audible = link.online && link.caps.outputChannels > 0;
    button(metronome ? cs::stopSong : cs::playSong, controls.removeFromLeft(s::nameWidth),
           [this] { toggleMetronome(); }, primary)->setEnabled(audible);
    controls.removeFromLeft(s::gap);
    button(metroTapping ? cs::tapCancel : cs::tap, controls, [this] {
        metroTapping = !metroTapping;
        metroTaps.clear();
        triggerAsyncUpdate();
    });
    auto caption = main.removeFromTop(s::button).withSizeKeepingCentre(s::fieldWidth, s::button);
    auto note = !link.online ? cs::noDeviceSong : !audible ? cs::noOutput
                 : metroTapping ? cs::tapHint + " " + String(metroTaps.size()) + " / 8" : String{};
    text(limit(skin.font(s::caption), note, caption.getWidth()), caption, s::caption,
         audible ? s::muted : s::warning);

    auto panel = inspectorBounds.reduced(s::inset);
    auto row = [&] {
        auto part = panel.removeFromTop(s::inspectorRow);
        panel.removeFromTop(s::inspectorGap);
        return part;
    };
    text(cs::meter.toUpperCase(), row(), s::section, s::navigationInk, false, true);
    const String current = String(metroGrid.beats) + "/" + String(metroGrid.unit);
    for (int i = 0; i < cs::meterNames.size();) {
        auto line = row();
        const int width = (line.getWidth() - 3 * s::grid) / 4;
        for (int column = 0; column < 4 && i < cs::meterNames.size(); ++column, ++i) {
            const auto name = cs::meterNames[i];
            button(name, line.removeFromLeft(width), [this, name] {
                const bool restart = bool(metronome);
                stopMetronome();
                metroGrid.beats = name.upToFirstOccurrenceOf("/", false, false).getIntValue();
                metroGrid.unit = name.fromFirstOccurrenceOf("/", false, false).getIntValue();
                if (restart)
                    toggleMetronome();
                else
                    triggerAsyncUpdate();
            }, name == current ? selected : plain);
            line.removeFromLeft(s::grid);
        }
    }
    combo(cs::clickMode, row(), {cs::songClick, cs::guideRhythm, cs::guideOff}, metroGuide, [this](int i) {
        const bool restart = bool(metronome);
        stopMetronome();
        metroGuide = i;
        if (restart)
            toggleMetronome();
        else
            triggerAsyncUpdate();
    });
    fader(cs::clickLevel, row(), 0, 100, settings.clickVolume, cs::percentUnit, [this](int value) {
        settings.clickVolume = value;
        if (metronome)
            metronome->configureClick(value, settings.clickSound);
        persist();
    });
    combo(cs::clickVoice, row(), cs::clickVoices, settings.clickSound, [this](int i) {
        settings.clickSound = i;
        if (metronome)
            metronome->configureClick(settings.clickVolume, i);
        persist();
    });
    button(cs::countIn, row(), [this] {
        metroCountIn = !metroCountIn;
        triggerAsyncUpdate();
    }, metroCountIn ? pillOn : pillOff)->setToggleState(metroCountIn, dontSendNotification);
    if (projectDir != File{} && !song.title.isEmpty())
        combo(cs::metroTarget, row(), {cs::chooseSong, cs::ideaClick}, metroForSong ? 0 : 1,
              [this](int i) { metroForSong = i == 0; triggerAsyncUpdate(); });
    if (metroForSong && !song.clips.empty() &&
        (song.grid.bpm != metroGrid.bpm || song.grid.beats != metroGrid.beats || song.grid.unit != metroGrid.unit))
        text(cs::tempoGridWarning, row(), s::caption, s::warning);
    button(metroForSong ? cs::applySongTempo : cs::applyIdeaTempo, row(), [this] {
        if (metroForSong) {
            const auto oldBeat = std::max<Frame>(0, song.grid.beatAt(view.cursor));
            const auto bar = oldBeat / song.grid.beats;
            const auto beatInBar = oldBeat % song.grid.beats;
            song.grid.bpm = metroGrid.bpm;
            song.grid.beats = metroGrid.beats;
            song.grid.unit = metroGrid.unit;
            song.projectOrder();
            view.cursor = song.grid.beat(bar * song.grid.beats +
                                        std::min<Frame>(song.grid.beats - 1, beatInBar));
            view.click = metroGuide != 2;
            if (view.click)
                view.rhythm = metroGuide == 1;
            view.countIn = metroCountIn;
            reveal(view.cursor);
            saveView();
            saveSong();
        } else {
            ideaTempo = metroGrid.bpm;
            ideaBeats = metroGrid.beats;
            ideaUnit = metroGrid.unit;
            ideaClick = metroGuide != 2;
            ideaRhythm = metroGuide == 1;
        }
        show(metroReturn);
    }, primary);
    button(cs::back, row(), [this] { show(metroReturn); });
}

void Studio::paintMetronome(Graphics& g) {
    const auto area = metroBeats;
    const int spacing = s::transport + s::grid;
    const int x = area.getCentreX() - metroGrid.beats * spacing / 2;
    const auto beat = metronome ? metroGrid.beatAt(metronome->position(metronome->cursor.load())) : -1;
    for (int i = 0; i < metroGrid.beats; ++i) {
        const int size = i == 0 ? s::transport : s::button;
        auto dot = Rect(x + i * spacing, area.getY(), spacing, area.getHeight())
                       .withSizeKeepingCentre(size, size).toFloat();
        g.setColour(i == 0 ? s::guitar : i == beat ? s::chalk : s::barGrid);
        if (i == beat)
            g.fillEllipse(dot);
        else
            g.drawEllipse(dot, s::stroke);
    }
    auto tempo = area.translated(0, area.getHeight()).withHeight(s::meterClock + s::gutter);
    g.setColour(s::chalk);
    g.setFont(skin.font(s::meterClock, true));
    g.drawText(String(metroGrid.bpm, metroGrid.bpm == std::round(metroGrid.bpm) ? 0 : 1)
                   .replaceCharacter('.', ','), tempo, Justification::centred);
}
} // namespace rocker
