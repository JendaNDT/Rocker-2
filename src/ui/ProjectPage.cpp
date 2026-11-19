#include "Studio.h"
#include "storage/ProjectFiles.h"
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
} // namespace
String Studio::limit(const Font& f, String value, int width) {
    if (GlyphArrangement::getStringWidth(f, value) <= width)
        return value;
    while (value.isNotEmpty() && GlyphArrangement::getStringWidth(f, value + String(u8"…")) > width)
        value = value.dropLastCharacters(1);
    return value + String(u8"…");
}
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
    if (projectDir != File{} && !song.title.isEmpty())
        history.remember(song, action.isEmpty() ? cs::edited : action);
    pendingSoundSave = 0;
    if (projectDir != File{} && !song.save(projectDir))
        status(cs::songSaveError);
}
void Studio::finishProject() {
    if (pendingSoundSave)
        saveSong();
    soundDeckOpen = namingPreset = false;
    if (performance)
        stopProject();
    if (projectDir != File{} && song.title.isNotEmpty()) {
        if (!view.save(projectDir) || !song.archive(projectDir))
            status(cs::songSaveError);
    }
}
void Studio::openProject(File dir) {
    if (dir == File{} || !dir.isDirectory()) {
        status(cs::noSong);
        return;
    }
    finishProject();
    exporting = false;
    delivered = {};
    lastMeter = {};
    lastLoudness.reset();
    selectedOccurrence = -1;
    askingRepair = false;
    const bool sameProject = projectDir == dir;
    projectDir = dir;
    comparing = false;
    preview = -1;
    previewGroup.clear();
    browsingVersions = false;
    drag = Drag::none;
    if (!sameProject)
        clipboard = {};

    library.data.clear();
    bool damaged = false;
    auto loaded = Song::load(dir, damaged);
    if (!loaded) {
        song = {};
        show(Page::rescue);
        return;
    }
    song = *loaded;
    if (!sameProject || !history.matches(song))
        history.reset(song);
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
    defaultMonitor();
    settings.lastSong = dir.getFullPathName();
    persist();
    lastArchive = Time::getMillisecondCounterHiRes();
    if (!dir.getChildFile("Zachrana").findChildFiles(File::findFiles, false, "*.json").isEmpty()) {
        show(Page::rescue);
        return;
    }
    show(Page::project);
    if (link.online && std::any_of(song.tracks.begin(), song.tracks.end(), [this](auto& t) {
            for (auto& port : t.inputs)
                if (!link.caps.inputs.contains(port))
                    return true;
            return false;
        }))
        beginRemap();
    if (syncedFolder(projectDir))
        status(cs::syncPath);
    if (damaged || !complete)
        status(cs::partialProject);
    else if (link.online && link.current.rate != song.grid.rate)
        status(cs::rateMismatch(song.grid.rate, link.current.rate));
}
void Studio::field(const String& label, Rect r, String& value) {
    text(label, r.removeFromLeft(s::labelWidth), s::body, s::muted);
    auto* edit = new TextEditor;
    controls.add(edit);
    addAndMakeVisible(edit);
    edit->setName(label);
    edit->setBounds(r.withWidth(std::min(r.getWidth(), s::fieldWidth)));
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
    link.audio.drums.monitor = false;
    if (view.selected < 0 || view.selected >= (int)song.tracks.size())
        return;
    auto& t = song.tracks[view.selected];
    if (t.kind == "midi") {
        link.audio.drums.kit = t.kit;
        link.audio.drums.monitor = !t.mute && activeMidi.isNotEmpty() && projectMonitor == 1;
    }
    if (t.kind != "kytara") {
        soundDeckOpen = soundDeckOpen && t.kind == "jine";
        namingPreset = false;
    }
    int port = link.caps.inputs.indexOf(t.inputs[0]);
    if (port < 0)
        return;
    live.selectInput(port);
    if (t.kind == "bici" && t.inputs.size() == 2)
        live.stereoRight = link.caps.inputs.indexOf(t.inputs[1]);
    live.setAmp(t.amp.values);
    if (projectMonitor == 2 && (laptop || t.kind != "kytara"))
        projectMonitor = 0;
    live.monitor =
        projectMonitor == 2 && !t.amp.enabled ? Monitor::clean : static_cast<Monitor>(projectMonitor);
}
Frame Studio::playPosition() const { return performance ? performance->cursor.load() : view.cursor; }
void Studio::toggleDistant() {
    if (auto* window = findParentComponentOfClass<ResizableWindow>()) {
        if (!view.distant)
            wasFullscreen = window->isFullScreen();
        window->setFullScreen(!view.distant || wasFullscreen);
    }
    offerDistant = false;
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
        draftKind = t.kind == "midi" ? t.kit : std::min(2, draftKind);
        draftPort = std::max(0, link.caps.inputs.indexOf(t.inputs[0]));
        draftChannels = std::max(1, t.inputs.size());
    }
    show(Page::trackSettings);
}
void Studio::startProject(bool record, bool repair, TakeGroup* auditionGroup) {
    if (performance)
        return;
    if (!link.online) {
        status(cs::noDeviceSong);
        return;
    }
    if (record && link.caps.separateClocks &&
        settings.clockOverride != Settings::calibrationKey(link.current)) {
        status(cs::clockConflict);
        return;
    }
    if (link.current.rate != song.grid.rate) {
        status(cs::rateMismatch(song.grid.rate, link.current.rate));
        return;
    }
    if (!library.load(song, projectDir)) {
        status(cs::partialProject);
        return;
    }
    if (!record && !auditionGroup && view.cursor >= song.end())
        view.cursor = 0;
    Frame start = view.cursor;

    if (repair) {
        start = std::max<Frame>(0, selection().from - song.grid.beat(view.leadBars * song.grid.beats));
    }
    auto p = std::make_unique<Performance>(song, library, start, record, view.click,
                                           record && view.countIn ? song.grid.beats : 0, view.rhythm);
    if (repair) {
        p->punchFrom = selection().from;
        p->punchTo = selection().to;
        p->monitorPunch = projectMonitor != 0;
        p->cleanPunch = projectMonitor == 1;
    }
    if (auditionGroup) {
        p->end = auditionGroup->to + song.grid.beat(song.grid.beats);
        selectKitAttempt(p->playback, *auditionGroup,
                         previewGroup == auditionGroup->id && preview >= 0 ? preview
                                                                           : auditionGroup->active);
    }
    if (record) {
        Song capture = song;
        if (repair)
            for (auto& t : capture.tracks)
                t.armed = song.linkedTrack(t.id, selectedTrack());
        int latency = link.caps.inputLatency + link.caps.outputLatency;
        auto saved = settings.latency.find(Settings::calibrationKey(link.current));
        if (saved != settings.latency.end())
            latency = saved->second.samples;
        if (std::none_of(capture.tracks.begin(), capture.tracks.end(), [](auto& t) { return t.armed; })) {
            status(cs::noneArmed);
            return;
        }
        for (auto& t : capture.tracks)
            if (t.armed && (t.kind == "midi"
                                ? activeMidi.isEmpty()
                                : t.inputs.isEmpty() || !link.caps.inputs.contains(t.inputs[0]) ||
                                      (t.inputs.size() > 1 && !link.caps.inputs.contains(t.inputs[1])))) {
                status(cs::missingInputs);
                return;
            }
        if (!p->arm(capture, projectDir, link.caps.inputs, latency, true,
                    link.caps.outputLatency +
                        settings.midiCorrections[Settings::calibrationKey(link.current) + activeMidi])) {
            status(p->diskStop ? cs::diskFull : cs::armError);
            return;
        }
        if (repair) {
            p->end = p->punchTo + song.grid.beat(view.tailBars * song.grid.beats) + p->compensation;
            link.audio.live.monitor = Monitor::off;
        }
        offerDistant =
            !view.distant && std::any_of(capture.tracks.begin(), capture.tracks.end(), [](auto& t) {
                return t.armed && (t.kind == "bici" || t.kind == "midi");
            });
    }
    preventSleep(true);
    liveWaves.clear();
    performance = std::move(p);
    link.audio.timeline.attach(*performance);
    if (repair) {
        auto* group = song.groupAt(selectedTrack(), selection().from, selection().to);
        status(cs::repairState + " · " + cs::bars + " " +
               String(song.grid.beatAt(selection().from) / song.grid.beats + 1) + " · " +
               cs::attemptLabel + " " + String(group ? (int)group->attempts.size() + 2 : 2));
    } else
        status({});
}
void Studio::stopProject(bool automatic) {
    if (!performance)
        return;
    link.audio.timeline.detach();
    comparing = false;
    preview = -1;
    previewGroup.clear();
    auto p = std::move(performance);
    if (!testingLoad && !diagnosing)
        lastMeter = p->playback.meter();
    p->finishDisk();
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
    bool complete = false;
    offerDistant = false;
    if (p->recording) {
        const auto before = song.assets.size();
        p->storeInputMarks(song);
        if (!recoverTake(song, projectDir, p->journal) || !library.load(song, projectDir) ||
            p->failed.load())
            status(p->journal.existsAsFile() ? cs::takeNeedsRecovery : cs::takeSaveError);
        else {
            if (p->drop < 0 && song.assets.size() > before)
                ++view.savedTakes;
            complete = p->finished && !p->failed && !p->diskStop && p->drop < 0;
            const Frame sec = view.cursor / song.grid.rate;
            status(p->diskStop
                       ? cs::diskStop + " " + String(sec / 60) + ":" + String(sec % 60).paddedLeft('0', 2)
                   : p->drop >= 0 ? cs::dropout
                                  : cs::savedSong);
        }
    }
    if (p->recording)
        history.remember(song, cs::newTake);
    if (p->isPunch()) {
        configureSongMonitor();
        if (auto* g = song.groupAt(selectedTrack(), p->punchFrom, p->punchTo);
            g && automatic && complete) {
            preview = g->active;
            previewGroup = g->id;
            audition(*g);
        }
    }
    if (!view.save(projectDir))
        status(cs::songSaveError);
    triggerAsyncUpdate();
}
void Studio::projectTick() {
    pollExport();
    pollFiles();
    if (performance && performance->diskWarning && !performance->diskStop && message != cs::diskLow)
        status(cs::diskLow);
    if (performance && performance->finished.load()) {
        stopProject(true);
        return;
    }
    if (performance && performance->recording && !diagnosing && !view.distant) {
        auto frame = playPosition();
        if (frame >= performance->start)
            for (const auto& t : song.tracks)
                if ((performance->isPunch() ? song.linkedTrack(t.id, selectedTrack()) : t.armed) &&
                    !t.inputs.isEmpty()) {
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
            status(cs::songSaveError);
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
                            status(cs::takeNeedsRecovery);
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
        } else if (view.selected < (int)song.tracks.size() && song.tracks[view.selected].kind == "midi") {
            auto& track = song.tracks[view.selected];
            combo(cs::kitPreset, row(area), cs::drumSounds, track.kit, [this](int i) { draftKind = i; });
        } else {
            if (view.selected == (int)song.tracks.size() && !drumTracks().empty())
                button(cs::insertKit, row(area).withWidth(s::labelWidth), [this] {
                    auto tracks = drumTracks();
                    if (tracks.empty())
                        return;
                    song.tracks.insert(song.tracks.end(), tracks.begin(), tracks.end());
                    defaultMonitor();
                    saveSong();
                    view.save(projectDir);
                    show(Page::project);
                });
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
                            if (i == 0) {
                                auto tracks = drumTracks();
                                if (!tracks.empty()) {
                                    fresh.tracks = std::move(tracks);
                                    continue;
                                }
                            }
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
                            status(cs::songSaveError);
                            return;
                        }
                        openProject(folder);
                        return;
                    }
                    song.title = draftTitle.trim();
                    song.grid.bpm = bpm;
                    song.grid.beats = draftBeats;
                    song.grid.unit = draftUnit;
                    song.projectOrder();
                } else {
                    if (draftTrack.trim().isEmpty())
                        return;
                    if (isPositiveAndBelow(view.selected, (int)song.tracks.size()) &&
                        song.tracks[view.selected].kind == "midi") {
                        auto& t = song.tracks[view.selected];
                        t.name = draftTrack.trim();
                        t.kit = std::clamp(draftKind, 0, 1);
                        if (!library.load(song, projectDir)) {
                            status(cs::partialProject);
                            return;
                        }
                        saveSong();
                        show(Page::project);
                        return;
                    }
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
        if (!performance) {
            const int targets[]{0, 4, 8, 16, 32};
            int selected = 0;
            for (int i = 0; i < 5; ++i)
                if (view.targetBars == targets[i])
                    selected = i;
            combo(cs::farTarget, cell(footer, s::fieldWidth), {cs::farFree, "4", "8", "16", "32"},
                  selected, [this](int i) {
                      const int bars[]{0, 4, 8, 16, 32};
                      view.targetBars = bars[i];
                      view.save(projectDir);
                      repaint();
                  });
        }
        text(message, footer, s::body, s::warning);
        return;
    }
    if (offerDistant)
        button(cs::farOffer, row(area).withWidth(s::fieldWidth), [this] { toggleDistant(); }, 1);
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
    if (!view.mixing && soloNames().isNotEmpty()) {
        auto room = top.withTrimmedRight(s::fieldWidth);
        text(limit(skin.font(s::caption), cs::soloStatus + soloNames(), room.getWidth()), room,
             s::caption, s::warning);
    }
    const bool mismatched = link.online && link.current.rate != song.grid.rate;
    if (mismatched && !performance) {
        auto rateRow = row(area);
        if (link.caps.rates.contains(double(song.grid.rate))) {
            button(cs::switchRate(song.grid.rate), rateRow.removeFromRight(s::fieldWidth), [this] {
                const auto previous = link.current;
                auto choice = previous;
                choice.rate = song.grid.rate;
                choose(choice, true);
                if (!link.online) {
                    choose(previous);
                    status(cs::failed);
                    return;
                }
                if (link.current.rate != choice.rate) {
                    status(cs::rateMismatch(choice.rate, link.current.rate));
                    return;
                }
                configureSongMonitor();
            });
        } else {
            text(cs::rateUnavailable(song.grid.rate), rateRow.removeFromRight(s::fieldWidth), s::body,
                 s::warning);
        }
        text(cs::rateMismatch(song.grid.rate, link.current.rate), rateRow, s::body, s::warning);
    }
    auto modes = row(area);
    button(
        cs::playMode, cell(modes, s::compactButton), [this] { editMode(false); },
        (view.editing || view.mixing) ? 0 : 1);
    button(
        cs::editMode, cell(modes, s::compactButton), [this] { editMode(true); }, view.editing ? 1 : 0);
    button(cs::mixMode, cell(modes, s::compactButton), [this] { mixMode(false); }, view.mixing ? 1 : 0);
    if (view.mixing) {
        button(cs::sendMix, cell(modes, s::clipWidth), [this] { mixMode(true); });
        auto names = soloNames();
        text(limit(skin.font(s::body), cs::soloStatus + (names.isEmpty() ? cs::noSolo : names),
                   modes.getWidth()),
             modes, s::body, names.isEmpty() ? s::muted : s::warning);
        if (exporting)
            exportPage(area);
        else
            mixPage(area);
        return;
    }
    if (!performance && !view.editing) {
        button(cs::addBacking, cell(modes, s::clipWidth), [this] {
            chooseImport();
        })->setEnabled(!importing.valid());
        button(importAtCursor ? cs::atCursor : cs::atBeginning, cell(modes, s::nameWidth), [this] {
            importAtCursor = !importAtCursor;
            triggerAsyncUpdate();
        });
    }
    if (!performance)
        button(cs::fileMenu, cell(modes, s::compactButton), [this] { showVersions(); });
    if (view.editing) {
        button(cs::beforeBars, cell(modes, s::clipWidth), [this] {
            view.firstBar = std::max<Frame>(0, view.firstBar - visibleBars());
            view.save(projectDir);
            repaint();
        });
        button(cs::afterBars, cell(modes, s::clipWidth), [this] {
            view.firstBar = std::min<Frame>(999999, view.firstBar + visibleBars());
            view.save(projectDir);
            repaint();
        });
        button(cs::zoomIn, cell(modes, s::compactButton), [this] { editKey(KeyPress('+')); });
        button(cs::zoomOut, cell(modes, s::compactButton), [this] { editKey(KeyPress('-')); });
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
        const auto guide = !view.click ? cs::guideOff : view.rhythm ? cs::guideRhythm : cs::songClick;
        button(
            guide, cell(tools, s::compactButton),
            [this] {
                if (!view.click) {
                    view.click = true;
                    view.rhythm = false;
                } else if (!view.rhythm) {
                    view.rhythm = true;
                } else {
                    view.click = false;
                    view.rhythm = false;
                }
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
    if (!performance && isPositiveAndBelow(view.selected, (int)song.tracks.size())) {
        const auto& selected = song.tracks[view.selected];
        for (size_t i = 0; i < song.tracks.size(); ++i)
            if (selected.group.isNotEmpty() && song.tracks[i].group == selected.group &&
                song.tracks[i].kind == "midi") {
                const bool toMidi = song.tracks[i].mute;
                button(toMidi ? cs::midiListen : cs::moduleListen, cell(tools, s::clipWidth),
                       [this, id = selected.group, toMidi] {
                           for (int n = 0; n < (int)song.tracks.size(); ++n)
                               if (song.tracks[n].group == id) {
                                   auto& t = song.tracks[n];
                                   t.mute = (t.kind == "midi") != toMidi;
                                   if (!t.mute)
                                       view.selected = n;
                               }
                           defaultMonitor();
                           configureSongMonitor();
                           saveSong();
                           view.save(projectDir);
                           triggerAsyncUpdate();
                       });
                break;
            }
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
            if (playPosition() < performance->start)
                heard.add(cs::songClick);
            else if (view.click)
                heard.add(view.rhythm ? cs::guideRhythm : cs::songClick);
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
                armed.add((t.kind == "midi" ? cs::midiModule : t.inputs.joinIntoString(" + ")) +
                          String(u8" → ") + t.name);
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
            else if (song.tracks[view.selected].kind == "jine")
                button(cs::voicePanel, cell(controlsRow, s::clipWidth),
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
    if (!mismatched || message != cs::rateMismatch(song.grid.rate, link.current.rate))
        text(message, notice, s::caption, s::warning);
    sectionStrip = row(area);
    text(cs::sections, sectionStrip.withWidth(s::trackHead), s::body, s::muted);
    ruler = row(area);
    text(cs::bars, ruler.withWidth(s::trackHead), s::body, s::guitar);
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
                    defaultMonitor();
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
        } else if (t.kind == "jine") {
            button(cs::voicePresets[t.voice.preset], head, [this, i] { openTrackSound(i); });
        } else
            text(limit(skin.font(s::caption),
                       t.kind == "midi" ? cs::drumSounds[t.kit] : t.inputs.joinIntoString(" + "),
                       head.getWidth()),
                 head, s::caption, s::muted);
    }
    if (!song.tracks.empty()) {
        auto r = trackCommands;
        if (!performance)
            button(
                song.tracks[view.selected].armed ? cs::disarm : cs::arm, cell(r, s::levelWidth),
                [this] {
                    auto& t = song.tracks[view.selected];
                    const bool enable = !t.armed;
                    for (auto& member : song.tracks)
                        if (song.linkedTrack(member.id, t.id))
                            member.armed = enable;
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
                defaultMonitor();
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
    const String state = performance && performance->isPunch() && !counting ? cs::repairState
                         : counting                                         ? cs::countIn
                         : rec                                              ? cs::stateRecord
                         : performance                                      ? cs::statePlay
                                                                            : cs::stateStop;
    const String clock =
        counting ? String(std::max<Frame>(1, -song.grid.beatAt(pos - performance->start)))
                 : String(beat / song.grid.beats + 1) + "." + String(beat % song.grid.beats + 1);
    if (view.distant) {
        auto r = getLocalBounds().withTrimmedBottom(s::footer + s::inset);
        const bool saved = !performance && view.savedTakes > 0 && message == cs::savedSong;
        g.setColour(counting ? s::warning : rec ? s::recording : saved ? s::ready : s::panel);
        g.fillRect(r);
        g.setColour(counting || saved ? s::background : s::chalk);
        g.setFont(skin.font(s::distantRecord, false, true));
        String label = performance ? state : saved ? cs::farSaved : cs::farReady;
        auto top = r.removeFromTop(r.getHeight() / 3);
        g.drawText(label, top, Justification::centred);
        auto line = r.removeFromBottom(s::header + s::gutter).reduced(s::margin, s::inset);
        auto note = r.removeFromBottom(s::header);
        g.setFont(skin.font(s::counter, true));
        String track = isPositiveAndBelow(view.selected, (int)song.tracks.size())
                           ? song.tracks[view.selected].name
                           : String{};
        g.drawText(track + cs::divider + cs::attemptLabel + " " +
                       String(view.savedTakes + (saved ? 0 : 1)),
                   note, Justification::centred);
        g.setFont(skin.font(s::distantBar, true));
        String bar = counting ? clock : String(beat / song.grid.beats + 1);
        if (!counting && view.targetBars > 0)
            bar += " / " + String(view.targetBars);
        g.drawText(bar, r, Justification::centred);
        auto seconds = std::max<Frame>(0, pos) / song.grid.rate;
        g.setFont(skin.font(s::counter, true));
        g.drawText(String(seconds / 60) + ":" + String(seconds % 60).paddedLeft('0', 2) + cs::divider +
                       String(song.grid.bpm, 0) + " BPM",
                   line.removeFromRight(s::fieldWidth), Justification::centredRight);
        const int size = std::min(s::button, line.getWidth() / std::max(1, song.grid.beats) - s::grid);
        auto step = counting ? song.grid.beatAt(pos - performance->start) : beat;
        int pulse = (int)((step % song.grid.beats + song.grid.beats) % song.grid.beats);
        for (int i = 0; i < song.grid.beats; ++i) {
            auto square = line.removeFromLeft(size + s::grid).withSizeKeepingCentre(size, size);
            if (performance && i <= pulse)
                g.fillRect(square);
            else
                g.drawRect(square, s::stroke);
        }
        return;
    }
    if (view.mixing && exporting)
        return;
    if (view.mixing)
        paintMix(g);
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
    if (!view.mixing && view.selected < (int)song.tracks.size()) {
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
    drawSections(g);
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
        auto ink = t.kind == "kytara"                       ? s::guitar
                   : (t.kind == "bici" || t.kind == "midi") ? s::drums
                                                            : s::other;
        auto drawClip = [&](const Clip& c, Frame firstFrame, Frame lastFrame, bool cover) {
            if (c.track == t.id && lastFrame > begin && firstFrame < last && lastFrame > firstFrame) {
                auto cache = library.waveFor(t, c.asset);
                if (cache == library.data.end())
                    return;
                auto bounds = lane.withLeft(x(std::max(firstFrame, begin)))
                                  .withRight(x(std::min(lastFrame, last)))
                                  .reduced(0, s::grid);
                if (cover) {
                    g.setColour(s::panel);
                    g.fillRect(bounds);
                }
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
        };
        for (const auto& clip : song.clips)
            drawClip(clip, clip.position, clip.position + clip.length, false);
        for (const auto& group : song.groups) {
            int index = previewGroup == group.id && preview >= 0 ? preview : group.active;
            if (group.track == t.id && index > 0 && index <= (int)group.attempts.size())
                for (const auto& pass : group.attempts[index - 1].clips)
                    drawClip(pass, std::max(pass.position, group.from),
                             std::min(pass.position + pass.length, group.to), true);
        }
        if (rec && (performance->isPunch() ? song.linkedTrack(t.id, selectedTrack()) : t.armed) &&
            pos > begin) {
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
    if (view.editing)
        paintEditOverlay(g);
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
    if (view.mixing && exporting)
        return;
    if (view.mixing && (sectionStrip.withTrimmedLeft(s::trackHead).contains(event.getPosition()) ||
                        timelineArea.withTrimmedLeft(s::trackHead).contains(event.getPosition()))) {
        seekMix(frameAt(event.x));
        return;
    }
    if (sectionStrip.withTrimmedLeft(s::trackHead).contains(event.getPosition())) {
        if (!view.editing)
            editMode(true);
        sectionDown(event);
        return;
    }
    if (view.editing) {
        selectedOccurrence = -1;
        askingRepair = false;
        editMouseDown(event);
        return;
    }
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
        status(cs::diagnosticError);
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
        status(cs::diagnosticError);
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
