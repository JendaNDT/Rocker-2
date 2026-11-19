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
    exporting = panel;
    view.mixing = true;
    view.editing = false;
    selectedOccurrence = -1;
    browsingVersions = soundDeckOpen = namingPreset = false;
    drag = Drag::none;
    if (!selection().valid())
        selectedRange = false;
    view.save(projectDir);
    triggerAsyncUpdate();
}
void Studio::seekMix(Frame frame) {
    if (performance)
        stopProject();
    view.cursor = std::clamp<Frame>(frame, 0, song.end());
    view.save(projectDir);
    triggerAsyncUpdate();
}
void Studio::toggleMix(int i, bool solo) {
    if (!isPositiveAndBelow(i, (int)song.tracks.size()))
        return;
    auto& t = song.tracks[i];
    auto& flag = solo ? t.solo : t.mute;
    flag = !flag;
    view.selected = i;
    if (performance)
        performance->playback.updateMix(song);
    saveSong();
    view.save(projectDir);
    triggerAsyncUpdate();
}
void Studio::mixPage(Rect area) {
    auto foot = area.removeFromBottom(s::songDeck);
    const int width = s::clipWidth + s::nameWidth + s::trackHead + 2 * s::faderReadout;
    auto axis = [&](Rect r) { return r.withTrimmedLeft(width - s::trackHead); };
    auto header = line(area);
    sectionStrip = axis(header);
    text(cs::trackLabel, header.removeFromLeft(s::clipWidth), s::caption, s::muted);
    text(cs::gainLabel, header.removeFromLeft(s::nameWidth), s::caption, s::muted);
    text(cs::panLabel, header.removeFromLeft(s::trackHead), s::caption, s::muted);
    ruler = axis(line(area, s::icon));
    lanes.clear();
    trackMeters.clear();
    const int visible = std::max(1, area.getHeight() / s::songLane);
    firstTrack = std::clamp(firstTrack, 0, std::max(0, (int)song.tracks.size() - visible));
    for (int i = firstTrack; i < std::min((int)song.tracks.size(), firstTrack + visible); ++i) {
        const auto& t = song.tracks[i];
        auto lane = area.removeFromTop(s::songLane);
        lanes.push_back(axis(lane));
        auto row = lane.removeFromTop(s::button);
        auto nameArea = row.removeFromLeft(s::clipWidth).reduced(s::grid, 0);
        auto label = button(
            limit(skin.font(s::body, false, true), t.name, nameArea.getWidth() - 2 * s::inset), nameArea,
            [this, i] {
                view.selected = i;
                view.save(projectDir);
                triggerAsyncUpdate();
            },
            view.selected == i ? 1 : 0);
        label->setTooltip(t.name);
        mixSlider(cs::gainLabel + " " + t.name, row.removeFromLeft(s::nameWidth), t.gainDb, false,
                  [this, i](float v) { song.tracks[i].gainDb = v; });
        mixSlider(cs::panLabel + " " + t.name, row.removeFromLeft(s::trackHead), t.pan, true,
                  [this, i](float v) { song.tracks[i].pan = v; });
        button(
            "S", row.removeFromLeft(s::faderReadout).reduced(s::grid, 0),
            [this, i] { toggleMix(i, true); }, t.solo ? 1 : 0)
            ->setTooltip(cs::solo);
        button(
            "M", row.removeFromLeft(s::faderReadout).reduced(s::grid, 0),
            [this, i] { toggleMix(i, false); }, t.mute ? 1 : 0)
            ->setTooltip(cs::mute);
        lane = lane.withHeight(s::button);
        if (t.kind == "kytara")
            text(limit(skin.font(s::caption), t.amp.name, s::clipWidth - 2 * s::grid),
                 lane.removeFromLeft(s::clipWidth).reduced(s::grid, 0), s::caption, s::guitar);
        else
            lane.removeFromLeft(s::clipWidth);
        trackMeters.push_back(lane.removeFromLeft(s::nameWidth));
        if (t.inputOver >= 0)
            button(cs::inputOver + cs::divider + bar(song, t.inputOver), lane.removeFromLeft(s::labelWidth),
                   [this, frame = t.inputOver] { seekMix(frame); });
    }
    timelineArea = ruler.withBottom(lanes.empty() ? ruler.getBottom() : lanes.back().getBottom());
    auto total = line(foot, s::transport);
    text(cs::master, total.removeFromLeft(s::clipWidth), s::heading, s::chalk, false, true);
    mixSlider(cs::master, total.removeFromLeft(s::nameWidth), song.masterDb, false,
              [this](float v) { song.masterDb = v; });
    stereoMeter = total.removeFromLeft(total.getWidth() / 2).reduced(s::gap, 0);
    mixReadout = total;
    auto actions = line(foot);
    button(cs::sendMix, actions.removeFromLeft(s::clipWidth), [this] { mixMode(true); }, 1);
    actions.removeFromLeft(s::gap);
    if (isPositiveAndBelow(view.selected, (int)song.tracks.size()) &&
        song.tracks[view.selected].group.isNotEmpty()) {
        auto& t = song.tracks[view.selected];
        text(cs::kitGain, actions.removeFromLeft(s::compactButton));
        mixSlider(cs::kitGain, actions.removeFromLeft(s::nameWidth), t.groupDb, false,
                  [this, id = t.group](float gain) {
                      for (auto& member : song.tracks)
                          if (member.group == id)
                              member.groupDb = gain;
                  });
    }
    auto meter = performance ? performance->playback.meter() : lastMeter;
    if (meter.over >= 0)
        button(cs::mixOver + cs::divider + bar(song, meter.over), actions.removeFromLeft(s::labelWidth),
               [this, frame = meter.over] { seekMix(frame); });
    else
        text(cs::noOver, actions.removeFromLeft(s::clipWidth), s::body, s::ready);
    if (firstTrack > 0)
        button(cs::prevTracks, actions.removeFromLeft(s::clipWidth), [this, visible] {
            firstTrack -= visible;
            triggerAsyncUpdate();
        });
    if (firstTrack + visible < (int)song.tracks.size())
        button(cs::nextTracks, actions.removeFromLeft(s::clipWidth), [this, visible] {
            firstTrack += visible;
            triggerAsyncUpdate();
        });
    auto notice = line(foot);
    if (lastLoudness)
        text(cs::lastDelivery + loudness(*lastLoudness), notice.removeFromRight(s::fieldWidth),
             s::caption, s::muted);
    text(message, notice, s::body, s::warning);
}
void Studio::paintMix(Graphics& g) {
    const auto m = performance ? performance->playback.meter() : lastMeter;
    auto draw = [&](Rect r, float level, const String& name) {
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
