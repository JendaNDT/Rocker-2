#include "Studio.h"
#include "Strings.h"

namespace rocker {
using namespace juce;
namespace s = style;
namespace {
using Rect = Rectangle<int>;
Rect line(Rect& area, int height = s::button) {
    auto part = area.removeFromTop(height);
    area.removeFromTop(s::gap);
    return part;
}
String db(float level) {
    return level > 0 ? String(Decibels::gainToDecibels(level), 1).replaceCharacter('.', ',') + " dBFS"
                     : cs::silence;
}
String loudness(const LoudnessReading& measurement) {
    if (!std::isfinite(measurement.integrated))
        return cs::loudnessLabel + " " + cs::silence;
    return cs::loudnessLabel + " " + String(measurement.integrated, 1).replaceCharacter('.', ',') +
           cs::lufsUnit;
}
String bar(const Song& song, Frame pos) {
    return cs::bar + " " + String(song.grid.beatAt(std::max<Frame>(0, pos)) / song.grid.beats + 1);
}
bool ready(auto& future) {
    return future.valid() && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
} // namespace
String Studio::soloNames() const {
    StringArray names;
    for (auto& t : song.tracks)
        if (t.solo)
            names.add(t.name);
    return names.joinIntoString(", ");
}
void Studio::mixMode(bool panel) {
    if (!view.mixing) {
        view.firstBar = firstBar();
        pendingModeReveal = true;
        if (view.selected < firstTrack || view.selected >= firstTrack + (int)lanes.size())
            firstTrack = view.selected;
    }
    exporting = panel;
    view.mixing = true;
    view.editing = false;
    selectedOccurrence = -1;
    browsingVersions = soundDeckOpen = namingPreset = false;
    drag = Drag::none;
    if (!selection().valid())
        selectedRange = false;
    saveView();
    triggerAsyncUpdate();
}
void Studio::seekMix(Frame frame) {
    if (performance)
        stopProject();
    view.cursor = std::clamp<Frame>(frame, 0, song.end());
    saveView();
    triggerAsyncUpdate();
}
void Studio::toggleMix(int i, bool solo) {
    if (!isPositiveAndBelow(i, (int)song.tracks.size()))
        return;
    auto& t = song.tracks[i];
    auto& flag = solo ? t.solo : t.mute;
    flag = !flag;
    if (!performance || !performance->recording)
        view.selected = i;
    if (performance)
        performance->playback.updateMix(song);
    saveSong();
    saveView();
    triggerAsyncUpdate();
}
void Studio::mixPage(Rect area) {
    stereoMeter = mixReadout = {};
    if (!mixInspectorCollapsed)
        inspectorBounds = area.removeFromRight(s::inspectorWidth);
    auto notice = area.removeFromBottom(s::button).reduced(s::inset, 0);
    takeActions(notice);
    auto trackNavigation = area.removeFromBottom(s::button).reduced(s::inset, 0);
    const int laneWidth = s::mixTrackHead;
    auto axis = [&](Rect r) { return r.withTrimmedLeft(laneWidth - s::trackHead); };
    auto header = area.removeFromTop(s::workspaceRuler / 2);
    sectionStrip = axis(header);
    text(cs::trackLabel, header.removeFromLeft(s::mixNameWidth).withTrimmedLeft(s::inset),
         s::caption, s::navigationInk);
    text(cs::gainLabel, header.removeFromLeft(s::mixGainWidth), s::caption, s::navigationInk);
    text(cs::panLabel, header.removeFromLeft(s::levelWidth), s::caption, s::navigationInk);
    ruler = axis(area.removeFromTop(s::workspaceRuler / 2));
    if (pendingModeReveal) {
        reveal(playPosition());
        pendingModeReveal = false;
        saveView();
    }
    lanes.clear();
    trackMeters.clear();
    const int visible = std::max(1, area.getHeight() / s::workspaceLane);
    firstTrack = std::clamp(firstTrack, 0, std::max(0, (int)song.tracks.size() - visible));
    for (int i = firstTrack; i < std::min((int)song.tracks.size(), firstTrack + visible); ++i) {
        const auto& t = song.tracks[i];
        auto lane = area.removeFromTop(s::workspaceLane);
        lanes.push_back(axis(lane));
        auto head = lane.removeFromLeft(laneWidth).reduced(s::grid);
        auto row = head.removeFromTop(s::button);
        auto nameArea = row.removeFromLeft(s::mixNameWidth).reduced(s::grid, 0);
        auto label = button(
            limit(skin.font(s::body, false, true), t.name, nameArea.getWidth() - 2 * s::inset), nameArea,
            [this, i] { selectProjectTrack(i); }, view.selected == i ? 1 : 0);
        label->setTooltip(t.name);
        label->setEnabled(!performance || !performance->recording);
        mixSlider(cs::gainLabel + " " + t.name, row.removeFromLeft(s::mixGainWidth), t.gainDb, false,
                  [this, i](float v) { song.tracks[i].gainDb = v; });
        auto toggles = row.removeFromRight(2 * (s::trackToggle + s::grid));
        mixSlider(cs::panLabel + " " + t.name, row, t.pan, true,
                  [this, i](float v) { song.tracks[i].pan = v; });
        auto toggle = [&](const String& description, const String& name, bool solo, bool set, int active) {
            auto* control = button(description + cs::divider + t.name,
                                   toggles.removeFromLeft(s::trackToggle), [this, i, solo] {
                                       toggleMix(i, solo);
                                   }, set ? active : plain, {}, Icon::none, name);
            control->setTooltip(description);
            control->setToggleState(set, dontSendNotification);
            toggles.removeFromLeft(s::grid);
        };
        toggle(cs::mute, cs::muteShort, false, t.mute, muteActive);
        toggle(cs::solo, cs::soloShort, true, t.solo, soloActive);
        auto details = head.removeFromTop(s::button);
        auto missing = missingPorts(t);
        if (missing.isNotEmpty()) {
            auto route = details.removeFromLeft(s::mixNameWidth);
            button(cs::chooseInput + cs::divider + t.name, route,
                   [this, i] { beginRemap(i); }, plain, {}, Icon::none, cs::chooseInput)->setEnabled(!performance);
            auto warning = details.removeFromLeft(s::mixGainWidth);
            text(limit(skin.font(s::caption), missing, warning.getWidth()), warning, s::caption, s::warning);
        } else if (t.kind == "kytara")
            text(limit(skin.font(s::caption), t.amp.name, s::mixNameWidth - 2 * s::grid),
                 details.removeFromLeft(s::mixNameWidth).reduced(s::grid, 0), s::caption, s::guitar);
        else
            details.removeFromLeft(s::mixNameWidth);
        if (t.inputOver >= 0)
            button(cs::inputOver + cs::divider + bar(song, t.inputOver), details.removeFromRight(s::labelWidth),
                   [this, frame = t.inputOver] { seekMix(frame); });
        trackMeters.push_back(details);
    }
    timelineArea = ruler.withBottom(lanes.empty() ? ruler.getBottom() : lanes.back().getBottom());
    if (firstTrack > 0)
        button(cs::prevTracks, trackNavigation.removeFromLeft(s::clipWidth), [this, visible] {
            firstTrack -= visible;
            triggerAsyncUpdate();
        });
    if (firstTrack + visible < (int)song.tracks.size())
        button(cs::nextTracks, trackNavigation.removeFromLeft(s::clipWidth), [this, visible] {
            firstTrack += visible;
            triggerAsyncUpdate();
        });
    text(limit(skin.font(s::caption), message, notice.getWidth()), notice, s::caption, s::warning);
    if (mixInspectorCollapsed)
        return;

    auto panel = inspectorBounds.reduced(s::inset);
    text(cs::master, line(panel), s::workspaceTitle, s::chalk, false, true);
    mixSlider(cs::master, line(panel, s::transport), song.masterDb, false,
              [this](float v) { song.masterDb = v; });
    stereoMeter = line(panel, s::transport);
    mixReadout = line(panel, s::workspaceTitle);
    auto solo = soloNames();
    auto label = line(panel);
    text(limit(skin.font(s::caption), cs::soloStatus + (solo.isEmpty() ? cs::noSolo : solo), label.getWidth()),
         label, s::caption, solo.isEmpty() ? s::muted : s::warning);
    if (lastLoudness)
        text(cs::lastDelivery + loudness(*lastLoudness), line(panel), s::caption, s::muted);
    auto meter = performance ? performance->playback.meter() : lastMeter;
    if (meter.over >= 0)
        button(cs::mixOver + cs::divider + bar(song, meter.over), line(panel),
               [this, frame = meter.over] { seekMix(frame); });
    else
        text(cs::noOver, line(panel), s::body, s::ready);
    if (isPositiveAndBelow(view.selected, (int)song.tracks.size()) &&
        song.tracks[view.selected].group.isNotEmpty()) {
        auto& t = song.tracks[view.selected];
        text(cs::kitGain, line(panel), s::caption, s::muted);
        mixSlider(cs::kitGain, line(panel), t.groupDb, false, [this, id = t.group](float gain) {
            for (auto& member : song.tracks)
                if (member.group == id)
                    member.groupDb = gain;
        });
    }
    auto send = panel.removeFromBottom(s::transport);
    button(cs::sendMix, send, [this] { mixMode(true); }, primary, {}, Icon::exportMix);
}
void Studio::paintMix(Graphics& g) {
    const auto m = performance ? performance->playback.meter() : lastMeter;
    auto draw = [&](Rect r, float level, const String& name) {
        if (r.isEmpty())
            return;
        g.setFont(skin.font(s::caption, true));
        if (name.isNotEmpty()) {
            g.setColour(s::muted);
            g.drawText(name, r.removeFromLeft(s::inset), Justification::centredLeft);
        }
        g.setColour(s::chalk);
        g.drawText(db(level), r.removeFromRight(s::scaleWidth), Justification::centredRight);
        auto rail = r.withSizeKeepingCentre(r.getWidth(), s::grid * 2);
        g.setColour(s::line);
        g.fillRect(rail);
        auto proportion = std::clamp((Decibels::gainToDecibels(level) + 60.f) / 60.f, 0.f, 1.f);
        g.setColour(level > 1 ? s::clipping : s::ready);
        g.fillRect(rail.withWidth(roundToInt(rail.getWidth() * proportion)));
    };
    for (size_t i = 0; i < trackMeters.size(); ++i)
        draw(trackMeters[i], performance ? performance->playback.trackPeak(firstTrack + i) : 0, {});
    auto r = stereoMeter;
    draw(r.removeFromTop(r.getHeight() / 2), performance ? m.left : 0, cs::leftShort);
    draw(r, performance ? m.right : 0, cs::rightShort);
    g.setColour(m.maximum > 1 ? s::clipping : s::chalk);
    g.drawText(cs::mixPeak + " " + db(m.maximum) +
                   (m.loudest >= 0 ? cs::divider + bar(song, m.loudest) : String{}),
               mixReadout, Justification::centredLeft);
}
void Studio::exportPage(Rect area) {
    text(cs::sendMix, line(area, s::header), s::title);
    auto choices = line(area);
    text(cs::formatLabel, choices.removeFromLeft(s::levelWidth));
    const String labels[]{cs::wave24, cs::wave16, cs::mp3, cs::m4a, cs::stems};
    for (int i = 0; i < int(std::size(labels)); ++i) {
        const auto choice = Delivery(i);
        if (!deliveryAvailable(choice))
            continue;
        auto b = button(
            labels[i], choices.removeFromLeft(s::nameWidth).reduced(s::grid, 0),
            [this, choice] {
                if (format != choice)
                    delivered = {};
                format = choice;
                triggerAsyncUpdate();
            },
            format == choice ? 1 : 0);
        b->setEnabled(!delivery.valid());
    }
    const bool stems = format == Delivery::stems;
    const bool noStems =
        stems && std::none_of(song.tracks.begin(), song.tracks.end(), [](const auto& track) { return !track.mute; });
    if (stems)
        text(noStems ? cs::allStemsMuted : cs::stemsHelp, line(area), s::body);
    else {
        auto scope = line(area);
        text(cs::rangeLabel, scope.removeFromLeft(s::levelWidth));
        button(
            cs::wholeSong, scope.removeFromLeft(s::nameWidth),
            [this] {
                selectedRange = false;
                triggerAsyncUpdate();
            },
            selectedRange ? 0 : 1)
            ->setEnabled(!delivery.valid());
        button(
            cs::rangeOnly, scope.removeFromLeft(s::nameWidth),
            [this] {
                selectedRange = true;
                triggerAsyncUpdate();
            },
            selectedRange ? 1 : 0)
            ->setEnabled(!delivery.valid() && selection().valid());
        if (selection().valid())
            text(bar(song, selection().from) + cs::rangeDash + bar(song, selection().to - 1), scope, s::body,
                 s::muted);
    }
    if (!stems) {
        auto level = line(area);
        button(cs::normalExport, level.removeFromLeft(s::nameWidth), [this] {
            louderExport = false;
            delivered = {};
            triggerAsyncUpdate();
        }, louderExport ? 0 : 1)->setEnabled(!delivery.valid());
        button(cs::louderExport, level.removeFromLeft(s::labelWidth), [this] {
            louderExport = true;
            delivered = {};
            triggerAsyncUpdate();
        }, louderExport ? 1 : 0)->setEnabled(!delivery.valid());
    }
    auto solo = soloNames();
    if (solo.isNotEmpty())
        text(stems ? cs::stemsSolo : cs::soloNotice, line(area), s::body, s::warning);
    auto actions = line(area);
    button(
        delivery.valid() ? cs::exporting
        : stems ? cs::makeStems
        : solo.isEmpty() ? cs::makeFile
                         : cs::exportSolo,
        actions.removeFromLeft(s::labelWidth), [this] { beginExport(); }, 1)
        ->setEnabled(!delivery.valid() && !noStems);
    button(cs::back, actions.removeFromLeft(s::levelWidth), [this] { mixMode(false); });
    if (delivered.ok) {
        text(limit(skin.font(s::heading), delivered.file.getFileName(), area.getWidth()), line(area),
             s::heading);
        if (delivered.file.isDirectory()) {
            button(cs::mixFolder, line(area).withWidth(s::nameWidth), [this] { delivered.file.revealToUser(); });
            return;
        }
        auto m = delivered.meter;
        text(cs::mixPeak + " " + db(m.maximum) +
                 (m.loudest >= 0 ? cs::divider + bar(song, m.loudest) : String{}),
             line(area), s::body, m.over >= 0 ? s::clipping : s::chalk);
        if (delivered.loudness) {
            const auto& stats = *delivered.loudness;
            text(loudness(stats) + cs::divider + cs::truePeakLabel + " " +
                     String(Decibels::gainToDecibels(stats.peak), 1).replaceCharacter('.', ',') + cs::truePeakUnit,
                 line(area), s::body);
        }
        auto play = line(area);
        button(cs::playMix, play.removeFromLeft(s::labelWidth), [this] {
            if (!delivered.file.startAsProcess())
                status(cs::noPlayer);
        });
        button(cs::mixFolder, play.removeFromLeft(s::nameWidth),
               [this] { delivered.file.revealToUser(); });
        if (m.over >= 0) {
            auto repair = line(area);
            button(cs::mixOver + cs::divider + bar(song, m.over), repair.removeFromLeft(s::labelWidth),
                   [this, m] {
                       mixMode(false);
                       seekMix(m.over);
                   });
            float reduction = std::ceil((Decibels::gainToDecibels(m.maximum) + 1.f) * 10) / 10.f;
            button(cs::reduceMix + String(reduction, 1).replaceCharacter('.', ',') + cs::retryMix,
                   repair.removeFromLeft(s::fieldWidth),
                   [this, reduction] {
                       song.masterDb = std::max(-96.f, song.masterDb - reduction);
                       if (performance)
                           performance->playback.updateMix(song);
                       saveSong();
                       beginExport();
                   })
                ->setEnabled(!delivery.valid());
        }
    }
    text(message, line(area), s::body, s::warning);
}
void Studio::beginExport() {
    if (delivery.valid())
        return;
    if (pendingSoundSave)
        saveSong();
    auto span = selectedRange ? selection() : Span{0, -1};
    auto folder = projectDir.getChildFile("Exporty");
    auto date = Time::getCurrentTime().formatted("%Y-%m-%d");
    const bool stems = format == Delivery::stems;
    const auto ext = stems ? "" : format == Delivery::mp3 ? ".mp3" : format == Delivery::m4a ? ".m4a" : ".wav";
    const bool louder = louderExport && !stems;
    const auto name = File::createLegalFileName(song.title + (stems ? cs::stemsSuffix : cs::mixSuffix) + date +
                                                (louder ? cs::louderSuffix : String{}));
    auto file = folder.getNonexistentChildFile(name, ext, true);
    deliveringProject = projectDir;
    delivered = {};
    message.clear();
    delivery = std::async(std::launch::async, [copy = song, dir = projectDir, file, type = format, span, louder] {
        AudioLibrary lib;
        if (!lib.load(copy, dir))
            return MixFile{file};
        if (type == Delivery::stems)
            return exportStems(copy, lib, file, cs::trackKinds[0]);
        return exportMix(copy, lib, file, type, span.from, span.to, louder);
    });
    triggerAsyncUpdate();
}
void Studio::pollExport() {
    if (ready(delivery)) {
        MixFile result;
        try {
            result = delivery.get();
        } catch (const std::exception&) {
        }
        if (projectDir == deliveringProject) {
            delivered = result;
            if (result.ok) {
                lastMeter = result.meter;
                if (result.loudness)
                    lastLoudness = result.loudness;
                if (page == Page::project && exporting && getPeer() != nullptr)
                    result.file.revealToUser();
            } else
                message = cs::exportFailed;
        }
        triggerAsyncUpdate();
    }
    if (ready(clickCheck)) {
        std::optional<bool> found;
        try {
            found = clickCheck.get();
        } catch (const std::exception&) {
        }
        if (page == Page::diagnostics)
            status(!found ? cs::clickFailed : *found ? cs::clickFound : cs::clickAbsent);
    }
    if (page == Page::project && view.mixing && performance) {
        auto m = performance->playback.meter();
        if (m.over != lastMeter.over) {
            lastMeter = m;
            triggerAsyncUpdate();
        }
    }
}
void Studio::findClick() {
    if (clickCheck.valid())
        return;
    clickChooser =
        std::make_unique<FileChooser>(cs::checkClick, projectDir.getChildFile("Exporty"), "*.wav");
    clickChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [self = Component::SafePointer<Studio>(this)](const FileChooser& chooser) {
                                  auto file = chooser.getResult();
                                  if (!self || !file.existsAsFile())
                                      return;
                                  auto grid = self->song.grid;
                                  auto start = (self->delivered.ok && file == self->delivered.file)
                                                   ? self->delivered.begin
                                                   : 0;
                                  self->clickCheck = std::async(std::launch::async, [file, grid, start] {
                                      return containsClick(file, grid, start);
                                  });
                                  self->status(cs::clickBusy);
                              });
}
} // namespace rocker
