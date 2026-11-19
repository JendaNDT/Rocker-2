#include "Studio.h"
#include "Strings.h"
#include <cmath>

namespace rocker {
using namespace juce;
namespace s = style;
String Studio::selectedTrack() const {
    return isPositiveAndBelow(view.selected, (int)song.tracks.size()) ? song.tracks[view.selected].id
                                                                      : String{};
}
int Studio::visibleBars() const {
    return view.editing
               ? jlimit(1, view.zoom, std::max(1, (ruler.getWidth() - s::trackHead) / s::target))
               : view.zoom;
}
Frame Studio::firstBar() const {
    if (view.editing)
        return view.firstBar;
    auto bar = song.grid.beatAt(std::max<Frame>(0, playPosition())) / song.grid.beats;
    return bar / view.zoom * view.zoom;
}
Frame Studio::frameAt(int x) const {
    auto axis = ruler.withTrimmedLeft(s::trackHead);
    auto begin = song.grid.beat(firstBar() * song.grid.beats),
         end = song.grid.beat((firstBar() + visibleBars()) * song.grid.beats);
    return begin + std::llround(double(jlimit(0, axis.getWidth(), x - axis.getX())) * (end - begin) /
                                std::max(1, axis.getWidth()));
}
int Studio::pixelAt(Frame pos) const {
    auto axis = ruler.withTrimmedLeft(s::trackHead);
    auto from = frameAt(axis.getX()), to = frameAt(axis.getRight());
    return axis.getX() +
           roundToInt(std::clamp(double(pos - from) / std::max<Frame>(1, to - from), -2., 3.) *
                      axis.getWidth());
}
void Studio::pick(Span span, String id) {
    preview = -1;
    previewGroup.clear();
    view.selectionStart = span.from;
    view.selectionEnd = span.to;
    view.selectedClip = std::move(id);
}
void Studio::normalizeSelection() {
    view.selected = jlimit(0, std::max(0, (int)song.tracks.size() - 1), view.selected);
    if (!view.selectedClip.isEmpty()) {
        for (const auto& c : song.clips)
            if (c.id == view.selectedClip && c.track == selectedTrack()) {
                pick({c.position, c.position + c.length}, c.id);
                return;
            }
        pick({});
    }
    if (!selection().valid() || view.selectionEnd > song.grid.beat(Frame(1000000) * song.grid.beats))
        pick({});
    anchorBar = song.grid.beatAt(view.selectionStart) / song.grid.beats;
}
void Studio::reveal(Frame pos) {
    auto bar = song.grid.beatAt(std::max<Frame>(0, pos)) / song.grid.beats;
    if (bar < view.firstBar || bar >= view.firstBar + visibleBars())
        view.firstBar = bar / visibleBars() * visibleBars();
}
void Studio::editMode(bool edit) {
    if (view.editing == edit && !browsingVersions)
        return;
    view.editing = edit;
    browsingVersions = soundDeckOpen = namingPreset = false;
    drag = Drag::none;
    if (edit)
        reveal(view.cursor);
    view.save(projectDir);
    triggerAsyncUpdate();
}
void Studio::selectBar(Frame bar, bool extend) {
    if (!extend)
        anchorBar = bar;
    auto first = std::min(anchorBar, bar), last = std::max(anchorBar, bar) + 1;
    pick({song.grid.beat(first * song.grid.beats), song.grid.beat(last * song.grid.beats)});
    view.cursor = view.selectionStart;
}
void Studio::editOp(EditOp op) {
    if (performance || selectedTrack().isEmpty())
        return;
    if (pendingSoundSave)
        history.remember(song, cs::edited);
    const auto track = selectedTrack();
    const auto span = selection();
    auto id = view.selectedClip;
    bool changed = false;
    String label;
    switch (op) {
    case EditOp::copy: {
        auto part = copySpan(song, track, span, id);
        if (part.clips.empty()) {
            status(cs::noAudio);
            return;
        }
        clipboard = std::move(part);
        status(cs::copied);
        return;
    }
    case EditOp::paste:
    case EditOp::repeat: {
        auto part = op == EditOp::paste ? clipboard : copySpan(song, track, span, id);
        Frame pos = op == EditOp::paste ? view.cursor : span.to;
        label = op == EditOp::paste ? cs::paste : cs::repeat;
        changed = pasteAt(song, track, pos, part);
        if (changed) {
            pick({pos, pos + part.length});
            view.cursor = pos;
        }
        break;
    }
    case EditOp::erase:
        label = cs::erase;
        changed = eraseSpan(song, track, span, id);
        if (changed)
            pick({});
        break;
    case EditOp::split:
        label = cs::split;
        changed = splitAt(song, track, view.cursor, id);
        if (changed)
            pick({});
        break;
    case EditOp::move:
        label = cs::move;
        changed = moveSpan(song, track, span, dragTarget, id);
        if (changed) {
            pick({dragTarget, dragTarget + span.length()});
            view.cursor = dragTarget;
        }
        break;
    case EditOp::trimLeft:
    case EditOp::trimRight:
        label = cs::trim;
        if (auto part = trimEdge(song, id, op == EditOp::trimLeft, dragTarget)) {
            changed = true;
            pick(*part, id);
            view.cursor = op == EditOp::trimLeft ? part->from : part->to;
        }
        break;
    }
    if (!changed) {
        status(cs::noAudio);
        return;
    }
    history.remember(song, label);
    pendingSoundSave = Time::getMillisecondCounterHiRes() + 1200;
    reveal(view.cursor);
    view.save(projectDir);
    status(label);
}
void Studio::undo(bool forward) {
    if (performance)
        return;
    if (pendingSoundSave)
        saveSong();
    auto action = history.step(song, forward);
    if (action.isEmpty())
        return;
    preview = -1;
    previewGroup.clear();
    normalizeSelection();
    view.cursor = std::min(view.cursor, song.end());
    soundDeckOpen = namingPreset = false;
    projectMonitor = 0;
    configureSongMonitor();
    const bool complete = library.load(song, projectDir);
    view.save(projectDir);
    status(complete ? (forward ? cs::redo : cs::undo) + " · " + action : cs::partialProject);
    saveSong();
}
void Studio::showVersions() {
    if (performance)
        return;
    versions = projectDir.getChildFile("Verze").findChildFiles(File::findFiles, false, "*.json");
    versions.sort();
    std::reverse(versions.begin(), versions.end());
    versionIndex = 0;
    browsingVersions = true;
    soundDeckOpen = namingPreset = false;
    triggerAsyncUpdate();
}
void Studio::restoreVersion() {
    if (!isPositiveAndBelow(versionIndex, versions.size()) || performance)
        return;
    bool bad = false;
    auto restored = Song::parse(JSON::parse(versions[versionIndex]), bad);
    AudioLibrary checked;
    if (!restored || bad || !checked.load(*restored, projectDir)) {
        status(cs::versionError);
        return;
    }
    if (pendingSoundSave)
        saveSong();
    if (!song.archive(projectDir) || !restored->save(projectDir)) {
        status(cs::writeError);
        return;
    }
    song = std::move(*restored);
    library = std::move(checked);
    history.remember(song, cs::versionRestored);
    normalizeSelection();
    view.cursor = std::min(view.cursor, song.end());
    reveal(view.cursor);
    view.save(projectDir);
    projectMonitor = 0;
    configureSongMonitor();
    browsingVersions = false;
    status(cs::versionRestored);
}
void Studio::versionsDeck(Rect area) {
    auto top = area.removeFromTop(s::button);
    text(cs::versions, top, s::heading);
    area.removeFromTop(s::gap);
    auto row = area.removeFromTop(s::button);
    button(cs::back, row.removeFromRight(s::levelWidth), [this] {
        browsingVersions = false;
        triggerAsyncUpdate();
    });
    if (versions.isEmpty()) {
        text(cs::noVersions, row);
        return;
    }
    row.removeFromRight(s::gap);
    button(cs::applyVersion, row.removeFromRight(s::nameWidth), [this] { restoreVersion(); });
    StringArray names;
    for (auto& file : versions)
        names.add(file.getLastModificationTime().formatted("%d.%m.%Y   %H:%M:%S"));
    combo(cs::versionTime, row, names, versionIndex, [this](int i) { versionIndex = i; });
}
void Studio::startRepair() {
    if (comparing)
        stopProject();
    if (performance)
        return;
    if (!selection().valid() || selectedTrack().isEmpty()) {
        status(cs::pickRepair);
        return;
    }
    startProject(true, true);
}
void Studio::audition(TakeGroup& group) {
    view.cursor = std::max<Frame>(0, group.from - song.grid.beat(song.grid.beats));
    startProject(false, false, &group);
    comparing = performance != nullptr;
}
void Studio::chooseAttempt(bool keep) {
    auto* g = song.groupAt(selectedTrack(), selection().from, selection().to);
    if (!g || (performance && performance->recording))
        return;
    if (previewGroup != g->id || preview < 0 || preview > (int)g->attempts.size()) {
        preview = g->active;
        previewGroup = g->id;
    }
    if (keep) {
        g->active = preview;
        history.remember(song, cs::keepAttempt);
        saveSong();
        status(cs::savedAttempt);
        return;
    }
    do {
        preview = (preview + 1) % (1 + (int)g->attempts.size());
    } while (preview && g->attempts[preview - 1].discarded);
    if (performance)
        performance->playback.selectAttempt(g->id, preview);
    else
        audition(*g);
    triggerAsyncUpdate();
}
void Studio::discardAttempt() {
    auto* g = song.groupAt(selectedTrack(), selection().from, selection().to);
    if (!g || (performance && performance->recording))
        return;
    int index = previewGroup == g->id && preview >= 0 ? preview : g->active;
    if (!index || index > (int)g->attempts.size())
        return;
    g->active = index;
    g->discard();
    preview = g->active;
    previewGroup = g->id;
    if (performance)
        performance->playback.selectAttempt(g->id, preview);
    history.remember(song, cs::discardAttempt);
    saveSong();
    status(cs::discardedAttempt);
}
void Studio::attemptDeck(TakeGroup& g, Rect area) {
    int index =
        previewGroup == g.id && preview >= 0 && preview <= (int)g.attempts.size() ? preview : g.active;
    auto line = area.removeFromTop(s::button);
    text(cs::takeGroup, line.removeFromLeft(s::levelWidth), s::heading);
    String name =
        cs::attemptLabel + " " + String(index + 1) + " · " +
        (index ? Time(g.attempts[index - 1].created).formatted("%d.%m. %H:%M:%S") : cs::originalTake);
    text(name, line, s::body);
    auto row = area.removeFromTop(s::button);
    auto add = [&](String label, Handler fn) {
        auto* b = button(label, row.removeFromLeft(s::clipWidth), std::move(fn));
        row.removeFromLeft(s::grid);
        return b;
    };
    add(cs::repair, [this] { startRepair(); });
    add(cs::nextAttempt, [this] { chooseAttempt(false); });
    add(cs::keepAttempt, [this] { chooseAttempt(true); });
    add(cs::discardAttempt, [this] { discardAttempt(); })->setEnabled(index != 0);
    if (std::any_of(g.attempts.begin(), g.attempts.end(), [](auto& a) { return a.discarded; }))
        add(cs::restoredTake, [this] {
            auto* active = song.groupAt(selectedTrack(), selection().from, selection().to);
            if (!active || performance)
                return;
            for (size_t i = active->attempts.size(); i > 0; --i) {
                auto& take = active->attempts[i - 1];
                if (!take.discarded)
                    continue;
                take.discarded = false;
                active->active = (int)i;
                previewGroup = active->id;
                preview = (int)i;
                history.remember(song, cs::restoredTake);
                saveSong();
                triggerAsyncUpdate();
                break;
            }
        });
}
void Studio::editDeck(Rect area) {
    auto span = selection();
    if (auto* group = song.groupAt(selectedTrack(), span.from, span.to)) {
        attemptDeck(*group, area);
        return;
    }
    auto line = area.removeFromTop(s::button);
    String caption = cs::rangeHint;
    if (span.valid() && !selectedTrack().isEmpty()) {
        auto first = song.grid.beatAt(span.from) / song.grid.beats + 1,
             last = song.grid.beatAt(span.to - 1) / song.grid.beats + 1;
        caption = cs::bars + " " + String(first) +
                  (last == first ? String{} : String(u8"–") + String(last)) + "  ·  " +
                  song.tracks[view.selected].name.substring(0, 32) + "  ·  " +
                  String(double(span.length()) / song.grid.rate, 2).replaceCharacter('.', ',') + " s";
    }
    text(cs::selectionTitle, line.removeFromLeft(s::levelWidth), s::heading);
    text(caption, line, s::body);
    area.removeFromTop(s::gap);
    auto row = area.removeFromTop(s::button);
    auto action = [&](String label, Handler f, bool enabled) {
        auto* b = button(label, row.removeFromLeft(s::levelWidth), std::move(f));
        b->setEnabled(enabled && !performance);
        b->setAlpha(b->isEnabled() ? 1.f : s::disabledOpacity);
        row.removeFromLeft(s::gap);
    };
    bool selected = span.valid();
    action(cs::split, [this] { editOp(EditOp::split); }, true);
    action(cs::copy, [this] { editOp(EditOp::copy); }, selected);
    action(cs::paste, [this] { editOp(EditOp::paste); }, !clipboard.clips.empty());
    action(cs::repeat, [this] { editOp(EditOp::repeat); }, selected);
    action(cs::erase, [this] { editOp(EditOp::erase); }, selected);
    action(cs::undo, [this] { undo(false); }, history.canStep(false));
    action(cs::redo, [this] { undo(true); }, history.canStep(true));
    action(cs::repair, [this] { startRepair(); }, selected);
    auto settingsRow = area.removeFromBottom(s::button);
    StringArray lengths;
    for (int i = 0; i <= 8; ++i)
        lengths.add(String(i));
    combo(cs::preRoll, settingsRow.removeFromLeft(s::clipWidth), lengths, view.leadBars, [this](int n) {
        view.leadBars = n;
        view.save(projectDir);
    });
    settingsRow.removeFromLeft(s::gap);
    combo(cs::postRoll, settingsRow.removeFromLeft(s::clipWidth), lengths, view.tailBars, [this](int n) {
        view.tailBars = n;
        view.save(projectDir);
    });
    text(cs::editHint, settingsRow.withTrimmedLeft(s::gap), s::caption, s::muted);
}
bool Studio::editKey(const KeyPress& key) {
    auto code = key.getKeyCode();
    auto mods = key.getModifiers();
    if (mods.isCtrlDown() || mods.isCommandDown()) {
        if (code == 'Z' || code == 'Y') {
            undo(code == 'Y' || mods.isShiftDown());
            return true;
        }
        if (code == 'S') {
            saveSong();
            return true;
        }
        if (!view.editing)
            return false;
        if (code == 'C' || code == 'V') {
            editOp(code == 'C' ? EditOp::copy : EditOp::paste);
            return true;
        }
        return false;
    }
    if (view.distant)
        return false;
    if (code == '1' || code == '2') {
        editMode(code == '2');
        return true;
    }
    if (!view.editing)
        return false;
    if (code == KeyPress::tabKey || code == KeyPress::returnKey) {
        if (!song.groupAt(selectedTrack(), selection().from, selection().to))
            return false;
        chooseAttempt(code == KeyPress::returnKey);
        return true;
    }
    switch (code) {
    case 'P':
        startRepair();
        return true;
    case 'X':
        editOp(EditOp::split);
        return true;
    case 'D':
        editOp(EditOp::repeat);
        return true;
    default:
        break;
    }
    if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey) {
        editOp(EditOp::erase);
        return true;
    }
    if (code == '+' || code == '-' || key.getTextCharacter() == '+' || key.getTextCharacter() == '-') {
        view.zoom = (code == '+' || key.getTextCharacter() == '+') ? std::max(1, visibleBars() / 2)
                                                                   : std::min(64, visibleBars() * 2);
        reveal(view.cursor);
        view.save(projectDir);
        triggerAsyncUpdate();
        return true;
    }
    if (!performance && (key == KeyPress::homeKey || key == KeyPress::endKey ||
                         key == KeyPress::leftKey || key == KeyPress::rightKey)) {
        auto bar = song.grid.beatAt(view.cursor) / song.grid.beats;
        view.cursor =
            key == KeyPress::homeKey ? 0
            : key == KeyPress::endKey
                ? song.end()
                : song.grid.beat(std::max<Frame>(0, bar + (key == KeyPress::leftKey ? -1 : 1)) *
                                 song.grid.beats);
        reveal(view.cursor);
        view.save(projectDir);
        repaint();
        return true;
    }
    return false;
}
void Studio::editMouseDown(const MouseEvent& e) {
    const auto axis = ruler.withTrimmedLeft(s::trackHead);
    pointer = e.getPosition();
    dragged = false;
    drag = Drag::none;
    if (!axis.withBottom(timelineArea.getBottom()).contains(pointer))
        return;
    auto pos = frameAt(e.x);
    if (axis.contains(pointer)) {
        auto bar = firstBar();
        while (bar + 1 < firstBar() + visibleBars() &&
               pixelAt(song.grid.beat((bar + 1) * song.grid.beats)) <= e.x)
            ++bar;
        selectBar(bar, e.mods.isShiftDown());
        drag = Drag::bars;
    } else {
        for (size_t i = 0; i < lanes.size(); ++i)
            if (lanes[i].contains(pointer)) {
                int index = firstTrack + int(i);
                if (index != view.selected) {
                    view.selected = index;
                    view.selectedClip.clear();
                    projectMonitor = 0;
                    configureSongMonitor();
                }
                auto track = selectedTrack();
                for (const auto& group : song.groups)
                    if (track == group.track && e.x >= pixelAt(group.from) - s::grid &&
                        e.x <= pixelAt(group.to) + s::grid && pointer.y < lanes[i].getY() + s::button) {
                        pick({group.from, group.to});
                        if (e.mods.isPopupMenu()) {
                            PopupMenu menu;
                            menu.addItem(1, cs::discardAttempt, group.active != 0);
                            Component::SafePointer<Studio> safe(this);
                            menu.showMenuAsync(PopupMenu::Options{}.withTargetComponent(this),
                                               [safe, id = group.id](int chosen) {
                                                   if (!safe || chosen != 1)
                                                       return;
                                                   auto* current = safe->song.groupAt(
                                                       safe->selectedTrack(), safe->selection().from,
                                                       safe->selection().to);
                                                   if (current && current->id == id)
                                                       safe->discardAttempt();
                                               });
                        } else if (std::abs(e.x - pixelAt(group.from)) <= s::grid)
                            drag = Drag::takeLeft;
                        else if (std::abs(e.x - pixelAt(group.to)) <= s::grid)
                            drag = Drag::takeRight;
                        view.save(projectDir);
                        triggerAsyncUpdate();
                        return;
                    }
                auto it = std::find_if(song.clips.rbegin(), song.clips.rend(), [&](const Clip& c) {
                    if (c.track != track)
                        return false;
                    const int first = pixelAt(c.position), last = pixelAt(c.position + c.length);
                    return e.x >= first - s::grid && e.x <= last + s::grid;
                });
                bool inside =
                    selection().valid() && pos >= view.selectionStart && pos < view.selectionEnd;
                if (it != song.clips.rend()) {
                    const auto& c = *it;
                    const int lx = pixelAt(c.position), rx = pixelAt(c.position + c.length);
                    if (std::abs(e.x - lx) <= s::grid || std::abs(e.x - rx) <= s::grid) {
                        drag = std::abs(e.x - lx) <= std::abs(e.x - rx) ? Drag::left : Drag::right;
                        pick({c.position, c.position + c.length}, c.id);
                    } else if (!inside || view.selectedClip.isNotEmpty())
                        pick({c.position, c.position + c.length}, c.id);
                    if (drag == Drag::none)
                        drag = Drag::move;
                } else if (inside)
                    drag = Drag::move;
                else
                    pick({});
                view.cursor = pos;
                grabOffset = pos - view.selectionStart;
                break;
            }
    }
    view.save(projectDir);
    triggerAsyncUpdate();
}
void Studio::mouseDrag(const MouseEvent& e) {
    if (drag == Drag::none || page != Page::project || !view.editing || performance ||
        !e.mouseWasDraggedSinceMouseDown())
        return;
    dragged = true;
    pointer = e.getPosition();
    auto axis = ruler.withTrimmedLeft(s::trackHead);
    double now = Time::getMillisecondCounterHiRes();
    if (now - lastPan > 240 && (e.x < axis.getX() || e.x >= axis.getRight())) {
        view.firstBar = jlimit<Frame>(0, 999999, view.firstBar + (e.x < axis.getX() ? -1 : 1));
        lastPan = now;
    }
    Frame at = frameAt(e.x);
    if (drag == Drag::bars)
        selectBar(song.grid.beatAt(at) / song.grid.beats, true);
    else {
        at = std::max<Frame>(0, at - (drag == Drag::move ? grabOffset : 0));
        dragTarget = e.mods.isAltDown() ? at : snapBeat(song.grid, at);
        if (drag != Drag::move)
            for (const auto& c : song.clips)
                if (c.id == view.selectedClip) {
                    if (drag == Drag::left)
                        dragTarget = jlimit(std::max<Frame>(0, c.position - c.offset),
                                            c.position + c.length - 1, dragTarget);
                    else
                        for (auto& a : song.assets)
                            if (a.id == c.asset)
                                dragTarget =
                                    jlimit(c.position + 1, c.position + a.length - c.offset, dragTarget);
                    break;
                }
    }
    repaint();
}
void Studio::mouseUp(const MouseEvent&) {
    if (!view.editing || drag == Drag::none)
        return;
    if (dragged && (drag == Drag::takeLeft || drag == Drag::takeRight)) {
        if (auto* group = song.groupAt(selectedTrack(), selection().from, selection().to)) {
            Frame first = 0, last = std::numeric_limits<Frame>::max();
            for (const auto& attempt : group->attempts)
                for (const auto& pass : attempt.clips) {
                    first = std::max(first, pass.position);
                    last = std::min(last, pass.position + pass.length);
                }
            if (drag == Drag::takeLeft && first < group->to)
                group->from = jlimit(first, group->to - 1, dragTarget);
            if (drag == Drag::takeRight && last > group->from)
                group->to = jlimit(group->from + 1, last, dragTarget);
            pick({group->from, group->to});
            history.remember(song, cs::trim);
            saveSong();
        }
    } else if (dragged && drag != Drag::bars)
        editOp(drag == Drag::move   ? EditOp::move
               : drag == Drag::left ? EditOp::trimLeft
                                    : EditOp::trimRight);
    drag = Drag::none;
    dragged = false;
    view.save(projectDir);
    triggerAsyncUpdate();
}
void Studio::paintEditOverlay(Graphics& g) {
    auto axis = ruler.withTrimmedLeft(s::trackHead).withBottom(timelineArea.getBottom());
    Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(axis);
    g.setColour(s::beatGrid);
    for (int i = 0; i < visibleBars(); ++i)
        for (int b = 1; b < song.grid.beats; ++b) {
            int x = pixelAt(song.grid.beat((firstBar() + i) * song.grid.beats + b));
            g.drawVerticalLine(x, axis.getY() + s::button, axis.getBottom());
        }
    for (size_t i = 0; i < lanes.size(); ++i)
        for (const auto& group : song.groups) {
            if (group.track != song.tracks[firstTrack + i].id)
                continue;
            auto badge =
                lanes[i].withLeft(pixelAt(group.from)).withRight(pixelAt(group.to)).withHeight(s::button);
            g.setColour(s::raised);
            g.fillRect(badge);
            g.setColour(s::chalk);
            g.setFont(skin.font(s::caption));
            StringArray numbers{"1"};
            for (size_t n = 0; n < group.attempts.size(); ++n)
                if (!group.attempts[n].discarded)
                    numbers.add(String((int)n + 2));
            g.drawFittedText(cs::takeGroup + " " + numbers.joinIntoString(" | "), badge,
                             Justification::centred, 1);
        }
    Span span = selection();
    if (dragged && drag == Drag::move)
        span = {dragTarget, dragTarget + span.length()};
    if (dragged && drag == Drag::takeLeft)
        span.from = std::min(dragTarget, span.to - 1);
    if (dragged && drag == Drag::takeRight)
        span.to = std::max(dragTarget, span.from + 1);
    if (span.valid()) {
        auto bounds = axis.withLeft(pixelAt(span.from)).withRight(pixelAt(span.to));
        g.setColour(s::selection.withAlpha(s::selectionOpacity));
        g.fillRect(bounds);
        g.setColour(s::selection);
        g.drawRect(bounds, s::stroke);
    }
}
void Studio::paintOverChildren(Graphics& g) {
    if (!dragged || (drag != Drag::left && drag != Drag::right) || page != Page::project)
        return;
    auto box = Rect(0, 0, s::fieldWidth, s::footer + s::button)
                   .withCentre(pointer.translated(0, -s::footer))
                   .constrainedWithin(getLocalBounds());
    Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(box);
    g.setColour(s::raised);
    g.fillRect(box);
    g.setColour(s::chalk);
    g.setFont(skin.font(s::body, true));
    auto title = box.removeFromTop(s::button);
    auto beat = song.grid.beatAt(dragTarget);
    g.drawText(cs::magnifier + "   " + String(beat / song.grid.beats + 1) + "." +
                   String(beat % song.grid.beats + 1),
               title, Justification::centred);
    double perPixel = double(frameAt(ruler.getRight()) - frameAt(ruler.getX() + s::trackHead)) /
                      std::max(1, ruler.getWidth() - s::trackHead) / 4.;
    Frame begin = dragTarget - std::llround(box.getWidth() * 0.5 * perPixel);
    for (Frame b = song.grid.beatAt(begin); song.grid.beat(b) < begin + box.getWidth() * perPixel; ++b) {
        int x = box.getX() + roundToInt((song.grid.beat(b) - begin) / perPixel);
        g.setColour(b % song.grid.beats == 0 ? s::barGrid : s::beatGrid);
        g.drawVerticalLine(x, box.getY(), box.getBottom());
    }
    for (auto& c : song.clips)
        if (c.id == view.selectedClip) {
            auto wave = library.data.find(c.asset);
            if (wave == library.data.end())
                break;
            for (int i = 0; i < box.getWidth(); ++i) {
                Frame at = begin + std::llround(i * perPixel), source = c.offset + at - c.position;
                if (source < 0 || source >= wave->second->reader->lengthInSamples)
                    continue;
                float h = std::min(1.f, wave->second->peak(source, std::max(1, roundToInt(perPixel)))) *
                          box.getHeight() / 2;
                bool active = drag == Drag::left ? at >= dragTarget && at < c.position + c.length
                                                 : at >= c.position && at < dragTarget;
                g.setColour(active ? s::chalk : s::faint);
                g.drawVerticalLine(box.getX() + i, box.getCentreY() - h,
                                   box.getCentreY() + h + s::hairline);
            }
            break;
        }
    g.setColour(s::selection);
    g.fillRect(box.getCentreX(), box.getY(), s::stroke, box.getHeight());
}
} // namespace rocker
