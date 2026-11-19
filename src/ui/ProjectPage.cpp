#include "Studio.h"
#include "engine/LoadTrial.h"
#include "Strings.h"
#include <numeric>
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif JUCE_MAC
#include <IOKit/pwr_mgt/IOPMLib.h>
#endif
namespace rocker {
using namespace juce;
namespace s = style;
namespace {
using Rect = juce::Rectangle<int>;
Rect row(Rect& r, int h = s::button) {
    auto line = r.removeFromTop(h);
    r.removeFromTop(s::gap);
    return line;
}
Rect cell(Rect& r, int width) {
    auto part = r.removeFromLeft(width);
    r.removeFromLeft(s::gap);
    return part;
}
const std::array<String, 3> kinds{"bici", "kytara", "jine"};
void preventSleep(bool active) {
#if JUCE_WINDOWS
    SetThreadExecutionState(active ? ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED
                                   : ES_CONTINUOUS);
#elif JUCE_MAC
    static IOPMAssertionID assertion = kIOPMNullAssertionID;
    if (active && assertion == kIOPMNullAssertionID)
        IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleSystemSleep, kIOPMAssertionLevelOn,
                                    CFSTR("Rocker 2"), &assertion);
    if (!active && assertion != kIOPMNullAssertionID) {
        IOPMAssertionRelease(assertion);
        assertion = kIOPMNullAssertionID;
    }
#endif
#if JUCE_WINDOWS || JUCE_MAC
    Desktop::setScreenSaverEnabled(!active);
#endif
}
String limit(const Font& f, String value, int width) {
    if (GlyphArrangement::getStringWidth(f, value) <= width)
        return value;
    while (value.isNotEmpty() && GlyphArrangement::getStringWidth(f, value + String(u8"…")) > width)
        value = value.dropLastCharacters(1);
    return value + String(u8"…");
}
} // namespace
void Studio::newProject() {
    editingTempo = false;
    draftTitle = cs::untitled;
    draftTempo = "112";
    draftBeats = 4;
    draftUnit = 4;
    lastTap = 0;
    show(Page::newProject);
}
void Studio::saveSong(String action) {
    if (projectDir != File{} && !song.title.isEmpty()) history.remember(song,action.isEmpty() ? cs::edited : action);
    pendingSoundSave = 0;
    if (projectDir != File{} && !song.save(projectDir))
        status(cs::writeError);
}
void Studio::finishProject() {
    if (pendingSoundSave)
        saveSong();
    soundDeckOpen = namingPreset = false;
    if (performance)
        stopProject();
    if (projectDir != File{} && song.title.isNotEmpty()) {
        if (!view.save(projectDir) || !song.archive(projectDir))
            status(cs::writeError);
    }
}
void Studio::openProject(File dir) {
    if (dir == File{} || !dir.isDirectory()) {
        status(cs::noSong);
        return;
    }
    finishProject();
    const bool sameProject = projectDir == dir;
    projectDir = dir;
    browsingVersions = false;
    drag = Drag::none;
    if (!sameProject) clipboard = {};

    library.data.clear();
    bool damaged = false;
    auto loaded = Song::load(dir, damaged);
    if (!loaded) {
        song = {};
        show(Page::rescue);
        return;
    }
    song = *loaded;
    if (!sameProject || !history.matches(song)) history.reset(song);
    for (int i = 0; i < (int)tunings.size(); ++i)
        if (tunings[i].name == song.tuning) {
            tuningIndex = i;
            break;
        }
    const bool complete = library.load(song, dir);
    view = ViewState::load(dir);
    view.selected = jlimit(0, std::max(0, (int)song.tracks.size() - 1), view.selected);
    view.cursor = std::min(view.cursor, song.end());
    normalizeSelection();
    projectMonitor = 0;
    settings.lastSong = dir.getFullPathName();
    persist();
    lastArchive = Time::getMillisecondCounterHiRes();
    if (!dir.getChildFile("Zachrana").findChildFiles(File::findFiles, false, "*.json").isEmpty()) {
        show(Page::rescue);
        return;
    }
    show(Page::project);
    if (damaged || !complete)
        status(cs::partialProject);
}
void Studio::field(const String& label, Rect r, String& value) {
    text(label, r.removeFromLeft(s::labelWidth), s::body, s::muted);
    auto* edit = new TextEditor;
    controls.add(edit);
    addAndMakeVisible(edit);
    edit->setName(label);
    edit->setBounds(r.withWidth(s::fieldWidth));
    edit->setFont(skin.font(s::body));
    edit->setInputRestrictions(64);
    edit->setText(value, false);
    edit->onTextChange = [edit, &value] { value = edit->getText(); };
    edit->onReturnKey = [this] {
        if (page == Page::newProject || page == Page::trackSettings || namingPreset)
            for (auto* c : controls)
                if (auto* b = dynamic_cast<Button*>(c);
                    b && (b->getName() == cs::apply || b->getName() == cs::createSong ||
                          b->getName() == cs::storePreset)) {
                    b->onClick();
                    break;
                }
    };
}
void Studio::configureSongMonitor() {
    auto& live = link.audio.live;
    live.monitor = Monitor::off;
    if (view.selected < 0 || view.selected >= (int)song.tracks.size())
        return;
    auto& t = song.tracks[view.selected];
    if (t.kind != "kytara")
        soundDeckOpen = namingPreset = false;
    int port = link.caps.inputs.indexOf(t.inputs[0]);
    if (port < 0)
        return;
    live.selectInput(port);
    live.setAmp(t.amp.values);
    if (projectMonitor == 2 && (laptop || t.kind != "kytara"))
        projectMonitor = 0;
    live.monitor =
        projectMonitor == 2 && !t.amp.enabled ? Monitor::clean : static_cast<Monitor>(projectMonitor);
}
Frame Studio::playPosition() const { return performance ? performance->cursor.load() : view.cursor; }
void Studio::toggleDistant() {
    view.distant = !view.distant;
    view.save(projectDir);
    triggerAsyncUpdate();
}
void Studio::editTrack(bool fresh) {
    if (performance)
        return;
    if (fresh) {
        draftTrack = cs::trackKinds[2];
        draftKind = 2;
        draftPort = 0;
        draftChannels = 1;
        view.selected = (int)song.tracks.size();
    } else {
        auto& t = song.tracks.at(view.selected);
        draftTrack = t.name;
        draftKind = (int)(std::find(kinds.begin(), kinds.end(), t.kind) - kinds.begin());
        draftKind = std::min(2, draftKind);
        draftPort = std::max(0, link.caps.inputs.indexOf(t.inputs[0]));
        draftChannels = std::max(1, t.inputs.size());
    }
    show(Page::trackSettings);
}
void Studio::startProject(bool record) {
    if (performance)
        return;
    if (!link.online) {
        status(cs::noDeviceSong);
        return;
    }
    if (link.current.rate != song.grid.rate) {
        status(cs::rateMismatch);
        return;
    }
    if (!library.load(song, projectDir)) {
        status(cs::partialProject);
        return;
    }
    if (!record && view.cursor >= song.end())
        view.cursor = 0;
    auto p = std::make_unique<Performance>(song, library, view.cursor, record, view.click,
                                           record && view.countIn ? song.grid.beats : 0);
    if (record) {
        int latency = link.caps.inputLatency + link.caps.outputLatency;
        auto saved = settings.latency.find(Settings::calibrationKey(link.current));
        if (saved != settings.latency.end())
            latency = saved->second.samples;
        if (std::none_of(song.tracks.begin(), song.tracks.end(), [](auto& t) { return t.armed; })) {
            status(cs::noneArmed);
            return;
        }
        for (auto& t : song.tracks)
            if (t.armed && (t.inputs.isEmpty() || !link.caps.inputs.contains(t.inputs[0]) ||
                            (t.inputs.size() > 1 && !link.caps.inputs.contains(t.inputs[1])))) {
                status(cs::missingInputs);
                return;
            }
        if (!p->arm(song, projectDir, link.caps.inputs, latency)) {
            status(cs::writeError);
            return;
        }
        preventSleep(true);
    }
    liveWaves.clear();
    performance = std::move(p);
    link.audio.timeline.attach(*performance);
    status({});
}
void Studio::stopProject() {
    if (!performance)
        return;
    link.audio.timeline.detach();
    auto p = std::move(performance);
    p->finishDisk();
    if (p->recording)
        preventSleep(false);
    if (testingLoad) {
        finishLoadTrial(*p);
        p.reset();
        diagnosticDir.deleteRecursively();
        return;
    }
    if (diagnosing) {
        finishSync(*p);
        return;
    }
    view.cursor = std::max(p->start, p->cursor.load());
    if (p->drop >= 0)
        song.dropouts.push_back(p->drop.load());
    if (p->recording) {
        if (!recoverTake(song, projectDir, p->journal) || !library.load(song, projectDir) ||
            p->failed.load())
            status(cs::writeError);
        else
            status(p->drop >= 0 ? cs::dropout : cs::savedSong);
    }
    if (p->recording) history.remember(song,cs::newTake);
    if (!view.save(projectDir))
        status(cs::writeError);
    triggerAsyncUpdate();
}
void Studio::projectTick() {
    if (performance && performance->finished.load()) {
        stopProject();
        return;
    }
    if (performance && performance->recording && !diagnosing && !view.distant) {
        auto frame = playPosition();
        if (frame >= performance->start)
            for (const auto& t : song.tracks)
                if (t.armed && !t.inputs.isEmpty()) {
                    auto& peaks = liveWaves[t.id];
                    if (peaks.empty() || peaks.back().first != frame) {
                        peaks.emplace_back(frame,
                                           link.audio.read(link.caps.inputs.indexOf(t.inputs[0])).peak);
                        while (!peaks.empty() &&
                               peaks.front().first < frame - song.grid.beat(view.zoom * song.grid.beats))
                            peaks.pop_front();
                    }
                }
    }
    if (page == Page::project && !performance)
        repaint(inputMeter);
    if (performance && (page == Page::project || diagnosing))
        repaint();
    auto now = Time::getMillisecondCounterHiRes();
    if (pendingSoundSave && now >= pendingSoundSave)
        saveSong();
    if (page == Page::project && now - lastArchive > 300000) {
        if (!song.archive(projectDir))
            status(cs::writeError);
        lastArchive = now;
    }
}
void Studio::projectPage(Rect area) {
    area.reduce(0, s::inset);
    if (page == Page::rescue) {
        text(cs::rescueTitle, row(area, s::header), s::title);
        bool version = song.title.isEmpty();
        text(version ? cs::rescueVersion : cs::rescueTake, row(area));
        button(
            cs::restore, row(area).withWidth(s::nameWidth),
            [this, version] {
                if (version) {
                    auto file = Song::latestVersion(projectDir);
                    bool damaged = false;
                    auto previous = Song::parse(JSON::parse(file), damaged);
                    if (!previous || !previous->save(projectDir)) {
                        status(cs::badProject);
                        return;
                    }
                } else
                    for (auto& f : projectDir.getChildFile("Zachrana")
                                       .findChildFiles(File::findFiles, false, "*.json"))
                        if (!recoverTake(song, projectDir, f)) {
                            status(cs::writeError);
                            return;
                        }
                openProject(projectDir);
            },
            1);
        button(cs::home, row(area).withWidth(s::nameWidth), [this] { show(Page::home); });
        text(message, row(area), s::body, s::warning);
        return;
    }
    if (page == Page::newProject || page == Page::trackSettings) {
        const bool project = page == Page::newProject;
        text(project ? (editingTempo ? cs::projectSetup : cs::song) : cs::trackEdit,
             row(area, s::header), s::title);
        field(project ? cs::titleSong : cs::trackName, row(area), project ? draftTitle : draftTrack);
        if (project) {
            field(cs::tempo, row(area), draftTempo);
            button(cs::tap, row(area).withWidth(s::nameWidth), [this] {
                double now = Time::getMillisecondCounterHiRes();
                double gap = now - lastTap;
                lastTap = now;
                if (gap >= 200 && gap <= 2000) {
                    draftTempo = String(60000 / gap, 1);
                    triggerAsyncUpdate();
                }
            });
            StringArray meters{"2/4", "3/4", "4/4", "6/8", "9/8", "12/8"};
            combo(cs::meter, row(area), meters,
                  meters.indexOf(String(draftBeats) + "/" + String(draftUnit)), [this, meters](int i) {
                      draftBeats = meters[i].upToFirstOccurrenceOf("/", false, false).getIntValue();
                      draftUnit = meters[i].fromFirstOccurrenceOf("/", false, false).getIntValue();
                  });
            if (editingTempo && !song.clips.empty())
                text(cs::editTempoWarning, row(area), s::body, s::warning);
        } else {
            combo(cs::trackKind, row(area), cs::trackKinds, draftKind, [this](int i) { draftKind = i; });
            if (!link.caps.inputs.isEmpty()) {
                combo(cs::input, row(area), inputNames(), draftPort, [this](int i) {
                    draftPort = i;
                    draftChannels = 1;
                    triggerAsyncUpdate();
                });
                StringArray modes{cs::inputMono};
                if (draftPort + 1 < link.caps.inputs.size())
                    modes.add(cs::inputPair);
                combo(cs::channels, row(area), modes, std::min(draftChannels, modes.size()) - 1,
                      [this](int i) { draftChannels = i + 1; });
            }
        }
        auto actions = row(area);
        button(
            project && !editingTempo ? cs::createSong : cs::apply, cell(actions, s::nameWidth),
            [this, project] {
                if (project) {
                    auto bpm = draftTempo.replaceCharacter(',', '.').getDoubleValue();
                    if (!std::isfinite(bpm) || bpm < 30 || bpm > 300 || draftTitle.trim().isEmpty()) {
                        status(cs::badTempo);
                        return;
                    }
                    if (!editingTempo) {
                        auto name =
                            File::createLegalFileName(draftTitle.trim()).removeCharacters(".").trim();
                        if (name.isEmpty()) {
                            status(cs::badTempo);
                            return;
                        }
                        auto folder = libraryRoot.getNonexistentChildFile(name, "", false);
                        Song fresh;
                        fresh.title = draftTitle.trim();
                        fresh.grid = {link.online ? link.current.rate : 48000, draftBeats, draftUnit,
                                      bpm};
                        fresh.tuning = tunings[tuningIndex].name;
                        for (int i = 0; i < 2; ++i) {
                            Track t{Uuid().toString(), cs::trackKinds[i], kinds[i]};
                            t.amp.enabled =
                                t.kind == "kytara" && (guitarSource == 0 || guitarSource == 3);
                            for (int c = 0; c < link.caps.inputs.size(); ++c) {
                                auto alias = settings.aliases.find(Settings::inputKey(link.current, c));
                                if (alias != settings.aliases.end() &&
                                    alias->second.equalsIgnoreCase(t.name)) {
                                    t.inputs.add(link.caps.inputs[c]);
                                    break;
                                }
                            }
                            if (t.inputs.isEmpty() && link.caps.inputs.size() == 1)
                                t.inputs.add(link.caps.inputs[0]);
                            fresh.tracks.push_back(t);
                        }
                        if (!fresh.save(folder) || !fresh.archive(folder)) {
                            status(cs::writeError);
                            return;
                        }
                        openProject(folder);
                        return;
                    }
                    song.title = draftTitle.trim();
                    song.grid.bpm = bpm;
                    song.grid.beats = draftBeats;
                    song.grid.unit = draftUnit;
                } else {
                    if (draftTrack.trim().isEmpty())
                        return;
                    if (view.selected == (int)song.tracks.size())
                        song.tracks.push_back({Uuid().toString()});
                    auto& t = song.tracks[view.selected];
                    t.name = draftTrack.trim();
                    if (t.kind != kinds[draftKind])
                        t.amp.enabled =
                            kinds[draftKind] == "kytara" && (guitarSource == 0 || guitarSource == 3);
                    t.kind = kinds[draftKind];
                    t.inputs.clear();
                    for (int i = 0; i < draftChannels && draftPort + i < link.caps.inputs.size(); ++i)
                        t.inputs.add(link.caps.inputs[draftPort + i]);
                }
                show(Page::project);
                saveSong();
                view.save(projectDir);
            },
            1);
        button(cs::back, cell(actions, s::nameWidth), [this, project] {
            view.selected = jlimit(0, std::max(0, (int)song.tracks.size() - 1), view.selected);
            show(project && !editingTempo ? Page::home : Page::project);
        });
        text(message, row(area), s::body, s::warning);
        return;
    }
    if (view.distant) {
        auto footer = area.removeFromBottom(s::footer);
        button(cs::normal, cell(footer, s::nameWidth), [this] { toggleDistant(); });
        button(performance ? cs::stopSong : cs::recordSong, cell(footer, s::nameWidth), [this] {
            if (performance)
                stopProject();
            else
                startProject(true);
        });
        text(message, footer, s::body, s::warning);
        return;
    }
    auto top = row(area, s::statusStrip);
    text(limit(skin.font(s::heading), song.title, s::nameWidth), cell(top, s::nameWidth), s::heading);
    button(
        performance ? cs::stopSong : cs::recordSong, cell(top, s::clipWidth),
        [this] {
            if (performance)
                stopProject();
            else
                startProject(true);
        },
        performance ? 1 : 2);
    if (!performance) {
        button(cs::playSong, cell(top, s::levelWidth), [this] { startProject(false); });
        button(cs::atStart, cell(top, s::levelWidth), [this] {
            view.cursor = 0;
            view.save(projectDir);
            repaint();
        });
    }
    auto modes = row(area);
    button(cs::playMode, cell(modes,s::compactButton),[this]{ editMode(false); },view.editing ? 0 : 1);
    button(cs::editMode, cell(modes,s::compactButton),[this]{ editMode(true); },view.editing ? 1 : 0);
    if (!performance) button(cs::fileMenu,cell(modes,s::compactButton),[this]{showVersions();});
    if (view.editing) {
        button(cs::beforeBars,cell(modes,s::clipWidth),[this]{view.firstBar=std::max<Frame>(0,view.firstBar-visibleBars());view.save(projectDir);repaint();});
        button(cs::afterBars,cell(modes,s::clipWidth),[this]{view.firstBar=std::min<Frame>(999999,view.firstBar+visibleBars());view.save(projectDir);repaint();});
        button(cs::zoomIn,cell(modes,s::compactButton),[this]{editKey(KeyPress('+'));});
        button(cs::zoomOut,cell(modes,s::compactButton),[this]{editKey(KeyPress('-'));});
    }
    auto tools = row(area);
    button(cs::distantView, cell(tools, s::compactButton), [this] { toggleDistant(); });
    if (!performance) {
        button(cs::newTrack, cell(tools, s::levelWidth), [this] { editTrack(true); });
        if (!song.tracks.empty())
            button(cs::trackEdit, cell(tools, s::clipWidth), [this] { editTrack(false); });
        button(cs::projectSetup, cell(tools, s::clipWidth), [this] {
            editingTempo = true;
            draftTitle = song.title;
            draftTempo = String(song.grid.bpm, 1);
            draftBeats = song.grid.beats;
            draftUnit = song.grid.unit;
            show(Page::newProject);
        });
        button(
            cs::songClick, cell(tools, s::compactButton),
            [this] {
                view.click = !view.click;
                view.save(projectDir);
                triggerAsyncUpdate();
            },
            view.click ? 1 : 0);
        button(
            cs::countIn, cell(tools, s::compactButton),
            [this] {
                view.countIn = !view.countIn;
                view.save(projectDir);
                triggerAsyncUpdate();
            },
            view.countIn ? 1 : 0);
        button(cs::home, cell(tools, s::nameWidth), [this] {
            finishProject();
            show(Page::home);
        });
    }
    auto foot = area.removeFromBottom(soundDeckOpen ? s::soundDeck : s::songDeck);
    panels.push_back(foot);
    foot.reduce(s::inset, s::gap);
    auto notice = area.removeFromBottom(s::button);
    if (browsingVersions) {
        inputMeter = {};
        versionsDeck(foot);
    } else if (view.editing && !soundDeckOpen) {
        inputMeter = {};
        editDeck(foot);
    } else if (soundDeckOpen) {
        inputMeter = {};
        trackSoundDeck(foot);
    } else {
        auto upper = row(foot);
        text(cs::hear.toUpperCase(), cell(upper, s::nameWidth), s::section, s::muted);
        text(cs::toRecord.toUpperCase(), cell(upper, s::labelWidth), s::section, s::muted);
        text(cs::delay, cell(upper, s::fieldWidth), s::caption, s::muted);
        text(cs::input, upper, s::caption, s::muted);
        auto info = row(foot);
        StringArray heard;
        if (performance) {
            const bool solo =
                std::any_of(song.tracks.begin(), song.tracks.end(), [](auto& t) { return t.solo; });
            for (auto& t : song.tracks)
                if (!t.mute && (!solo || t.solo) &&
                    std::any_of(song.clips.begin(), song.clips.end(),
                                [&](auto& c) { return c.track == t.id; }))
                    heard.add(t.name);
            if (view.click || playPosition() < performance->start)
                heard.add(cs::songClick);
        }
        if (projectMonitor > 0 && view.selected < (int)song.tracks.size())
            heard.add(song.tracks[view.selected].name + " / " +
                      (projectMonitor == 2 && song.tracks[view.selected].amp.enabled
                           ? song.tracks[view.selected].amp.name
                           : cs::monitors[1]));
        text(limit(skin.font(s::body), heard.isEmpty() ? cs::monitors[0] : heard.joinIntoString(" + "),
                   s::nameWidth),
             cell(info, s::nameWidth));
        StringArray armed;
        for (auto& t : song.tracks)
            if (t.armed)
                armed.add(t.inputs.joinIntoString(" + ") + String(u8" → ") + t.name);
        text(limit(skin.font(s::body), armed.isEmpty() ? cs::noneArmed : armed.joinIntoString(", "),
                   s::labelWidth),
             cell(info, s::labelWidth));
        text(latencyLabel(), cell(info, s::fieldWidth));
        inputMeter = info;
        if (!performance && view.selected < (int)song.tracks.size()) {
            auto controlsRow = row(foot);
            auto modes = cs::monitors;
            if (laptop || song.tracks[view.selected].kind != "kytara")
                modes.remove(2);
            combo(cs::listen, cell(controlsRow, s::fieldWidth), modes, projectMonitor, [this](int i) {
                projectMonitor = i;
                configureSongMonitor();
                triggerAsyncUpdate();
            });
            if (song.tracks[view.selected].kind == "kytara")
                button(cs::ampPage, cell(controlsRow, s::clipWidth),
                       [this] { openTrackSound(view.selected); });
            button(cs::tuner, cell(controlsRow, s::levelWidth), [this] { toggleTuner(); });
        }
    }
    auto disk = notice.removeFromRight(s::labelWidth);
    const int channels = std::max(
        1, std::accumulate(song.tracks.begin(), song.tracks.end(), 0, [](int n, const Track& t) {
            return n + (t.armed ? std::max(1, t.inputs.size()) : 0);
        }));
    text(cs::availableSpace + "  " +
             String(projectDir.getBytesFreeOnVolume() / (3600. * 4 * channels * song.grid.rate), 1) +
             " h",
         disk, s::caption, s::muted, true, false, true);
    text(message, notice, s::caption, s::warning);
    ruler = row(area);
    text(cs::verse + "   /   " + cs::bars, ruler.withWidth(s::trackHead), s::body, s::guitar);
    auto trackCommands = area.removeFromBottom(s::button);
    timelineArea = area;
    lanes.clear();
    int count = std::max(1, area.getHeight() / s::songLane);
    firstTrack = std::max(0, view.selected - count + 1);
    for (int i = firstTrack; i < (int)song.tracks.size() && (int)lanes.size() < count; ++i) {
        auto lane = area.removeFromTop(s::songLane);
        lanes.push_back(lane);
        auto head = lane.removeFromLeft(s::trackHead).reduced(s::grid);
        auto& t = song.tracks[i];
        auto nameArea = head.removeFromTop(s::button - s::grid);
        button(
            limit(skin.font(s::body, false, true), t.name, nameArea.getWidth() - 2 * s::inset), nameArea,
            [this, i] {
                if (!performance) {
                    view.selected = i;
                    view.selectedClip.clear();
                    projectMonitor = 0;
                    configureSongMonitor();
                    view.save(projectDir);
                    triggerAsyncUpdate();
                }
            },
            i == view.selected ? 1 : 0);
        if (t.kind == "kytara") {
            const auto label = t.amp.enabled ? t.amp.name + (t.amp.modified ? "*" : "") : cs::bypass;
            auto* badge =
                button(limit(skin.font(s::body, false, true), label, head.getWidth() - 2 * s::inset),
                       head, [this, i] { openTrackSound(i); });
            if (i == view.selected)
                soundBadge = badge;
        } else
            text(limit(skin.font(s::caption), t.inputs.joinIntoString(" + "), head.getWidth()), head,
                 s::caption, s::muted);
    }
    if (!song.tracks.empty()) {
        auto r = trackCommands;
        if (!performance)
            button(
                song.tracks[view.selected].armed ? cs::disarm : cs::arm, cell(r, s::levelWidth),
                [this] {
                    auto& t = song.tracks[view.selected];
                    t.armed = !t.armed;
                    saveSong();
                    triggerAsyncUpdate();
                },
                song.tracks[view.selected].armed ? 1 : 0);
        button(
            cs::mute, cell(r, s::compactButton),
            [this] {
                auto& t = song.tracks[view.selected];
                t.mute = !t.mute;
                if (performance)
                    performance->playback.updateMix(song);
                saveSong();
                triggerAsyncUpdate();
            },
            song.tracks[view.selected].mute ? 1 : 0);
        button(
            cs::solo, cell(r, s::compactButton),
            [this] {
                auto& t = song.tracks[view.selected];
                t.solo = !t.solo;
                if (performance)
                    performance->playback.updateMix(song);
                saveSong();
                triggerAsyncUpdate();
            },
            song.tracks[view.selected].solo ? 1 : 0);
        StringArray names;
        for (auto& t : song.tracks)
            names.add(t.name);
        if (!performance || !performance->recording)
            combo(cs::audioTracks, cell(r, s::fieldWidth), names, view.selected, [this](int i) {
                view.selected = i;
                view.selectedClip.clear();
                projectMonitor = 0;
                configureSongMonitor();
                view.save(projectDir);
                triggerAsyncUpdate();
            });
    }
}

