#include "Song.h"
#include "SectionLabels.h"
#include <algorithm>
#include <set>

namespace rocker {
using namespace juce;
namespace {
String fresh() {
    return Uuid().toString();
}
Clip portion(Clip c, Frame first, Frame last, Frame shift) {
    c.offset += first - c.position;
    c.length = last - first;
    c.position = first + shift;
    return c;
}
void shiftGroup(TakeGroup& group, Frame delta) {
    group.from += delta;
    group.to += delta;
    for (auto& attempt : group.attempts)
        for (auto& pass : attempt.clips)
            pass.position += delta;
}
Section sliceSection(const Section& source, Frame first, Frame last, int bars) {
    Section result{fresh(), source.name, source.colour, bars};
    for (const auto& c : source.clips) {
        Frame a = std::max(first, c.position), b = std::min(last, c.position + c.length);
        if (b > a)
            result.clips.push_back(portion(c, a, b, -first));
    }
    for (auto g : source.groups) {
        g.from = std::max(first, g.from);
        g.to = std::min(last, g.to);
        if (g.to <= g.from)
            continue;
        shiftGroup(g, -first);
        result.groups.push_back(std::move(g));
    }
    return result;
}
} // namespace
Frame Song::translate(Frame value, int fromBar, int toBar) const {
    auto beat = grid.beatAt(value);
    if (value == grid.beat(beat))
        return grid.beat(beat + Frame(toBar - fromBar) * grid.beats);
    return value - barFrame(fromBar) + barFrame(toBar);
}
int Song::startBar(int index) const {
    int bar = 0;
    for (int i = 0; i < index && i < (int)order.size(); ++i)
        bar += order[i].length;
    return bar;
}
int Song::occurrenceAt(int bar) const {
    for (int i = 0; i < (int)order.size() && bar >= 0; ++i) {
        if (bar < order[i].length)
            return i;
        bar -= order[i].length;
    }
    return -1;
}
bool Song::linked(int index) const {
    return isPositiveAndBelow(index, (int)order.size()) &&
           std::count_if(order.begin(), order.end(),
                         [&](const auto& o) { return o.section == order[index].section; }) > 1;
}
void Song::initOrder() {
    if (!order.empty())
        return;
    bars = std::max(bars, (int)(grid.beatAt(std::max<Frame>(0, end() - 1)) / grid.beats + 1));
    Section section{"s1", cs::verseName, "sloka", bars, clips, groups};
    sections.emplace(section.id, section);
    order.push_back({"i1", section.id, 0, bars});
    projectOrder();
}
void Song::projectOrder() {
    if (order.empty())
        return;
    clips.clear();
    groups.clear();
    int bar = 0;
    for (const auto& o : order) {
        const auto& section = sections.at(o.section);
        int start = bar;
        Frame at = barFrame(start), first = barFrame(o.crop);
        bar += o.length;
        Frame last = first + barFrame(bar) - at;
        for (const auto& c : section.clips) {
            Frame a = std::max(first, c.position), b = std::min(last, c.position + c.length);
            if (b <= a)
                continue;
            auto item = portion(c, a, b, at - first);
            item.id = o.id + "/" + c.id;
            clips.push_back(std::move(item));
        }
        for (auto g : section.groups) {
            auto begin = std::max(at, translate(g.from, o.crop, start));
            auto end = std::min(barFrame(bar), translate(g.to, o.crop, start));
            if (end <= begin)
                continue;
            g.id = o.id + "/" + g.id;
            shiftGroup(g, at - first);
            g.from = begin;
            g.to = end;
            groups.push_back(std::move(g));
        }
    }
    bars = bar;
}
bool Song::splitSection(int bar) {
    if (bar <= 0 || bar >= bars)
        return false;
    initOrder();
    int index = occurrenceAt(bar);
    if (index < 0)
        return false;
    const auto o = order[index];
    int left = bar - startBar(index);
    if (!left)
        return false;
    int cut = o.crop + left;
    const auto& source = sections.at(o.section);
    Frame point = barFrame(o.crop) + barFrame(bar) - barFrame(startBar(index));
    auto a = sliceSection(source, 0, point, cut);
    auto b = sliceSection(source, point, std::numeric_limits<Frame>::max(), source.bars - cut);
    b.name = cs::chorusName;
    b.colour = "refren";
    order[index] = {o.id, a.id, o.crop, left};
    order.insert(order.begin() + index + 1, {fresh(), b.id, 0, o.length - left});
    sections.emplace(a.id, a);
    sections.emplace(b.id, b);
    if (std::none_of(order.begin(), order.end(), [&](auto& v) { return v.section == o.section; }))
        sections.erase(o.section);
    projectOrder();
    return true;
}
bool Song::repeatSection(int index, int insert) {
    initOrder();
    if (!isPositiveAndBelow(index, (int)order.size()) || insert < 0 || insert > (int)order.size() ||
        bars > 1000000 - order[index].length)
        return false;
    auto copy = order[index];
    copy.id = fresh();
    order.insert(order.begin() + insert, copy);
    projectOrder();
    return true;
}
bool Song::detachSection(int index) {
    if (!linked(index))
        return false;
    auto copy = sections.at(order[index].section);
    copy.id = fresh();
    copy.name += " (2)";
    order[index].section = copy.id;
    sections.emplace(copy.id, copy);
    projectOrder();
    return true;
}
bool Song::resizeOccurrence(int index, int length) {
    if (!isPositiveAndBelow(index, (int)order.size()))
        return false;
    auto& o = order[index];
    if (length < 1 || length > sections.at(o.section).bars - o.crop ||
        bars - o.length > 1000000 - length || length == o.length)
        return false;
    o.length = length;
    projectOrder();
    return true;
}
void Song::extendOrder(Frame to) {
    if (order.empty())
        return;
    int current = startBar((int)order.size());
    int extra = int(grid.beatAt(std::max<Frame>(0, to - 1)) / grid.beats + 1) - current;
    if (extra <= 0)
        return;
    auto& last = order.back();
    auto& section = sections.at(last.section);
    if (!linked(int(order.size() - 1)) && last.crop + last.length == section.bars) {
        section.bars += extra;
        last.length += extra;
    } else {
        Section tail{fresh(), cs::endingName, "zaver", extra};
        sections.emplace(tail.id, tail);
        order.push_back({fresh(), tail.id, 0, extra});
    }
    bars = current + extra;
}
// Clip tools edit the audible timeline. Translate only their changed spans back
// to local source material, leaving cropped-away audio and other tracks intact.
void Song::commitClips() {
    if (order.empty())
        return;
    auto edited = std::move(clips);
    projectOrder();
    std::map<String, Clip> original, changed;
    for (const auto& c : clips)
        original.emplace(c.id, c);
    for (const auto& c : edited)
        changed.emplace(c.id, c);
    using Key = std::pair<String, String>;
    std::map<Key, std::vector<std::pair<Frame, Frame>>> removals;
    for (const auto& [id, c] : original) {
        auto it = changed.find(id);
        if (it != changed.end() && it->second == c)
            continue;
        int index = occurrenceAt((int)(grid.beatAt(c.position) / grid.beats));
        if (index < 0)
            continue;
        const auto& o = order[index];
        Frame a = c.position - barFrame(startBar(index)) + barFrame(o.crop);
        removals[{o.section, id.fromFirstOccurrenceOf("/", false, false)}].emplace_back(a, a + c.length);
    }
    for (auto& [key, ranges] : removals) {
        auto& local = sections.at(key.first).clips;
        auto it = std::find_if(local.begin(), local.end(),
                               [&](const auto& part) { return part.id == key.second; });
        if (it == local.end())
            continue;
        Clip source = *it;
        local.erase(it);
        std::sort(ranges.begin(), ranges.end());
        Frame cursor = source.position, end = cursor + source.length;
        auto preserve = [&](Frame a, Frame b) {
            if (b <= a)
                return;
            auto hidden = portion(source, a, b, 0);
            hidden.id = fresh();
            local.push_back(std::move(hidden));
        };
        for (auto [a, b] : ranges) {
            preserve(cursor, std::min(a, end));
            cursor = std::max(cursor, b);
        }
        preserve(cursor, end);
    }
    for (const auto& c : edited) {
        auto it = original.find(c.id);
        if (it != original.end() && it->second == c)
            continue;
        extendOrder(c.position + c.length);
        int bar = 0;
        for (const auto& o : order) {
            Frame at = barFrame(bar);
            bar += o.length;
            Frame a = std::max(at, c.position), b = std::min(barFrame(bar), c.position + c.length);
            if (b <= a)
                continue;
            auto item = portion(c, a, b, barFrame(o.crop) - at);
            auto& local = sections.at(o.section).clips;
            if (c.id.startsWith(o.id + "/"))
                item.id = c.id.substring(o.id.length() + 1);
            else
                item.id = c.id.containsChar('/') ? fresh() : c.id;
            if (std::find(local.begin(), local.end(), item) != local.end())
                continue;
            if (std::any_of(local.begin(), local.end(), [&](auto& old) { return old.id == item.id; }))
                item.id = fresh();
            local.push_back(std::move(item));
        }
    }
    projectOrder();
}
void Song::commitGroup(const TakeGroup& edited, Frame previousFrom, Frame previousTo) {
    if (previousFrom < 0)
        previousFrom = edited.from;
    if (previousTo < 0)
        previousTo = edited.to;
    std::vector<TakeGroup> changes;
    for (auto& peer : groups) {
        if (peer.from != previousFrom || peer.to != previousTo ||
            !linkedTrack(peer.track, edited.track) || peer.attempts.size() != edited.attempts.size())
            continue;
        auto copy = peer;
        copy.active = edited.active;
        copy.from = edited.from;
        copy.to = edited.to;
        for (size_t i = 0; i < copy.attempts.size(); ++i)
            copy.attempts[i].discarded = edited.attempts[i].discarded;
        if (copy.id != edited.id)
            changes.push_back(std::move(copy));
    }
    changes.push_back(edited);
    for (auto& change : changes)
        commitSingleGroup(change);
}
void Song::commitSingleGroup(const TakeGroup& edited) {
    for (auto& peer : groups)
        if (peer.id == edited.id) {
            peer = edited;
            break;
        }
    for (int i = 0; i < (int)order.size(); ++i) {
        const auto& o = order[i];
        for (auto& g : sections.at(o.section).groups) {
            if (edited.id != o.id + "/" + g.id)
                continue;
            auto update = edited;
            update.id = g.id;
            Frame local = barFrame(o.crop), at = barFrame(startBar(i));
            shiftGroup(update, local - at);
            update.from = translate(edited.from, startBar(i), o.crop);
            update.to = translate(edited.to, startBar(i), o.crop);
            if (update.from == local)
                update.from = std::min(g.from, local);
            Frame end = barFrame(o.crop + o.length);
            if (update.to == end)
                update.to = std::max(g.to, end);
            g = std::move(update);
            projectOrder();
            return;
        }
    }
}
} // namespace rocker
