#include "Edit.h"
namespace rocker {
namespace {
bool intersects(const Clip& c, const juce::String& track, Span s, const juce::String& id) {
    return c.track == track && (id.isEmpty() || c.id == id) && c.position < s.to &&
           c.position + c.length > s.from;
}
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
    part.length = span.length();
    for (const auto& c : s.clips)
        if (intersects(c, track, span, id)) {
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
    std::vector<Clip> result;
    bool changed = false;
    for (const auto& c : s.clips) {
        if (!intersects(c, track, span, id)) {
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
    std::vector<Clip> after;
    for (auto& c : s.clips)
        if (c.track == track && (id.isEmpty() || id == c.id) && c.position < at &&
            at < c.position + c.length) {
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
    for (auto& c : part.clips) {
        auto asset =
            std::find_if(s.assets.begin(), s.assets.end(), [&](auto& a) { return a.id == c.asset; });
        if (asset == s.assets.end() || c.offset < 0 || c.length <= 0 ||
            c.offset > asset->length - c.length || c.position < 0 || c.position > part.length - c.length)
            return false;
    }
    eraseSpan(s, track, {at, at + part.length});
    for (auto c : part.clips) {
        c.id = juce::Uuid().toString();
        c.track = track;
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
    for (auto& c : s.clips)
        if (c.id == id) {
            const Frame end = c.position + c.length;
            auto a = std::find_if(s.assets.begin(), s.assets.end(),
                                  [&](auto& asset) { return asset.id == c.asset; });
            if (a == s.assets.end())
                return {};
            if (left) {
                auto pos = std::clamp(target, std::max<Frame>(0, c.position - c.offset), end - 1);
                if (pos == c.position)
                    return {};
                c = slice(c, pos, end, false);
            } else {
                auto length = std::clamp(target - c.position, Frame(1), a->length - c.offset);
                if (length == c.length || !fits(s, c.position, length))
                    return {};
                c.length = length;
            }
            extend(s, c.position + c.length);
            return Span{c.position, c.position + c.length};
        }
    return {};
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
