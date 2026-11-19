#include "Studio.h"
#include "Strings.h"
#include "storage/ProjectFiles.h"
#include "storage/AtomicFile.h"

namespace rocker {
using namespace juce;
namespace s = style;
void Studio::fileOptions(Rect& row) {
    if (confirmClean) {
        button(cs::moveTrash, row.removeFromRight(s::nameWidth), [this] { cleanProject(); }, 1);
        button(cs::back, row.removeFromRight(s::levelWidth), [this] {
            confirmClean = false;
            triggerAsyncUpdate();
        });
        return;
    }
    button(cs::cleanSong, row.removeFromRight(s::clipWidth), [this] {
        const auto count = unusedAudio(song, projectDir, history).size();
        confirmClean = count > 0;
        status(count ? cs::trashPreview + " · " + String(count) : cs::nothingToClean);
    });
    button(cs::remap, row.removeFromRight(s::clipWidth), [this] { beginRemap(); });
    button(cs::openPackage, row.removeFromRight(s::clipWidth), [this] { choosePack(true); });
    button(cs::packageSong, row.removeFromRight(s::clipWidth), [this] { choosePack(false); }, 1);
}
void Studio::cleanProject() {
    confirmClean = false;
    library.data.clear(); // Release memory maps before moving unused files on Windows.
    const int result =
        trashAudio(song, projectDir, history, [](const File& f) { return f.moveToTrash(); });
    bool loaded = library.load(song, projectDir);
    status(result < 0 || !loaded ? cs::writeError : cs::moveTrash + " · " + String(result));
}
void Studio::choosePack(bool open) {
    if (fileTask.valid() || performance || importing.valid() || delivery.valid())
        return;
    auto dest = open ? libraryRoot
                     : projectDir.getSiblingFile(File::createLegalFileName(song.title) + ".rocker2");
    fileChooser =
        std::make_unique<FileChooser>(open ? cs::openPackage : cs::packageSong, dest, "*.rocker2");
    auto mode = open ? FileBrowserComponent::openMode
                     : FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync(mode | FileBrowserComponent::canSelectFiles,
                             [self = Component::SafePointer<Studio>(this), open](const FileChooser& f) {
                                 if (!self || f.getResult() == File{})
                                     return;
                                 self->startPackage(open, f.getResult());
                             });
}
void Studio::startPackage(bool open, File file) {
    if (fileTask.valid() || performance || importing.valid() || delivery.valid())
        return;
    if (!open) {
        if (!song.save(projectDir) || !view.save(projectDir)) {
            status(cs::writeError);
            return;
        }
    }
    fileTask = std::async(std::launch::async, [open, file, project = projectDir, root = libraryRoot] {
        if (open) {
            auto dir = unpackSong(file, root);
            return FileResult{dir.value_or(File{}), bool(dir)};
        }
        return FileResult{{}, packSong(project, file.withFileExtension(".rocker2"))};
    });
    triggerAsyncUpdate();
}
void Studio::pollFiles() {
    if (!fileTask.valid() || fileTask.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;
    auto loaded = fileTask.get();
    if (loaded.ok && loaded.song != File{})
        openProject(loaded.song);
    else
        status(loaded.ok ? cs::transferDone : cs::transferFailed);
}
void Studio::chooseLibrary() {
    fileChooser = std::make_unique<FileChooser>(cs::libraryFolder, libraryRoot);
    fileChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                             [self = Component::SafePointer<Studio>(this)](const FileChooser& f) {
                                 if (!self || !f.getResult().isDirectory())
                                     return;
                                 self->libraryRoot = f.getResult();
                                 self->settings.libraryPath = self->libraryRoot.getFullPathName();
                                 self->persist();
                                 self->status(syncedFolder(self->libraryRoot) ? cs::syncPath : cs::saved);
                             });
}
void Studio::beginRemap() {
    remapped.clear();
    for (auto& t : song.tracks) {
        auto ports = t.inputs;
        for (int i = 0; i < ports.size(); ++i)
            if (!link.caps.inputs.contains(ports[i]))
                ports.set(i, {});
        remapped.push_back(ports);
    }
    remapIndex = 0;
    show(Page::remap);
}
void Studio::remapPage(Rect area) {
    auto line = [&](int height = s::button) {
        auto r = area.removeFromTop(height);
        area.removeFromTop(s::gap);
        return r;
    };
    text(cs::remap, line(s::header), s::title);
    StringArray names;
    for (auto& t : song.tracks)
        names.add(t.name);
    if (!names.isEmpty()) {
        combo(cs::trackLabel, line(), names, remapIndex, [this](int i) {
            remapIndex = i;
            triggerAsyncUpdate();
        });
        auto& track = song.tracks[remapIndex];
        auto& routes = remapped[(size_t)remapIndex];
        text(cs::oldInput + " · " +
                 (track.kind == "midi" ? cs::midiModule : track.inputs.joinIntoString(" + ")),
             line());
        if (track.kind != "midi") {
            StringArray options{cs::noneAssigned};
            options.addArray(link.caps.inputs);
            for (int c = 0; c < std::max(1, track.inputs.size()); ++c) {
                combo(cs::newInput + " " + String(c + 1), line(), options,
                      std::max(0, options.indexOf(routes[c])), [this, c](int i) {
                          auto& ports = remapped[(size_t)remapIndex];
                          while (ports.size() <= c)
                              ports.add({});
                          ports.set(c, i > 0 ? link.caps.inputs[i - 1] : String{});
                      });
            }
        }
    }
    auto actions = line();
    button(
        cs::apply, actions.removeFromLeft(s::clipWidth),
        [this] {
            for (size_t i = 0; i < song.tracks.size(); ++i) {
                auto& ports = remapped[i];
                if (ports.size() > 1 && ports[0].isNotEmpty() && ports[0] == ports[1]) {
                    status(cs::missingInputs);
                    return;
                }
            }
            for (size_t i = 0; i < song.tracks.size(); ++i) {
                auto ports = remapped[i];
                ports.removeEmptyStrings();
                song.tracks[i].inputs = ports;
            }
            saveSong();
            show(Page::project);
        },
        1);
    button(cs::remapLater, actions.removeFromLeft(s::nameWidth), [this] { show(Page::project); });
    text(message, line(), s::body, s::warning);
}
String Studio::rigReport() const {
    StringArray lines{
        cs::app + " · " + cs::checkRig,
        Time::getCurrentTime().toISO8601(true),
        SystemStats::getOperatingSystemName(),
        cs::reportCPU + " · " + SystemStats::getCpuModel() + " / " + String(SystemStats::getNumCpus()),
        cs::driver + " · " +
            (link.online ? String(cs::drivers[(int)link.current.driver]) : cs::disconnected),
        cs::reportPorts + " · " + link.current.input + " / " + link.caps.inputs.joinIntoString(", "),
        cs::reportOutput + " · " + link.current.output,
        cs::frequency(link.current.rate) + " / " + cs::bufferSize(link.current.buffer),
        cs::delay + " · " + latencyLabel(),
        cs::xruns + " · " + (devices.xruns() < 0 ? cs::unreported : String(devices.xruns()))};
    StringArray midi;
    for (auto& p : midiPorts)
        midi.add(p.name);
    lines.add(cs::reportMidi + " · " + (midi.isEmpty() ? cs::noPort : midi.joinIntoString(", ")));
    if (link.caps.separateClocks)
        lines.add(cs::clockConflict);
    const auto calibration = settings.latency.find(Settings::calibrationKey(link.current));
    lines.add(cs::measured + " · " +
              (calibration == settings.latency.end()
                   ? cs::notMeasured
                   : String(calibration->second.samples) + cs::samples));
    lines.add(cs::reportObserved + " · " + cs::manualPending);
    if (syncedFolder(libraryRoot))
        lines.add(cs::syncPath);
    return lines.joinIntoString("\r\n");
}
void Studio::saveReport() {
    fileChooser = std::make_unique<FileChooser>(
        cs::report, libraryRoot.getSiblingFile("Rocker-2-zprava.txt"), "*.txt");
    fileChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles |
                                 FileBrowserComponent::warnAboutOverwriting,
                             [self = Component::SafePointer<Studio>(this)](const FileChooser& chooser) {
                                 if (!self || chooser.getResult() == File{})
                                     return;
                                 auto path = chooser.getResult().withFileExtension(".txt");
                                 TemporaryFile tmp(path);
                                 bool ok = tmp.getFile().replaceWithText(self->rigReport()) &&
                                           commitTemporary(tmp);
                                 self->status(ok ? cs::reportSaved : cs::reportFailed);
                             });
}
} // namespace rocker
