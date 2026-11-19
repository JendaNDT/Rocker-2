#include "Studio.h"
#include "Strings.h"

namespace rocker {
using namespace juce;
namespace s = style;
namespace {
String micName(int index, int count) {
    if (count == 1)
        return cs::trackKinds[0];
    if (index == 0)
        return cs::kickName;
    if (index == 1)
        return cs::snareName;
    if (count >= 4 && index == count - 2)
        return cs::ohLeft;
    if (count >= 4 && index == count - 1)
        return cs::ohRight;
    return cs::tomName + " " + String(index - 1);
}
} // namespace
void Studio::defaultMonitor() {
    projectMonitor = 0;
    if (isPositiveAndBelow(view.selected, (int)song.tracks.size())) {
        auto& t = song.tracks[view.selected];
        if (t.kind == "bici" && t.inputs.size() == 2 && !laptop)
            projectMonitor = 1;
    }
}
void Studio::resetDrums() {
    drumDraft = {};
    hitPorts.clear();
    hitLevels.clear();
    drumIndex = 0;
    hitMeasured = false;
    message.clear();
    link.audio.beginHitTest();
}
void Studio::finishHits() {
    hitPorts.clear();
    hitLevels.clear();
    for (int i = 0; i < link.caps.inputs.size(); ++i) {
        const float peak = link.audio.hitPeak(i);
        if (decibels(peak) < -36.f)
            continue;
        hitPorts.add(link.caps.inputs[i]);
        hitLevels.push_back(peak);
    }
    drumDraft = {};
    drumDraft.ports = hitPorts;
    for (int i = 0; i < hitPorts.size(); ++i)
        drumDraft.names.add(micName(i, hitPorts.size()));
    drumDraft.stereo = hitPorts.size() == 2;
    drumIndex = 0;
    hitMeasured = true;
    triggerAsyncUpdate();
}
std::vector<Track> Studio::drumTracks() const {
    const auto it = settings.drumSetups.find(Settings::inputKey(link.current, 0));
    if (it == settings.drumSetups.end())
        return {};
    auto& kit = it->second;
    for (auto& port : kit.ports)
        if (!link.caps.inputs.contains(port))
            return {};
    std::vector<Track> tracks;
    if (kit.stereo) {
        Track t{Uuid().toString(), cs::trackKinds[0], "bici", kit.ports};
        tracks.push_back(t);
    } else {
        auto group = kit.ports.size() > 1 ? Uuid().toString() : String{};
        for (int i = 0; i < kit.ports.size(); ++i) {
            Track t{Uuid().toString(), kit.names[i], "bici", {kit.ports[i]}};
            t.group = group;
            if (kit.wide && kit.ports.size() >= 4) {
                if (i == kit.ports.size() - 2)
                    t.pan = -1;
                if (i == kit.ports.size() - 1)
                    t.pan = 1;
            }
            tracks.push_back(t);
        }
    }
    return tracks;
}
void Studio::drumPage(Rect area) {
    auto line = [&](int height = s::button) {
        auto r = area.removeFromTop(height);
        area.removeFromTop(s::gap);
        return r;
    };
    text(cs::drumTest, line(s::header), s::title);
    text(cs::hitPrompt, line());
    auto actions = line();
    button(
        hitMeasured ? cs::drumAgain : cs::finishHits, actions.removeFromLeft(s::nameWidth),
        [this] {
            if (hitMeasured) {
                resetDrums();
                triggerAsyncUpdate();
            } else
                finishHits();
        },
        1);
    if (hitMeasured && hitPorts.isEmpty())
        text(cs::noHits, line(), s::body, s::warning);
    if (!hitPorts.isEmpty()) {
        if (hitPorts.size() == 2)
            combo(cs::trackLabel, line().withWidth(s::fieldWidth), {cs::multiKit, cs::stereoKit},
                  drumDraft.stereo ? 1 : 0, [this](int i) {
                      drumDraft.stereo = i == 1;
                      triggerAsyncUpdate();
                  });
        else
            text(hitPorts.size() == 1 ? cs::oneMic : cs::multiKit, line(), s::heading);
        text(drumDraft.stereo ? cs::stereoAdvice : (hitPorts.size() == 1 ? cs::micAdvice : cs::kitHint),
             line());
        StringArray labels;
        for (int i = 0; i < drumDraft.ports.size(); ++i)
            labels.add(String(i + 1) + cs::divider +
                       (drumDraft.stereo ? (i ? cs::rightShort : cs::leftShort) : drumDraft.names[i]));
        auto routes = line();
        combo(cs::kitStrip, routes.removeFromLeft(s::fieldWidth), labels, drumIndex, [this](int i) {
            drumIndex = i;
            triggerAsyncUpdate();
        });
        routes.removeFromLeft(s::gutter);
        combo(cs::kitPort, routes.removeFromLeft(s::fieldWidth), hitPorts,
              hitPorts.indexOf(drumDraft.ports[drumIndex]), [this](int i) {
                  int other = drumDraft.ports.indexOf(hitPorts[i]);
                  std::swap(drumDraft.ports.getReference(drumIndex), drumDraft.ports.getReference(other));
                  triggerAsyncUpdate();
              });
        if (!drumDraft.stereo)
            field(cs::kitRole, line(), drumDraft.names.getReference(drumIndex));
        float db = decibels(hitLevels[(size_t)hitPorts.indexOf(drumDraft.ports[drumIndex])]);
        text(cs::signalPeak + "  " + String(db, 1).replaceCharacter('.', ',') + " dBFS   " +
                 (db >= -6 ? cs::hitHot : cs::hitGood),
             line(), s::body, db >= -6 ? s::warning : s::ready);
        if (hitPorts.size() >= 4)
            button(drumDraft.wide ? cs::kitWide : cs::kitCentre, line().withWidth(s::labelWidth), [this] {
                drumDraft.wide = !drumDraft.wide;
                triggerAsyncUpdate();
            });
        button(
            cs::kitApply, line().withWidth(s::labelWidth),
            [this] {
                if (!link.online)
                    return;
                for (int i = 0; i < drumDraft.names.size(); ++i)
                    if (drumDraft.names[i].trim().isEmpty())
                        drumDraft.names.set(i, micName(i, drumDraft.names.size()));
                settings.drumSetups[Settings::inputKey(link.current, 0)] = drumDraft;
                persist();
                show(Page::inputs);
            },
            1);
    }
    if (link.caps.inputs.size() <= 2)
        text(cs::portAdvice, line(), s::body, s::muted);
    button(cs::back, line().withWidth(s::clipWidth), [this] { show(Page::inputs); });
}
} // namespace rocker
