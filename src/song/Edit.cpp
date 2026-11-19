#include "Edit.h"
namespace rocker {
namespace {
struct EditTracks {
    juce::StringArray members;
    juce::String selected;
    Frame at = -1, length = 0;
    EditTracks(const Song& song, const juce::String& track, const juce::String& id)
        : members(song.members(track)), selected(id) {
        for (auto& c : song.clips)
            if (c.id == id) {
                at = c.position;
                length = c.length;
                break;
            }
    }
    bool includes(const Clip& c) const {
        return members.contains(c.track) &&
               (selected.isEmpty() || selected == c.id ||
                (members.size() > 1 && c.position == at && c.length == length));
    }
    bool intersects(const Clip& c, Span s) const {
        return includes(c) && c.position < s.to && c.position + c.length > s.from;
    }
};
Clip slice(Clip c, Frame begin, Frame end, bool fresh) {
    c.offset += begin - c.position;
    c.position = begin;
    c.length = end - begin;
    if (fresh)
        c.id = juce::Uuid().toString();
    return c;
}
void extend(Song& s, Frame end) {
    s.bars = std::max(s.bars, int(s.grid.beatAt(end - 1) / s.grid.beats + 1));
}
bool fits(const Song& s, Frame pos, Frame length) {
    const auto limit =
        std::min(Frame(s.grid.rate) * 86400 * 30, s.grid.beat(Frame(1000000) * s.grid.beats));
    return pos >= 0 && length > 0 && pos <= limit - length;
}
} // namespace
Frame snapBeat(const BeatGrid& grid, Frame at) {
    auto beat = grid.beatAt(std::max<Frame>(0, at));
    auto left = grid.beat(beat), right = grid.beat(beat + 1);
    return at - left < right - at ? left : right;
}
Fragment copySpan(const Song& s, const juce::String& track, Span span, const juce::String& id) {
    Fragment part;
    if (!span.valid())
        return part;
    EditTracks group(s, track, id);
    part.sourceTracks = group.members;
    part.length = span.length();
    for (const auto& c : s.clips)
        if (group.intersects(c, span)) {
            auto piece = slice(c, std::max(span.from, c.position),
                               std::min(span.to, c.position + c.length), false);
            piece.position -= span.from;
            part.clips.push_back(std::move(piece));
        }
    return part;
}
bool eraseSpan(Song& s, const juce::String& track, Span span, const juce::String& id) {
    if (!span.valid())
        return false;
    EditTracks group(s, track, id);
    std::vector<Clip> result;
    bool changed = false;
    for (const auto& c : s.clips) {
        if (!group.intersects(c, span)) {
            result.push_back(c);
            continue;
        }
        changed = true;
        if (c.position < span.from)
            result.push_back(slice(c, c.position, span.from, false));
        if (c.position + c.length > span.to)
            result.push_back(slice(c, span.to, c.position + c.length, c.position < span.from));
    }
    if (changed)
        s.clips = std::move(result);
    return changed;
}
bool splitAt(Song& s, const juce::String& track, Frame at, const juce::String& id) {
    EditTracks group(s, track, id);
    std::vector<Clip> after;
    for (auto& c : s.clips)
        if (group.includes(c) && c.position < at && at < c.position + c.length) {
            after.push_back(slice(c, at, c.position + c.length, true));
            c.length = at - c.position;
        }
    s.clips.insert(s.clips.end(), after.begin(), after.end());
    return !after.empty();
}
bool pasteAt(Song& s, const juce::String& track, Frame at, const Fragment& part) {
    if (!fits(s, at, part.length) || part.clips.empty() ||
        std::none_of(s.tracks.begin(), s.tracks.end(), [&](auto& t) { return t.id == track; }))
        return false;
    auto destinations = s.members(track);
    if (part.sourceTracks.size() > 1 && part.sourceTracks.size() != destinations.size())
        return false;
    if (part.sourceTracks.size() == 1 && destinations.size() > 1)
        return false;
    for (auto& c : part.clips) {
        auto asset =
            std::find_if(s.assets.begin(), s.assets.end(), [&](auto& a) { return a.id == c.asset; });
        const auto target =
            part.sourceTracks.size() > 1 ? destinations[part.sourceTracks.indexOf(c.track)] : track;
        auto strip =
            std::find_if(s.tracks.begin(), s.tracks.end(), [&](auto& t) { return t.id == target; });
        if (strip == s.tracks.end() || asset == s.assets.end() ||
            (strip->kind == "midi") != asset->path.endsWithIgnoreCase(".mid") || c.offset < 0 ||
            c.length <= 0 || c.offset > asset->length - c.length || c.position < 0 ||
            c.position > part.length - c.length)
            return false;
    }
    eraseSpan(s, track, {at, at + part.length});
    for (auto c : part.clips) {
        c.id = juce::Uuid().toString();
        c.track = part.sourceTracks.size() > 1 ? destinations[part.sourceTracks.indexOf(c.track)] : track;
        c.position += at;
        s.clips.push_back(std::move(c));
    }
    extend(s, at + part.length);
    return true;
}
bool moveSpan(Song& s, const juce::String& track, Span span, Frame at, const juce::String& id) {
    if (at == span.from || !span.valid() || !fits(s, at, span.length()))
        return false;
    auto part = copySpan(s, track, span, id);
    if (part.clips.empty())
        return false;
    Song result = s;
    eraseSpan(result, track, span, id);
    if (!pasteAt(result, track, at, part))
        return false;
    s = std::move(result);
    return true;
}
std::optional<Span> trimEdge(Song& s, const juce::String& id, bool left, Frame target) {
    auto selected = std::find_if(s.clips.begin(), s.clips.end(), [&](auto& c) { return c.id == id; });
    if (selected == s.clips.end())
        return {};
    EditTracks group(s, selected->track, id);
    const Frame oldStart = selected->position, oldEnd = oldStart + selected->length;
    Frame low = left ? 0 : oldStart + 1, high = left ? oldEnd - 1 : std::numeric_limits<Frame>::max();
    std::vector<Clip*> clips;
    for (auto& c : s.clips)
        if (group.includes(c)) {
            auto asset =
                std::find_if(s.assets.begin(), s.assets.end(), [&](auto& a) { return a.id == c.asset; });
            if (asset == s.assets.end())
                return {};
            if (left)
                low = std::max(low, c.position - c.offset);
            else
                high = std::min(high, c.position + asset->length - c.offset);
            clips.push_back(&c);
        }
    auto edge = std::clamp(target, low, high);
    Span bounds{left ? edge : oldStart, left ? oldEnd : edge};
    if (edge == (left ? oldStart : oldEnd) || !fits(s, bounds.from, bounds.length()))
        return {};
    for (auto* c : clips) {
        c->offset += bounds.from - c->position;
        c->position = bounds.from;
        c->length = bounds.length();
    }
    extend(s, bounds.to);
    return bounds;
}
void EditHistory::reset(const Song& s) {
    states = {{s, {}}};
    index = 0;
}
bool EditHistory::remember(const Song& s, const juce::String& action) {
    if (matches(s))
        return false;
    states.resize(states.empty() ? 0 : index + 1);
    states.push_back({s, action});
    index = states.size() - 1;
    return true;
}
bool EditHistory::canStep(bool forward) const { return forward ? index + 1 < states.size() : index > 0; }
juce::String EditHistory::step(Song& song, bool forward) {
    if (!canStep(forward))
        return {};
    auto label = states[forward ? index + 1 : index].action;
    index = forward ? index + 1 : index - 1;
    song = states[index].song;
    return label;
}
} // namespace rocker
