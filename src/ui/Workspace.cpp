#include "Studio.h"
#include "Strings.h"

namespace rocker {
using namespace juce;
namespace s = style;

void Studio::navigation(Rect bounds) {
    navigationBounds = bounds;
    auto rail = bounds.reduced(s::grid, s::gap);
    text(cs::monogram, rail.removeFromTop(s::railLogoHeight).withTrimmedLeft(s::inset),
         s::workspaceTitle, s::chalk, false, true);
    auto item = [&](String title, String label, Icon icon, bool active, Handler action,
                    bool enabled = true, String key = {}, int height = s::railItemHeight) {
        auto spot = rail.removeFromTop(height);
        rail.removeFromTop(s::grid);
        auto* control = button(title, spot, std::move(action), active ? railCurrent : railIdle,
                               std::move(key), icon, std::move(label));
        control->setEnabled(enabled);
        if (active)
            control->setToggleState(true, dontSendNotification);
    };
    const bool workspace = page == Page::project;
    const bool capturing = (performance && performance->recording) || bool(ideaRecording);
    const bool canLeave = !capturing && !fileTask.valid() && !delivery.valid();
    item(cs::home, cs::navHome, Icon::home,
         page == Page::home, [this] {
             if (page >= Page::device && page <= Page::inputs) {
                 finish();
                 return;
             }
             if (page == Page::project)
                 finishProject();
             show(Page::home);
         }, canLeave);
    item(cs::ideas, cs::ideas, Icon::ideas, page == Page::ideas,
         [this] { show(Page::ideas); }, canLeave);
    auto label = rail.removeFromTop(s::caption + 2 * s::grid);
    text(cs::navSong.toUpperCase(), label.withTrimmedLeft(s::grid), s::railLabel, s::navigationInk,
         false, true);
    const bool hasSong = projectDir != File{} && !song.title.isEmpty();
    auto mode = [this, workspace, hasSong](int next) {
        if (!workspace) {
            if (!hasSong) {
                show(Page::live);
                return;
            }
            show(Page::project);
        }
        if (next == 3)
            mixMode(false);
        else
            editMode(next == 2);
    };
    item(cs::playMode, cs::livePage, Icon::playMode,
         (workspace && !view.editing && !view.mixing) || page == Page::live || page == Page::amp,
         [mode] { mode(1); }, workspace || canLeave, "1", s::railModeHeight);
    item(cs::editMode, cs::navEdit, Icon::editMode, workspace && view.editing,
         [mode] { mode(2); }, workspace || (hasSong && canLeave), "2", s::railModeHeight);
    item(cs::mixMode, cs::navMix, Icon::mixMode, workspace && view.mixing,
         [mode] { mode(3); }, workspace || (hasSong && canLeave), "3", s::railModeHeight);
    rail = rail.withTop(rail.getBottom() - 3 * (s::railItemHeight + s::grid));
    item(cs::navMetro, cs::navMetro, Icon::metronome, page == Page::metronome,
         [this] { if (page != Page::metronome) openMetronome(); }, canLeave);
    item(cs::tuner, cs::tuner, Icon::tuner, page == Page::tuner || page == Page::customTuning,
         [this] { toggleTuner(); }, canLeave, "T");
    item(cs::setup, cs::navSetup, Icon::device,
         (page >= Page::device && page <= Page::inputs) || page == Page::midi || page == Page::drums,
         [this] { show(Page::device); }, canLeave);
}

void Studio::deviceStatus(Rect bounds) {
    const auto endpoint = link.current.output.isEmpty() ? link.current.input : link.current.output;
    const auto label = link.online ? cs::drivers[(int)link.current.driver] + cs::divider + endpoint +
                                   cs::divider + cs::frequency(link.current.rate) + cs::divider +
                                   cs::bufferSize(link.current.buffer)
                                 : String(cs::disconnected);
    text(limit(skin.font(s::caption), label, bounds.getWidth()), bounds, s::caption,
         link.online ? s::muted : s::warning, false, false, true);
}

void Studio::paintWorkspace(Graphics& g) {
    if (!navigationBounds.isEmpty()) {
        g.setColour(s::panel);
        g.fillRect(navigationBounds);
        g.setColour(s::line);
        g.fillRect(navigationBounds.withLeft(navigationBounds.getRight() - s::hairline));
    }
    for (auto band : {workspaceHeading, transportBounds, inspectorBounds}) {
        if (band.isEmpty())
            continue;
        g.setColour(s::panel);
        g.fillRect(band);
        g.setColour(s::line);
        g.fillRect(band.withHeight(s::hairline));
    }
    if (inspectorBounds.getWidth() > 0) {
        g.setColour(s::line);
        g.fillRect(inspectorBounds.withWidth(s::hairline));
    }
}

void Studio::openTempoSettings() {
    if (performance)
        return;
    editingTempo = true;
    draftTitle = song.title;
    draftTempo = String(song.grid.bpm, 1);
    draftBeats = song.grid.beats;
    draftUnit = song.grid.unit;
    show(Page::newProject);
}

void Studio::projectTransport(Rect bounds) {
    transportBounds = bounds;
    auto row = bounds.reduced(s::inset, 0);
    auto left = row.removeFromLeft(s::transportSide);
    transportPosition = left.removeFromLeft(s::transportTimeWidth).reduced(0, s::gap);
    const String bpm = String(song.grid.bpm, 1).replaceCharacter('.', ',') + " BPM";
    auto* meterButton = button(cs::tempo, left.withSizeKeepingCentre(left.getWidth() - s::gap, s::button),
                               [this] { openMetronome(); }, plain, {}, Icon::none,
                               bpm + cs::divider + String(song.grid.beats) + "/" + String(song.grid.unit));
    meterButton->setEnabled(!performance);
    meterButton->getProperties().set("number", true);
    auto right = row.removeFromRight(s::transportSide);
    const int controlsWidth = s::transport + 2 * s::recordButton + 3 * s::gap;
    row = row.withLeft(row.getCentreX() - controlsWidth / 2);
    auto* start = button(cs::atStart, row.removeFromLeft(s::transport + s::gap)
                                         .withSizeKeepingCentre(s::transport, s::transport),
                         [this] {
                             view.cursor = 0;
                             saveView();
                             repaint();
                         }, roundIdle, {}, Icon::start);
    start->setEnabled(!performance);
    const bool recordingNow = performance && performance->recording;
    const bool playingNow = performance && !recordingNow;
    auto control = [&] {
        return row.removeFromLeft(s::recordButton + s::gap)
            .withSizeKeepingCentre(s::recordButton, s::recordButton);
    };
    auto* play = button(playingNow ? cs::stopSong : cs::playSong, control(), [this] {
        if (performance)
            stopFromTransport();
        else
            startProject(false);
    }, roundPlay, {}, playingNow ? Icon::stop : Icon::play);
    play->setEnabled(!recordingNow);
    const auto recordZone = row.withWidth(s::recordButton + s::gap);
    auto* record = button(recordingNow ? cs::stopSong : cs::recordSong, control(), [this] {
        if (performance)
            stopFromTransport();
        else
            startProject(true);
    }, recordingNow ? roundRecording : roundRecord, {}, recordingNow ? Icon::stop : Icon::none);
    record->setEnabled(!playingNow);
    text(recordingNow ? cs::stopSong : cs::recordShort, recordZone.withTrimmedRight(s::grid)
             .removeFromBottom(s::caption + s::grid).withTrimmedLeft(s::grid),
         s::railLabel, recordingNow ? s::armedInk : s::muted);
    transportInfo = row.reduced(s::grid, s::gap);

    auto pill = [&](String name, int width, bool on, Handler action) {
        auto bounds = right.removeFromRight(width + s::grid)
                          .withSizeKeepingCentre(width, s::transportPill);
        auto* b = button(name, bounds, std::move(action), on ? pillOn : pillOff);
        b->setEnabled(!performance);
        b->setToggleState(on, dontSendNotification);
        return b;
    };
    auto* loop = pill(cs::playLoop, s::levelWidth - s::gutter, view.playLoop && selection().valid(), [this] {
        view.playLoop = !view.playLoop;
        saveView();
        triggerAsyncUpdate();
    });
    loop->setEnabled(!performance && selection().valid());
    loop->setTooltip(cs::playLoopHelp);
    pill(cs::countIn, s::levelWidth - s::gutter, view.countIn, [this] {
        view.countIn = !view.countIn;
        saveView();
        triggerAsyncUpdate();
    });
    auto guide = !view.click ? cs::guideOff : view.rhythm ? cs::guideRhythm : cs::songClick;
    pill(guide, s::levelWidth - s::gutter, view.click, [this] {
        view.click = !view.click;
        saveView();
        triggerAsyncUpdate();
    });
}

void Studio::paintTransport(Graphics& g) {
    if (transportBounds.isEmpty())
        return;
    const auto pos = std::max<Frame>(0, playPosition());
    const auto beat = song.grid.beatAt(pos);
    const bool recordingNow = performance && performance->recording;
    const bool counting = recordingNow && playPosition() < performance->start;
    const auto state = counting ? cs::countIn : recordingNow ? cs::stateRecord
                                                     : performance ? cs::statePlay : cs::stateStop;
    const auto clock = counting ? String(std::max<Frame>(1, -song.grid.beatAt(playPosition() - performance->start)))
                                : String(beat / song.grid.beats + 1) + "." + String(beat % song.grid.beats + 1);
    auto position = transportPosition;
    auto time = position.removeFromTop(s::workspaceFooter / 2);
    auto bars = time.removeFromLeft(time.getWidth() * 2 / 3);
    g.setColour(s::chalk);
    g.setFont(skin.font(s::counter, true));
    g.drawText(limit(skin.font(s::counter, true), clock, bars.getWidth()), bars,
               Justification::centredLeft);
    const auto seconds = pos / song.grid.rate;
    g.setFont(skin.font(s::section, true));
    g.drawText(String(seconds / 60) + ":" + String(seconds % 60).paddedLeft('0', 2), time,
               Justification::centredRight);
    g.setColour(counting ? s::warning : recordingNow ? s::armedInk : s::muted);
    g.setFont(skin.font(s::caption, false, true));
    g.drawText(state, position, Justification::centredLeft);
    auto info = transportInfo;
    StringArray destinations;
    for (const auto& track : song.tracks)
        if (recordingNow ? performance->captures(track.id) : track.armed)
            destinations.add(track.name);
    if (!destinations.isEmpty()) {
        auto action = info.removeFromTop(s::caption + s::grid);
        g.drawText(recordingNow ? cs::recordingInto : cs::readyToRecord, action,
                   Justification::centredLeft);
    }
    auto value = destinations.isEmpty() ? cs::noneArmed : destinations.joinIntoString(", ");
    auto names = info.removeFromTop(s::workspaceTitle);
    g.setColour(s::chalk);
    g.drawText(limit(skin.font(s::caption, false, true), value, names.getWidth()), names,
               Justification::centredLeft);
    if (recordingNow)
        if (auto loop = performance->loopSpan()) {
            const auto pass = 1 + std::max<Frame>(0, performance->cursor.load() - loop->from) / loop->length();
            g.setColour(s::armedInk);
            g.drawText(performance->loopStopping() ? cs::postRoll
                                                   : cs::attemptLabel + " " + String(pass),
                       info, Justification::centredLeft);
        }
}

void Studio::selectProjectTrack(int i) {
    if (performance && performance->recording)
        return;
    view.selected = i;
    monitorHelpOpen = false;
    view.selectedClip.clear();
    defaultMonitor();
    configureSongMonitor();
    saveView();
    triggerAsyncUpdate();
}

void Studio::projectTrack(int i, Rect bounds) {
    const auto& t = song.tracks[i];
    auto head = bounds.reduced(s::gap, s::grid);
    auto nameArea = head.removeFromTop(s::inspectorRow);
    const auto caption = t.kind == "midi" ? cs::midiModule : t.inputs.joinIntoString(" + ");
    auto* track = button(limit(skin.font(s::body, false, true), t.name,
                               nameArea.getWidth() - 2 * s::inset), nameArea,
                         [this, i] { selectProjectTrack(i); }, i == view.selected ? 1 : 0);
    track->setTooltip(t.name);
    track->setEnabled(!performance || !performance->recording);
    auto toggles = head.removeFromTop(s::inspectorRow).reduced(0, s::stroke);
    auto qualified = [&](const String& title) { return i == view.selected ? title : title + cs::divider + t.name; };
    const auto lost = missingPorts(t);
    const auto armLabel = t.armed ? cs::disarm : cs::arm;
    auto armArea = toggles.removeFromLeft(s::armWidth);
    if (lost.isNotEmpty()) {
        auto* route = button(qualified(cs::chooseInput), armArea, [this, i] {
            beginRemap(i);
        }, plain, {}, Icon::none, cs::chooseInput);
        route->setEnabled(!performance);
        route->setTooltip(lost);
    } else {
    auto* arm = button(qualified(armLabel), armArea, [this, i] {
        auto& t = song.tracks[i];
        const bool enable = !t.armed;
        for (auto& member : song.tracks)
            if (song.linkedTrack(member.id, t.id))
                member.armed = enable;
        saveSong();
        triggerAsyncUpdate();
    }, t.armed ? armActive : plain, {}, Icon::none, armLabel);
    arm->setToggleState(t.armed, dontSendNotification);
    arm->setEnabled(!performance);
    }
    auto toggle = [&](const String& description, const String& label, bool solo, int style) {
        toggles.removeFromLeft(s::grid);
        const bool enabled = solo ? t.solo : t.mute;
        auto* control = button(qualified(description), toggles.removeFromLeft(s::trackToggle),
                               [this, i, solo] {
                                   auto& track = song.tracks[i];
                                   auto& flag = solo ? track.solo : track.mute;
                                   flag = !flag;
                                   if (performance)
                                       performance->playback.updateMix(song);
                                   saveSong();
                                   triggerAsyncUpdate();
                               }, enabled ? style : plain, {}, Icon::none, label);
        control->setTooltip(description + cs::divider + t.name);
        control->setToggleState(enabled, dontSendNotification);
    };
    toggle(cs::mute, cs::muteShort, false, muteActive);
    toggle(cs::solo, cs::soloShort, true, soloActive);
    head.removeFromBottom(s::traceHeight);
    if (lost.isNotEmpty())
        text(limit(skin.font(s::caption), lost, head.getWidth()), head, s::caption, s::warning);
    else if (t.kind == "kytara") {
        auto name = t.amp.enabled ? t.amp.name + (t.amp.modified ? "*" : "") : cs::bypass;
        auto* badge = button(limit(skin.font(s::body, false, true), name, head.getWidth() - 2 * s::inset),
                             head, [this, i] { openTrackSound(i); });
        badge->setEnabled(!performance || !performance->recording || i == view.selected);
        if (i == view.selected)
            soundBadge = badge;
    } else if (head.getHeight() >= skin.font(s::caption).getHeight())
        text(limit(skin.font(s::caption, true), caption, head.getWidth()), head, s::caption,
             s::muted, true);
}

void Studio::playInspector(Rect area) {
    auto row = [&](int height = s::inspectorRow) {
        auto part = area.removeFromTop(height);
        area.removeFromTop(s::inspectorGap);
        return part;
    };
    if (!isPositiveAndBelow(view.selected, (int)song.tracks.size())) {
        text(cs::audioTracks, row(), s::workspaceTitle, s::chalk, false, true);
        return;
    }
    auto& t = song.tracks[view.selected];
    const auto lost = missingPorts(t);
    const auto input = lost.isNotEmpty() ? lost : t.kind == "midi" ? cs::midiModule : t.inputs.joinIntoString(" + ");
    auto title = row();
    text(limit(skin.font(s::workspaceTitle, false, true), t.name, title.getWidth()), title,
         s::workspaceTitle, s::chalk, false, true);
    auto connection = row(t.kind == "kytara" ? s::inspectorRow : s::caption + s::grid);
    if (t.kind == "kytara")
        button(cs::monitorHelp, connection.removeFromRight(s::nameWidth), [this] {
            monitorHelpOpen = !monitorHelpOpen;
            triggerAsyncUpdate();
        });
    text(limit(skin.font(s::caption, true), input, connection.getWidth()), connection,
         s::caption, s::muted, true);
    if (!monitorHelpOpen && (t.kind == "kytara" || t.kind == "jine")) {
        text(cs::voicePanel.toUpperCase(), row(s::caption + s::grid), s::section, s::navigationInk,
             false, true);
        if (t.kind == "kytara") {
            for (int r = 0; r < 2; ++r) {
                auto presetsRow = row(s::button);
                const int width = (presetsRow.getWidth() - s::grid) / 2;
                for (int i = r * 2; i < r * 2 + 2; ++i) {
                    button(cs::preset + cs::divider + cs::presetNames[i], presetsRow.removeFromLeft(width), [this, i] {
                        song.tracks[view.selected].amp = {cs::presetNames[i], presets[i], true, false};
                        applyTrackSound(true);
                        triggerAsyncUpdate();
                    }, t.amp.enabled && t.amp.name == cs::presetNames[i] ? 1 : 0, {}, Icon::none,
                       cs::presetNames[i]);
                    presetsRow.removeFromLeft(s::grid);
                }
            }
            auto sound = [this, &t, &row](const String& name, std::uint8_t AmpParameters::* member) {
                fader(name, row(), 0, 100, t.amp.values.*member, cs::percentUnit, [this, member](int v) {
                    auto& amp = song.tracks[view.selected].amp;
                    amp.values.*member = static_cast<std::uint8_t>(v);
                    amp.modified = true;
                    applyTrackSound(false);
                });
            };
            sound(cs::drive, &AmpParameters::drive);
            sound(cs::bass, &AmpParameters::bass);
            sound(cs::middle, &AmpParameters::mid);
            sound(cs::treble, &AmpParameters::treble);
        }
        button(t.kind == "kytara" ? cs::ampPage : cs::voicePanel, row(), [this] {
            openTrackSound(view.selected);
        }, plain, {}, Icon::none, cs::advancedSound);
    }
    if (!performance) {
        StringArray names;
        for (const auto& track : song.tracks)
            names.add(track.name);
        combo(cs::audioTracks, row(), names, view.selected, [this](int i) { selectProjectTrack(i); });
        button(cs::trackEdit, row(), [this] { editTrack(false); });
        auto monitors = cs::monitors;
        if (laptop || t.kind != "kytara")
            monitors.remove(2);
        combo(cs::listen, row(), monitors, std::min(projectMonitor, monitors.size() - 1), [this](int i) {
            projectMonitor = i;
            configureSongMonitor();
            triggerAsyncUpdate();
        });
        auto delay = row(s::caption + s::grid);
        text(limit(skin.font(s::caption), latencyLabel(), delay.getWidth()), delay, s::caption, s::muted);
        if (t.group.isNotEmpty())
            for (const auto& member : song.tracks) {
                if (member.group != t.group || member.kind != "midi")
                    continue;
                const bool toMidi = member.mute;
                button(toMidi ? cs::midiListen : cs::moduleListen, row(s::button),
                       [this, id = t.group, toMidi] {
                           for (int n = 0; n < (int)song.tracks.size(); ++n) {
                               auto& track = song.tracks[n];
                               if (track.group == id) {
                                   track.mute = (track.kind == "midi") != toMidi;
                                   if (!track.mute)
                                       view.selected = n;
                               }
                           }
                           defaultMonitor();
                           configureSongMonitor();
                           saveSong();
                           saveView();
                           triggerAsyncUpdate();
                       });
                break;
            }
        button(cs::addBacking, row(), [this] { chooseImport(); })->setEnabled(!importing.valid());
        button(importAtCursor ? cs::atCursor : cs::atBeginning, row(), [this] {
            importAtCursor = !importAtCursor;
            triggerAsyncUpdate();
        });
    }
}
} // namespace rocker
