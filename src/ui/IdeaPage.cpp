#include "Studio.h"
#include "Strings.h"

namespace rocker {
using namespace juce;
namespace s = style;
namespace {
using Rect = Rectangle<int>;
Rect row(Rect& area, int height = s::button) {
    auto r = area.removeFromTop(height);
    area.removeFromTop(s::gap);
    return r;
}
Rect cell(Rect& area, int w) {
    auto r = area.removeFromLeft(w);
    area.removeFromLeft(s::gap);
    return r;
}
String duration(Frame length, int rate) {
    auto seconds = length / std::max(1, rate);
    return String(seconds / 60) + ":" + String(seconds % 60).paddedLeft('0', 2);
}
String title(const Idea& idea) {
    return cs::ideaTitle + Time(idea.created).formatted("%d. %m. %H:%M");
}
bool ready(auto& job) {
    return job.valid() && job.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
} // namespace
const Idea* Studio::pickedIdea() const {
    for (auto& x : ideas->all())
        if (x.id == ideaId)
            return &x;
    return nullptr;
}
void Studio::chooseIdea(const Idea& idea) {
    pendingIdeaRemoval.clear();
    stopPreview();
    ideaId = idea.id;
    ideaNote = idea.note;
    triggerAsyncUpdate();
}
void Studio::enableInsurance() {
    if (ideaRecording)
        return;
    settings.insurance = !settings.insurance;
    link.audio.setInsurance(settings.insurance);
    persist();
    triggerAsyncUpdate();
}
void Studio::startIdea(bool retrospective) {
    if (ideaRecording || savingIdea.valid() || (retrospective && !settings.insurance))
        return;
    if (!link.online || !link.audio.ideaInput || link.caps.inputs.isEmpty()) {
        status(cs::badIdeaInput);
        return;
    }
    stopPreview();
    show(Page::ideas);
    auto input = link.audio.ideaInput;
    if (retrospective && input->end() <= input->earliest()) {
        status(cs::emptyHistory);
        return;
    }
    capturedIdea = {};
    capturedIdea.id = Uuid().toString();
    capturedIdea.created = Time::getCurrentTime().toMilliseconds();
    capturedIdea.tuning = tunings[tuningIndex].name;
    ideaRecording = std::make_unique<IdeaRecording>(input, ideas->wave(capturedIdea.id), retrospective);
    if (!ideaRecording->valid()) {
        ideaRecording.reset();
        status(cs::ideaSaveError);
        return;
    }
    recentInput.clear();
    if (retrospective) {
        stopIdea();
        return;
    }
    if (ideaClick) {
        capturedIdea.bpm = ideaTempo;
        capturedIdea.beats = ideaBeats;
        capturedIdea.unit = ideaUnit;
        capturedIdea.firstBeat = ideaRecording->recorded();
        Song click;
        click.grid = {link.current.rate, ideaBeats, ideaUnit, ideaTempo};
        click.bars = 1000000;
        ideaMetronome = std::make_unique<Performance>(click, AudioLibrary{}, 0, false, true, 0,
                                                      ideaRhythm);
        ideaMetronome->configureClick(settings.clickVolume, settings.clickSound);
        link.audio.timeline.attach(*ideaMetronome);
    }
    link.audio.live.selectInput(link.audio.ideaPort.load());
    link.audio.live.setAmp(sound);
    link.audio.live.monitor = laptop                                     ? Monitor::off
                              : (guitarSource == 0 || guitarSource == 3) ? Monitor::amp
                                                                         : Monitor::clean;
    message.clear();
    triggerAsyncUpdate();
}
void Studio::stopIdea() {
    if (!ideaRecording)
        return;
    if (ideaMetronome) {
        link.audio.timeline.detach();
        ideaMetronome.reset();
    }
    link.audio.live.silence();
    ideaRecording->requestStop();
    auto folder = ideas->root();
    savingIdea =
        std::async(std::launch::async, [take = std::move(ideaRecording), x = capturedIdea, folder] {
            if (!take->finish())
                return false;
            Ideas catalog(folder);
            return catalog.append(x);
        });
    status(cs::savingIdea);
}
void Studio::stopPreview() {
    if (ideaPreview) {
        link.audio.timeline.detach();
        ideaPreview.reset();
    }
    acceptPreview = false;
    tapping = false;
    taps.clear();
    ideaAudio.data.clear();
    if (previewCache != File{}) {
        previewCache.deleteRecursively();
        previewCache = File{};
    }
}
void Studio::playIdea(bool tap) {
    if (ideaRecording || savingIdea.valid())
        return;
    if (ideaPreview) {
        stopPreview();
        triggerAsyncUpdate();
        if (!tap)
            return;
    }
    auto* x = pickedIdea();
    if (!x || !link.online || link.caps.outputChannels == 0) {
        status(cs::noDeviceSong);
        return;
    }
    if (preparingPreview.valid())
        return;
    acceptPreview = true;
    auto folder = ideas->root();
    auto original = ideas->wave(x->id);
    preparingPreview = std::async(
        std::launch::async, [item = *x, folder, original, rate = link.current.rate, tap, amp = sound] {
            PreviewSource result;
            result.id = item.id;
            result.tap = tap;
            result.rate = rate;
            result.song.grid.rate = rate;
            result.song.bars = 0;
            auto dir = folder;
            Asset asset{item.id, "Zvuk/" + item.id + ".wav", item.length, 1};
            if (rate != item.rate) {
                result.temp = File::getSpecialLocation(File::tempDirectory)
                                  .getNonexistentChildFile("Rocker2-preview", "", false);
                auto a = importAudio(original, result.temp, rate);
                if (!a)
                    return result;
                asset = *a;
                dir = result.temp;
            }
            Track t;
            t.id = "idea";
            t.kind = "kytara";
            t.amp.values = amp;
            t.amp.enabled = true;
            result.song.tracks = {t};
            result.song.assets = {asset};
            result.song.clips = {{"riff", t.id, asset.id, 0, 0, asset.length}};
            result.ok = result.audio.load(result.song, dir);
            return result;
        });
    triggerAsyncUpdate();
}
void Studio::finishPreview() {
    PreviewSource result;
    try {
        result = preparingPreview.get();
    } catch (const std::exception&) {
    }
    bool current = acceptPreview && page == Page::ideas && result.id == ideaId &&
                   result.rate == link.current.rate && link.online;
    if (!current || !result.ok) {
        result.audio.data.clear();
        if (result.temp != File{})
            result.temp.deleteRecursively();
        if (current)
            status(cs::previewRate);
        return;
    }
    previewCache = result.temp;
    ideaAudio = std::move(result.audio);
    ideaPreview = std::make_unique<Performance>(result.song, ideaAudio, 0, false, false, 0);
    tapping = result.tap;
    taps.clear();
    link.audio.timeline.attach(*ideaPreview);
    status(tapping ? cs::tapHint : String{});
}
void Studio::tapIdea() {
    if (!ideaPreview || !tapping || !pickedIdea())
        return;
    Idea x = *pickedIdea();
    taps.push_back(std::llround(double(ideaPreview->cursor.load()) * x.rate / link.current.rate));
    if (taps.size() < 8) {
        triggerAsyncUpdate();
        return;
    }
    auto tempo = measureTaps(taps, x.rate);
    stopPreview();
    if (!tempo) {
        status(cs::badTaps);
        return;
    }
    x.bpm = tempo->bpm;
    x.firstBeat = tempo->firstBeat;
    status(ideas->update(x) ? cs::freeTempo : cs::ideaSaveError);
}
void Studio::makeIdeaSong() {
    auto* x = pickedIdea();
    if (!x)
        return;
    stopPreview();
    auto dest = libraryRoot.getNonexistentChildFile(File::createLegalFileName(title(*x)), "", true);
    if (!ideas->toSong(*x, dest, title(*x), cs::guitar)) {
        status(cs::songSaveError);
        return;
    }
    openProject(dest);
    if (x->bpm <= 0)
        status(cs::freeTempo);
}
void Studio::ideaTick() {
    if (ready(preparingPreview))
        finishPreview();
    if (ready(savingIdea)) {
        bool saved = false;
        try {
            saved = savingIdea.get();
        } catch (const std::exception&) {
        }
        ideas->load();
        if (saved && !ideas->all().empty()) {
            auto& x = ideas->all().front();
            ideaId = x.id;
            ideaNote = x.note;
            ideaOffset = 0;
            status(cs::ideaSaved + title(x) + " (" + duration(x.length, x.rate) + ")");
        } else
            status(cs::ideaSaveError);
    }
    if (ideaPreview && ideaPreview->finished.load()) {
        bool incomplete = tapping;
        stopPreview();
        status(incomplete ? cs::badTaps : String{});
    }
    if (ideaRecording && page == Page::ideas) {
        recentInput.push_back(link.audio.read(link.audio.ideaPort.load()).peak);
        while (recentInput.size() > 256)
            recentInput.pop_front();
        repaint();
    }
    if (!performance && ready(importing))
        finishImport();
}
void Studio::ideaHome(Rect area) {
    for (const auto& idea : ideas->all()) {
        if (area.getHeight() < s::button)
            break;
        auto r = row(area);
        auto time = r.removeFromRight(s::statusStrip);
        auto trace = r.removeFromRight(s::railWidth);
        auto name = (idea.star ? cs::starMark : String{}) + title(idea);
        button(limit(skin.font(s::body, false, true), name, r.getWidth() - 2 * s::inset), r,
               [this, id = idea.id] {
                   show(Page::ideas);
                   for (const auto& x : ideas->all())
                       if (x.id == id) {
                           chooseIdea(x);
                           break;
                       }
               })->setTooltip(name);
        text(duration(idea.length, idea.rate), time, s::caption, s::muted, true, false, true);
        ideaWaves[idea.id] = trace.reduced(s::grid, s::stroke);
    }
}
void Studio::ideaPage(Rect area) {
    area.reduce(0, s::inset);
    auto head = row(area, s::header);
    text(ideaRecording ? cs::ideaRecording : cs::ideas, cell(head, s::fieldWidth), s::title,
         ideaRecording ? s::record : s::chalk);
    if (ideaRecording) {
        text(cs::guitar, cell(head, s::nameWidth), s::heading, s::guitar);
        button(
            cs::keepRecording, row(area, s::recordButton).withWidth(s::labelWidth),
            [this] { stopIdea(); }, 1);
        ideaTrace = area.removeFromTop(s::soundDeck);
        return;
    }
    auto actions = row(area);
    button(
        cs::record, cell(actions, s::nameWidth), [this] { startIdea(false); }, 1)
        ->setEnabled(!savingIdea.valid());
    button(cs::keepHistory, cell(actions, s::nameWidth), [this] {
        startIdea(true);
    })->setEnabled(settings.insurance && !savingIdea.valid());
    button(settings.insurance ? cs::insuranceOn : cs::insuranceOff, cell(actions, s::clipWidth),
           [this] { enableInsurance(); });
    button(
        cs::ideaClick, cell(actions, s::clipWidth),
        [this] {
            ideaClick = !ideaClick;
            triggerAsyncUpdate();
        },
        ideaClick ? 1 : 0);
    if (ideaClick) {
        const auto label = String(ideaTempo, 1).replaceCharacter('.', ',') + " BPM" + cs::divider +
                           String(ideaBeats) + "/" + String(ideaUnit);
        auto* tempo = button(cs::tempo, actions.removeFromLeft(s::labelWidth),
                             [this] { openMetronome(); }, plain, {}, Icon::none, label);
        tempo->setTooltip(label);
        tempo->getProperties().set("number", true);
    } else if (link.caps.inputs.size() > 1)
        combo(cs::chooseIdeaInput, actions, inputNames(), link.audio.ideaPort.load(), [this](int i) {
            link.audio.ideaPort = i;
            if (link.audio.ideaInput)
                link.audio.ideaInput->forget();
        });
    auto search = row(area);
    button(
        starsOnly ? cs::allIdeas : cs::starredOnly, cell(search, s::clipWidth),
        [this] {
            starsOnly = !starsOnly;
            ideaOffset = 0;
            triggerAsyncUpdate();
        },
        starsOnly ? 1 : 0);
    field(cs::searchNote, cell(search, s::fieldWidth), ideaFilter);
    button(cs::filterIdeas, cell(search, s::clipWidth), [this] {
        ideaOffset = 0;
        triggerAsyncUpdate();
    });
    auto footer = area.removeFromBottom(s::button);
    text(limit(skin.font(s::body), message, footer.getWidth()), footer, s::body, s::warning);
    auto details = area.removeFromBottom(s::songDeck + s::button);
    std::vector<const Idea*> matches;
    for (const auto& x : ideas->all())
        if ((!starsOnly || x.star) && x.note.containsIgnoreCase(ideaFilter))
            matches.push_back(&x);
    auto nav = area.removeFromBottom(s::button);
    int visible = std::max(1, area.getHeight() / (s::button + s::gap));
    ideaOffset = std::clamp(ideaOffset, 0, std::max(0, (int)matches.size() - visible));
    if (ideaOffset > 0)
        button(cs::prevIdeas, cell(nav, s::levelWidth), [this, visible] {
            ideaOffset -= visible;
            triggerAsyncUpdate();
        });
    if (ideaOffset + visible < (int)matches.size())
        button(cs::nextIdeas, cell(nav, s::levelWidth), [this, visible] {
            ideaOffset += visible;
            triggerAsyncUpdate();
        });
    if (matches.empty())
        text(ideas->all().empty() ? cs::emptyIdeas : cs::noMatches, row(area));
    for (int i = ideaOffset; i < std::min(int(matches.size()), ideaOffset + visible); ++i) {
        const auto& x = *matches[i];
        auto r = row(area);
        button((x.star ? cs::starMark : String{}) + title(x), cell(r, s::labelWidth),
               [this, id = x.id] {
                   for (auto& item : ideas->all())
                       if (item.id == id) {
                           chooseIdea(item);
                           break;
                       }
               },
               x.id == ideaId ? 1 : 0);
        text(duration(x.length, x.rate), cell(r, s::scaleWidth), s::body, s::muted, true);
        ideaWaves[x.id] = cell(r, s::nameWidth);
        text(x.tuning + (x.bpm > 0 ? cs::divider + String(x.bpm, 1) + " BPM" : String{}),
             cell(r, s::nameWidth));
        text(limit(skin.font(s::body), x.note, r.getWidth()), r);
    }
    auto* x = pickedIdea();
    if (!x || std::find(matches.begin(), matches.end(), x) == matches.end()) {
        auto* next = matches.empty() ? nullptr : matches.front();
        if (x != next) {
            stopPreview();
            ideaId = next ? next->id : String{};
            ideaNote = next ? next->note : String{};
        }
        x = next;
    }
    if (x && !savingIdea.valid()) {
        auto buttons = row(details);
        button(ideaPreview ? cs::stopSong : cs::playSong, cell(buttons, s::levelWidth),
               [this] { playIdea(false); });
        button(
            cs::ideaStar, cell(buttons, s::clipWidth),
            [this] {
                auto item = *pickedIdea();
                item.star = !item.star;
                if (!ideas->update(item))
                    status(cs::ideaSaveError);
                triggerAsyncUpdate();
            },
            x->star ? 1 : 0);
        button(cs::useIdea, cell(buttons, s::nameWidth), [this] { makeIdeaSong(); }, 1);
        button(cs::tapIdea, cell(buttons, s::clipWidth), [this] { playIdea(true); });
        if (pendingIdeaRemoval != x->id) {
            button(cs::deleteIdea, cell(buttons, s::nameWidth), [this, id = x->id] {
                pendingIdeaRemoval = id;
                triggerAsyncUpdate();
            })->setEnabled(!ideaRecording && !ideaPreview && !preparingPreview.valid());
        }
        if (pendingIdeaRemoval == x->id) {
            auto confirm = row(details);
            text(cs::recycleIdea, cell(confirm, s::labelWidth));
            button(cs::answers[0], cell(confirm, s::compactButton), [this, id = x->id] {
                pendingIdeaRemoval.clear();
                if (ideaRecording || savingIdea.valid() || preparingPreview.valid() || ideaPreview)
                    return;
                const bool removed =
                    ideas->remove(id, [](const File& file) { return file.moveToTrash(); });
                status(removed ? cs::ideaRemoved : cs::ideaDeleteError);
            });
            button(cs::answers[2], cell(confirm, s::compactButton), [this] {
                pendingIdeaRemoval.clear();
                triggerAsyncUpdate();
            });
        }
        auto note = row(details);
        field(cs::note, cell(note, s::fieldWidth + s::nameWidth), ideaNote);
        button(cs::saveNote, cell(note, s::clipWidth), [this] {
            auto item = *pickedIdea();
            for (auto* c : controls)
                if (auto* edit = dynamic_cast<TextEditor*>(c); edit && edit->getName() == cs::note)
                    ideaNote = edit->getText();
            item.note = ideaNote;
            if (!ideas->update(item))
                status(cs::ideaSaveError);
            triggerAsyncUpdate();
        });
        if (tapping)
            button(cs::tap + " " + String(int(taps.size())) + " / 8",
                   row(details).withWidth(s::nameWidth), [this] { tapIdea(); });
    }
}
void Studio::paintIdeas(Graphics& g) {
    auto draw = [&](Rect r, const auto& peaks) {
        if (peaks.empty() || r.isEmpty())
            return;
        g.setColour(s::raised);
        g.fillRect(r);
        g.setColour(s::guitar);
        int count = (int)peaks.size();
        for (int i = 0; i < count; ++i) {
            float h = std::max(1.f, peaks[i] * r.getHeight() / 2);
            g.drawVerticalLine(r.getX() + i * r.getWidth() / count, r.getCentreY() - h,
                               r.getCentreY() + h);
        }
    };
    if (ideaRecording && !ideaTrace.isEmpty()) {
        g.setColour(s::recording);
        g.drawRect(getLocalBounds(), s::recordFrame);
        g.setFont(skin.font(s::counter, true));
        g.setColour(s::chalk);
        g.drawText(duration(ideaRecording->recorded(), link.current.rate),
                   getLocalBounds().reduced(s::margin, s::inset).removeFromTop(s::header),
                   Justification::centredRight);
        draw(ideaTrace, recentInput);
        return;
    }
    for (const auto& x : ideas->all())
        if (auto it = ideaWaves.find(x.id); it != ideaWaves.end())
            draw(it->second, x.peaks);
}
bool Studio::isInterestedInFileDrag(const StringArray& files) {
    if (fileTask.valid())
        return false;
    if (files.size() == 1 && (page == Page::home || page == Page::project) && !performance &&
        File(files[0]).hasFileExtension("rocker2"))
        return true;
    return page == Page::project && files.size() == 1 && supportsImport(File(files[0])) && !performance &&
           !importing.valid();
}
void Studio::filesDropped(const StringArray& files, int, int) {
    if (isInterestedInFileDrag(files) && File(files[0]).hasFileExtension("rocker2"))
        startPackage(true, File(files[0]));
    else if (isInterestedInFileDrag(files))
        beginImport(File(files[0]));
    else if (page == Page::project)
        status(cs::importFailed);
}
void Studio::chooseImport() {
    if (importing.valid() || performance)
        return;
    auto types = String("*.wav;*.aif;*.aiff;*.flac;*.ogg;*.oga;*.opus;*.mp3");
#if JUCE_WINDOWS || JUCE_MAC
    types += ";*.m4a;*.aac";
#endif
    importChooser = std::make_unique<FileChooser>(cs::addBacking, File{}, types);
    importChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                               [self = Component::SafePointer<Studio>(this)](const FileChooser& f) {
                                   if (self && f.getResult().existsAsFile())
                                       self->beginImport(f.getResult());
                               });
}
void Studio::beginImport(const File& file) {
    if (page != Page::project || importing.valid() || performance)
        return;
    importName = file.getFileNameWithoutExtension();
    importProject = projectDir;
    importPosition = importAtCursor ? view.cursor : 0;
    importing = std::async(std::launch::async, [file, dir = projectDir, rate = song.grid.rate] {
        return importAudio(file, dir, rate);
    });
    status(cs::importing);
}
void Studio::finishImport() {
    std::optional<Asset> a;
    try {
        a = importing.get();
    } catch (const std::exception&) {
    }
    if (!a) {
        status(cs::importFailed);
        return;
    }
    // Complete in the originating song even if the user opened a different project meanwhile.
    bool here = projectDir == importProject;
    bool damaged = false;
    auto target = here ? std::optional<Song>(song) : Song::load(importProject, damaged);
    if (!target || damaged) {
        status(cs::importError);
        return;
    }
    Track t;
    t.id = Uuid().toString();
    t.name = importName;
    t.kind = "podklad";
    target->extendOrder(importPosition + a->length);
    target->assets.push_back(*a);
    target->tracks.push_back(t);
    target->clips.push_back({Uuid().toString(), t.id, a->id, importPosition, 0, a->length});
    target->commitClips();
    if (!target->save(importProject)) {
        status(cs::importError);
        return;
    }
    if (here) {
        if (performance)
            stopProject();
        song = std::move(*target);
        view.selected = int(song.tracks.size()) - 1;
        history.remember(song, cs::addBacking);
        view.save(projectDir);
        if (!library.load(song, projectDir)) {
            status(cs::partialProject);
            return;
        }
    }
    status(cs::savedSong);
}
} // namespace rocker
