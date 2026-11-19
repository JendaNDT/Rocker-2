#include "Studio.h"
#include "Strings.h"

namespace rocker {
using namespace juce;
namespace s = style;
void Studio::refreshMidi() {
    auto previous = activeMidi;
    midiPorts = devices.midiDevices();
    auto found = std::find_if(midiPorts.begin(), midiPorts.end(),
                              [this](auto& p) { return p.identifier == settings.midiDevice; });
    String id = found != midiPorts.end() ? found->identifier
                                         : (midiPorts.isEmpty() ? String{} : midiPorts[0].identifier);
    activeMidi = devices.openMidi(id, link.audio) ? id : String{};
    if (previous.isNotEmpty() && previous != activeMidi && performance && performance->recording)
        stopProject();
    if (activeMidi.isEmpty()) {
        link.audio.drums.monitor = false;
        learningPad = -1;
        return;
    }
    settings.midiDevice = activeMidi;
    auto& setup = settings.midiSetups[activeMidi];
    for (int i = 0; i < 128; ++i) {
        auto it = setup.pads.find(i);
        link.audio.drums.mapping[i] = it == setup.pads.end() ? i : it->second;
    }
}
std::vector<Track> Studio::drumTracks() const {
    auto audio = audioDrumTracks();
    auto entry = settings.midiSetups.find(activeMidi);
    if (activeMidi.isEmpty() || entry == settings.midiSetups.end() || !entry->second.enabled)
        return audio;
    Track midi{Uuid().toString(), cs::midiDrums, "midi"};
    if (entry->second.coupled && audio.size() == 1 && audio[0].inputs.size() == 2) {
        midi.group = audio[0].group = Uuid().toString();
        midi.mute = true;
        audio.push_back(midi);
        return audio;
    }
    return {midi};
}
void Studio::pollMidi() {
    if (page != Page::midi || learningPad < 0 || activeMidi.isEmpty())
        return;
    int note = link.audio.drums.lastNote.exchange(-1);
    if (note < 0)
        return;
    const auto target = DrumPlayer::pads[(size_t)learningPad];
    if (learningMap.contains(note))
        return;
    learningMap[note] = target;
    link.audio.drums.mapping[note] = target;
    if (++learningPad == (int)DrumPlayer::pads.size()) {
        learningPad = -1;
        settings.midiSetups[activeMidi].pads = learningMap;
        refreshMidi();
        persist();
    }
    triggerAsyncUpdate();
}
void Studio::midiPage(Rect area) {
    auto line = [&](int height = s::button) {
        auto r = area.removeFromTop(height);
        area.removeFromTop(s::gap);
        return r;
    };
    text(cs::midiDrums, line(s::header), s::title);
    StringArray names;
    int index = 0;
    for (int i = 0; i < midiPorts.size(); ++i) {
        names.add(midiPorts[i].name);
        if (midiPorts[i].identifier == activeMidi)
            index = i;
    }
    if (names.isEmpty())
        text(cs::midiNoDevice, line(), s::body, s::warning);
    else
        combo(cs::midiModule, line(), names, index, [this](int i) {
            settings.midiDevice = midiPorts[i].identifier;
            learningPad = -1;
            refreshMidi();
            persist();
            triggerAsyncUpdate();
        });
    if (activeMidi.isNotEmpty()) {
        auto& setup = settings.midiSetups[activeMidi];
        StringArray modes{cs::midiDisabled, cs::midiOnly};
        auto audio = audioDrumTracks();
        if (audio.size() == 1 && audio[0].inputs.size() == 2)
            modes.add(cs::midiBoth);
        combo(cs::midiMode, line(), modes,
              !setup.enabled                       ? 0
              : setup.coupled && modes.size() == 3 ? 2
                                                   : 1,
              [this](int i) {
                  auto& config = settings.midiSetups[activeMidi];
                  config.enabled = i > 0;
                  config.coupled = i == 2;
                  persist();
                  triggerAsyncUpdate();
              });
        text(cs::midiReady, line());
        auto actions = line();
        button(
            cs::padLearn, actions.removeFromLeft(s::nameWidth),
            [this] {
                learningMap.clear();
                learningPad = 0;
                link.audio.drums.lastNote = -1;
                triggerAsyncUpdate();
            },
            1);
        actions.removeFromLeft(s::gap);
        button(cs::gmMap, actions.removeFromLeft(s::nameWidth), [this] {
            learningPad = -1;
            settings.midiSetups[activeMidi].pads.clear();
            refreshMidi();
            persist();
            triggerAsyncUpdate();
        });
        if (learningPad >= 0) {
            const String names[]{cs::kickName, cs::snareName, cs::closedHat, cs::openHat,
                                 cs::lowTom,   cs::highTom,   cs::crash,     cs::ride};
            text(String(learningPad + 1) + " / 8" + cs::divider + cs::padPrompt + cs::divider +
                     names[learningPad],
                 line(s::row), s::counter, s::drums);
        }
    }
    button(cs::back, line().withWidth(s::clipWidth), [this] { show(Page::inputs); });
}
void Studio::compareMidi() {
    const Track* midi = nullptr;
    const Track* audio = nullptr;
    for (auto& t : song.tracks)
        if (t.kind == "midi" && t.group.isNotEmpty()) {
            for (auto& a : song.tracks)
                if (a.group == t.group && a.kind == "bici") {
                    midi = &t;
                    audio = &a;
                    break;
                }
            if (audio)
                break;
        }
    if (!midi || !audio) {
        status(cs::compareHelp);
        return;
    }
    auto asset = [this](const Clip& clip) {
        for (auto& a : song.assets)
            if (a.id == clip.asset)
                return projectDir.getChildFile(a.path);
        return File{};
    };
    for (auto i = song.clips.rbegin(); i != song.clips.rend(); ++i)
        if (i->track == midi->id) {
            for (auto j = song.clips.rbegin(); j != song.clips.rend(); ++j)
                if (j->track == audio->id && j->position == i->position) {
                    auto hits = readMidi(asset(*i), song.grid.rate);
                    auto reader = waveReader(asset(*j));
                    if (!reader || !hits)
                        continue;
                    const auto shift = compareSnare(*hits, *reader, i->offset, j->offset);
                    if (!shift)
                        continue;
                    auto& correction =
                        settings.midiCorrections[Settings::calibrationKey(link.current) + activeMidi];
                    // Replace the correction associated with this recording, so measuring twice is
                    // idempotent.
                    correction = int(i->offset) - link.caps.outputLatency + *shift;
                    persist();
                    status(cs::midiOffset + " " + String(*shift * 1000. / song.grid.rate, 2) + " ms");
                    return;
                }
        }
    status(cs::compareHelp);
}
} // namespace rocker
