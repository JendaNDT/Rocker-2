#include "Studio.h"
#include "Strings.h"

namespace rocker {
using namespace juce;
namespace s = style;

bool Studio::saveView() {
    view.trackId = selectedTrack();
    return view.save(projectDir);
}

String Studio::endpoint(const DeviceChoice& device) {
    return String(int(device.driver)) + "/" + device.input;
}

void Studio::bindInput(const Track& track) {
    if (link.online && !track.inputs.isEmpty())
        view.inputDevices[track.id] = endpoint(link.current);
}

void Studio::initializeInputs() {
    const auto preferred = settings.device.input.isEmpty() ? link.current : settings.device;
    for (const auto& t : song.tracks)
        if (!t.inputs.isEmpty() && !view.inputDevices.contains(t.id))
            view.inputDevices[t.id] = preferred.input.isEmpty() ? "?" : endpoint(preferred);
}

String Studio::missingPorts(const Track& track) const {
    if (track.inputs.isEmpty() || track.kind == "midi")
        return {};
    const auto binding = view.inputDevices.find(track.id);
    const bool deviceLost = !link.online || (binding != view.inputDevices.end() &&
                                             binding->second != endpoint(link.current));
    StringArray lost;
    for (const auto& port : track.inputs)
        if (deviceLost || !link.caps.inputs.contains(port)) {
            auto name = port;
            if (name.startsWith("In ") && name.substring(3).containsOnly("0123456789"))
                name = cs::input + " " + name.substring(3);
            lost.add(name + " " + cs::inputLost);
        }
    return lost.joinIntoString(", ");
}

void Studio::monitorHelp(Rect& area) {
    auto box = area.removeFromTop(s::statusStrip).reduced(s::gap, 0);
    auto actions = box.removeFromRight(s::labelWidth);
    button(projectMonitor ? cs::monitorDisable : cs::monitorRestore,
           actions.removeFromTop(s::button), [this] {
               if (projectMonitor) {
                   previousMonitor = projectMonitor;
                   projectMonitor = 0;
               } else
                   projectMonitor = previousMonitor;
               configureSongMonitor();
               triggerAsyncUpdate();
           });
    button(cs::closeHelp, actions, [this] { monitorHelpOpen = false; triggerAsyncUpdate(); });
    text(cs::monitorHelpInput, box.removeFromTop(s::button), s::body);
    text(projectMonitor ? cs::monitorHelpApp : cs::monitorHelpHardware, box, s::body);
}

void Studio::rememberTake(const Performance& p, size_t firstAsset) {
    SavedTake take;
    take.start = p.start;
    take.track = selectedTrack();
    take.family = p.retakeFamily.isEmpty() ? Uuid().toString() : p.retakeFamily;
    take.loop = p.loopSpan().has_value();
    if (p.isPunch())
        take.selection = {p.punchFrom, p.punchTo};
    take.audio.from = std::numeric_limits<Frame>::max();
    for (size_t i = firstAsset; i < song.assets.size(); ++i)
        take.assets.add(song.assets[i].id);
    auto include = [&](const Clip& clip, Frame from, Frame to) {
        from = std::max(from, take.selection.valid() ? take.selection.from : p.start);
        to = std::min(to, take.selection.valid() ? take.selection.to : p.cursor.load());
        if (!take.assets.contains(clip.asset) || to <= from)
            return;
        take.tracks.addIfNotAlreadyThere(clip.track);
        take.audio.from = std::min(take.audio.from, from);
        take.audio.to = std::max(take.audio.to, to);
    };
    for (const auto& clip : song.clips)
        include(clip, std::max(p.start, clip.position), clip.position + clip.length);
    for (const auto& group : song.groups) {
        if (!group.active || (take.selection.valid() &&
            (group.to <= take.selection.from || group.from >= take.selection.to)))
            continue;
        const auto& active = group.attempts[size_t(group.active - 1)];
        for (const auto& clip : active.clips) {
            if (!take.assets.contains(clip.asset))
                continue;
            take.attempts[group.id] = active.id;
            take.number = std::max(take.number, group.active + (take.loop ? 0 : 1));
            include(clip, std::max(group.from, clip.position),
                    std::min(group.to, clip.position + clip.length));
        }
    }
    if (!take.audio.valid())
        return;
    take.previousEnd = std::max(take.audio.to, p.previousTakeEnd);
    StringArray names;
    for (const auto& t : song.tracks)
        if (take.tracks.contains(t.id))
            names.add(t.name);
    status(names.joinIntoString(", ") + cs::divider + cs::attemptLabel + " " + String(take.number) +
           cs::divider + cs::savedTakeLabel);
    if (p.retakeFamily.isNotEmpty())
        pick({take.start, take.previousEnd});
    savedTake = std::move(take);
}

bool Studio::savedTakeAvailable() const {
    if (!savedTake)
        return false;
    const auto& take = *savedTake;
    for (const auto& [groupId, attemptId] : take.attempts) {
        auto group = std::find_if(song.groups.begin(), song.groups.end(),
                                  [&](const auto& g) { return g.id == groupId; });
        if (group == song.groups.end() || std::none_of(group->attempts.begin(), group->attempts.end(),
            [&](const auto& attempt) { return attempt.id == attemptId && !attempt.discarded; }))
            return false;
    }
    if (take.attempts.empty())
        for (const auto& asset : take.assets)
            if (std::none_of(song.clips.begin(), song.clips.end(), [&](const Clip& clip) {
                return clip.asset == asset && clip.position < take.audio.to &&
                       clip.position + clip.length > take.audio.from;
            }))
                return false;
    return true;
}

void Studio::takeActions(Rect& area) {
    if ((performance && !comparing) || !savedTakeAvailable())
        return;
    auto place = [&] {
        auto buttonArea = area.removeFromRight(s::nameWidth);
        area.removeFromRight(s::grid);
        return buttonArea;
    };
    auto activate = [this](TakeAction action) {
        if (comparing)
            stopProject();
        startProject(action == TakeAction::repeat, false, nullptr, false, action);
    };
    button(cs::repeatTake, place(), [activate] { activate(TakeAction::repeat); });
    button(cs::listenTake, place(), [activate] { activate(TakeAction::listen); }, primary);
}
} // namespace rocker