void Studio::paintProject(Graphics& g) {
    const auto pos = playPosition();
    const auto beat = song.grid.beatAt(std::max<Frame>(pos, 0));
    const bool counting = performance && pos < performance->start;
    const bool rec = performance && performance->recording && !counting;
    const String state = counting      ? cs::countIn
                         : rec         ? cs::stateRecord
                         : performance ? cs::statePlay
                                       : cs::stateStop;
    const String clock =
        counting ? String(std::max<Frame>(1, -song.grid.beatAt(pos - performance->start)))
                 : String(beat / song.grid.beats + 1) + "." + String(beat % song.grid.beats + 1);
    if (view.distant) {
        auto r = getLocalBounds().withTrimmedBottom(s::footer + s::inset);
        g.setColour(rec ? s::recording : s::panel);
        g.fillRect(r);
        g.setColour(s::chalk);
        g.setFont(skin.font(s::distantRecord, false, true));
        g.drawText(state, r.removeFromTop(r.getHeight() / 2), Justification::centred);
        g.setFont(skin.font(s::distantBar, true));
        g.drawText(clock, r, Justification::centred);
        return;
    }
    if (rec) {
        g.setColour(s::recording);
        g.fillRect(getLocalBounds().removeFromTop(s::statusStrip + s::inset));
        g.setColour(s::record);
        g.drawRect(getLocalBounds(), s::recordFrame);
    }
    auto statusArea = ruler.withY(s::inset).withHeight(s::statusStrip);
    statusArea = statusArea.removeFromRight(s::fieldWidth);
    g.setColour(s::chalk);
    g.setFont(skin.font(s::heading, true));
    g.drawText(state + "   " + clock, statusArea, Justification::centredRight);
    auto sub = statusArea.removeFromBottom(s::caption + s::grid);
    g.setFont(skin.font(s::caption, true));
    g.setColour(s::muted);
    g.drawText(String(song.grid.bpm, 1) + " BPM   " + String(song.grid.beats) + "/" +
                   String(song.grid.unit) + "   " +
                   String(std::max<Frame>(pos, 0) / (song.grid.rate * 60)) + ":" +
                   String((std::max<Frame>(pos, 0) / song.grid.rate) % 60).paddedLeft('0', 2),
               sub, Justification::centredRight);
    if (view.selected < (int)song.tracks.size()) {
        auto& t = song.tracks[view.selected];
        auto barArea = inputMeter.withHeight(s::grid * 2).withCentre(inputMeter.getCentre());
        float peak =
            t.inputs.isEmpty() ? 0 : link.audio.read(link.caps.inputs.indexOf(t.inputs[0])).peak;
        g.setColour(s::line);
        g.fillRect(barArea);
        g.setColour(peak >= 0.98855f ? s::clipping : s::ready);
        g.fillRect(barArea.withWidth(
            roundToInt(barArea.getWidth() * std::clamp((decibels(peak) + 60.f) / 60.f, 0.f, 1.f))));
    }
    const Frame first = firstBar();
    const int bars = visibleBars();
    auto axis = ruler.withTrimmedLeft(s::trackHead);
    Frame begin = song.grid.beat(first * song.grid.beats),
          last = song.grid.beat((first + bars) * song.grid.beats);
    auto x = [&](Frame f) {
        return axis.getX() + roundToInt((double)(f - begin) / (last - begin) * axis.getWidth());
    };
    for (int i = 0; i < bars; ++i) {
        int left = x(song.grid.beat((first + i) * song.grid.beats));
        g.setColour(s::barGrid);
        g.drawVerticalLine(left, (float)axis.getY(), (float)timelineArea.getBottom());
        g.setFont(skin.font(view.editing ? s::heading : s::caption, true));
        g.setColour(s::muted);
        g.drawText(String(first + i + 1), left + s::grid, axis.getY(), s::scaleWidth, axis.getHeight(),
                   Justification::centredLeft);
    }
    for (size_t i = 0; i < lanes.size(); ++i) {
        const auto& t = song.tracks[firstTrack + i];
        auto lane = lanes[i].withTrimmedLeft(s::trackHead);
        g.setColour(s::line);
        g.drawHorizontalLine(lane.getBottom() - 1, (float)lane.getX(), (float)lane.getRight());
        auto ink = t.kind == "kytara" ? s::guitar : t.kind == "bici" ? s::drums : s::other;
        for (auto& c : song.clips)
            if (c.track == t.id && c.position < last && c.position + c.length > begin) {
                auto cache = library.data.find(c.asset);
                if (cache == library.data.end())
                    continue;
                auto bounds = lane.withLeft(x(std::max(c.position, begin)))
                                  .withRight(x(std::min(c.position + c.length, last)))
                                  .reduced(0, s::grid);
                g.setColour(ink.withAlpha(s::selectionOpacity));
                g.fillRect(bounds);
                g.setColour(ink);
                for (int px = bounds.getX(); px < bounds.getRight(); ++px) {
                    auto source = c.offset + begin - c.position +
                                  (Frame)((double)(px - axis.getX()) * (last - begin) / axis.getWidth());
                    source = std::clamp(source, c.offset, c.offset + c.length - 1);
                    int width = std::min<Frame>(c.offset + c.length - source,
                                                std::max<Frame>(1, (last - begin) / axis.getWidth()));
                    float peak = cache->second->peak(source, width);
                    float height = std::max(1.f, std::min(1.f, peak) * bounds.getHeight() / 2);
                    g.drawVerticalLine(px, bounds.getCentreY() - height, bounds.getCentreY() + height);
                }
            }
        if (rec && t.armed && pos > begin) {
            auto recorded = lane.withLeft(x(std::max(begin, performance->start)))
                                .withRight(x(std::min(pos, last)))
                                .reduced(0, s::grid);
            g.setColour(s::record.withAlpha(s::selectionOpacity));
            g.fillRect(recorded);
            g.setColour(ink);
            if (auto it = liveWaves.find(t.id); it != liveWaves.end())
                for (const auto& [frame, peak] : it->second) {
                    if (frame < begin || frame >= last)
                        continue;
                    const float height = std::max(1.f, std::min(1.f, peak) * recorded.getHeight() / 2);
                    g.drawVerticalLine(x(frame), recorded.getCentreY() - height,
                                       recorded.getCentreY() + height);
                }
        }
    }
    if(view.editing) paintEditOverlay(g);
    for (auto mark : song.dropouts)
        if (mark >= begin && mark < last) {
            g.setColour(s::warning);
            g.fillRect(x(mark), ruler.getY(), s::stroke, timelineArea.getBottom() - ruler.getY());
        }
    if (pos >= begin && pos <= last) {
        g.setColour(s::playhead);
        g.fillRect(x(pos), ruler.getY(), s::stroke, timelineArea.getBottom() - ruler.getY());
    }
}
void Studio::mouseDown(const MouseEvent& event) {
    if (page != Page::project || performance || view.distant)
        return;
    if (view.editing) { editMouseDown(event); return; }
    auto axis = ruler.withTrimmedLeft(s::trackHead);
    if (event.x < axis.getX() || event.x >= axis.getRight() || event.y < axis.getY() ||
        event.y >= timelineArea.getBottom())
        return;
    Frame bar = song.grid.beatAt(view.cursor) / song.grid.beats;
    Frame first = bar / view.zoom * view.zoom;
    Frame begin = song.grid.beat(first * song.grid.beats),
          last = song.grid.beat((first + view.zoom) * song.grid.beats);
    view.cursor = begin + (Frame)((double)(event.x - axis.getX()) * (last - begin) / axis.getWidth());
    view.save(projectDir);
    repaint();
}
void Studio::startLoadTrial() {
    if (performance || !link.online || link.caps.outputChannels == 0)
        return;
    diagnosticDir =
        File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("Rocker2-load", "", false);
    AudioLibrary sources;
    if (!makeLoadTrial(diagnosticSong, diagnosticDir, link.current.rate, trialAmps) ||
        !sources.load(diagnosticSong, diagnosticDir)) {
        sources.data.clear();
        diagnosticDir.deleteRecursively();
        status(cs::writeError);
        return;
    }
    link.audio.live.silence();
    performance = std::make_unique<Performance>(diagnosticSong, sources, 0, false, false, 0);
    performance->end = Frame(link.current.rate) * 600;
    initialXruns = devices.xruns();
    link.audio.peakLoad = 0;
    link.audio.measuringLoad = testingLoad = true;
    preventSleep(true);
    link.audio.timeline.attach(*performance);
    status(cs::trialRunning);
}
void Studio::finishLoadTrial(Performance& p) {
    link.audio.measuringLoad = testingLoad = false;
    preventSleep(false);
    const int current = devices.xruns();
    const String lost =
        initialXruns >= 0 && current >= initialXruns ? String(current - initialXruns) : cs::unreported;
    status((p.finished ? cs::trialFinished : cs::trialStopped) + "  " + cs::maxLoad + " " +
           String(link.audio.peakLoad.load(), 1).replaceCharacter('.', ',') + cs::percentUnit + "  /  " +
           cs::xruns + " " + lost);
}
void Studio::syncTest() {
    if (performance || !link.online || link.caps.inputs.isEmpty() || link.caps.outputChannels == 0)
        return;
    link.audio.live.silence();
    link.audio.stopTone();
    diagnosticDir =
        File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("Rocker2-sync", "", false);
    diagnosticSong = {};
    diagnosticSong.grid.rate = link.current.rate;
    diagnosticSong.grid.bpm = 120;
    diagnosticSong.tracks.push_back(
        {"return", "Return", "jine", {link.caps.inputs[measurementInput]}, true});
    auto p = std::make_unique<Performance>(diagnosticSong, AudioLibrary{}, 0, true, false, 0);
    int delay = std::max(0, listeningDelaySamples());
    if (!p->arm(diagnosticSong, diagnosticDir, link.caps.inputs, delay)) {
        status(cs::writeError);
        diagnosticDir.deleteRecursively();
        return;
    }
    p->syncPulse = true;
    p->end = Frame(link.current.rate) * 600 + delay;
    performance = std::move(p);
    diagnosing = true;
    preventSleep(true);
    link.audio.timeline.attach(*performance);
    status(cs::syncRunning);
}
void Studio::finishSync(Performance& p) {
    diagnosing = false;
    String result = cs::stopSong;
    if (p.finished && !p.failed && p.drop < 0 && recoverTake(diagnosticSong, diagnosticDir, p.journal) &&
        !diagnosticSong.assets.empty()) {
        auto reader = waveReader(diagnosticDir.getChildFile(diagnosticSong.assets.front().path));
        auto offsets = reader ? syncOffsets(*reader, diagnosticSong.grid, p.compensation) : std::nullopt;
        if (offsets) {
            auto [a, b] = *offsets;
            double scale = 1000. / diagnosticSong.grid.rate;
            result = cs::syncFirst + " " + String(a * scale, 2) + " ms  /  " + cs::syncLast + " " +
                     String(b * scale, 2) + " ms   " +
                     (std::max(std::abs(a), std::abs(b)) * scale <= 1 && std::abs(a - b) * scale <= 0.1
                          ? cs::syncPass
                          : cs::syncFail);
        } else
            result = cs::syncBad;
    } else if (p.finished)
        result = cs::syncBad;
    diagnosticDir.deleteRecursively();
    status(result);
}
} // namespace rocker
