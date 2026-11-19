#include "Ideas.h"
#include "AtomicFile.h"

namespace rocker {
using namespace juce;
Ideas::Ideas(File root) : folder(std::move(root)) {
    load();
}
File Ideas::wave(const String& id) const {
    return folder.getChildFile("Zvuk").getChildFile(id + ".wav");
}
void Ideas::load() {
    entries.clear();
    auto v = JSON::parse(folder.getChildFile("napady.json"));
    auto rows = v.getProperty("napady", {});
    if (auto* all = rows.getArray())
        for (auto& row : *all) {
            Idea x;
            x.id = row.getProperty("id", {}).toString();
            if (x.id.length() != 32 || !x.id.containsOnly("0123456789abcdef") ||
                !wave(x.id).existsAsFile())
                continue;
            x.created = row.getProperty("vznik", 0);
            x.rate = row.getProperty("frekvence", 48000);
            x.length = row.getProperty("delka", 0);
            x.note = row.getProperty("poznamka", {}).toString();
            x.tuning = row.getProperty("ladeni", "Standard").toString();
            x.star = row.getProperty("hvezda", false);
            x.bpm = row.getProperty("tempo", 0.);
            x.beats = jlimit(1, 16, int(row.getProperty("dobyVTaktu", 4)));
            const int unit = row.getProperty("jednotka", 4);
            x.unit = unit == 8 ? 8 : 4;
            x.firstBeat = row.getProperty("prvniDoba", 0);
            auto peaks = row.getProperty("nahled", {});
            if (auto* a = peaks.getArray())
                for (const auto& p : *a)
                    if (x.peaks.size() < 96)
                        x.peaks.push_back(jlimit(0.f, 1.f, float(p)));
            if (x.rate >= 8000 && x.rate <= 384000 && x.length > 0)
                entries.push_back(std::move(x));
        }
    std::stable_sort(entries.begin(), entries.end(),
                     [](auto& a, auto& b) { return a.created > b.created; });
}
void Ideas::recover() {
    auto dir = folder.getChildFile("Zvuk");
    for (auto& partial : dir.findChildFiles(File::findFiles, false, "*.partial.wav")) {
        auto id = partial.getFileName().upToLastOccurrenceOf(".partial.wav", false, false);
        if (id.length() != 32 || !id.containsOnly("0123456789abcdef") || wave(id).existsAsFile())
            continue;
        auto readable = waveReader(partial);
        if (readable && readable->lengthInSamples > 0) {
            readable.reset();
            partial.moveFileTo(wave(id));
        }
    }
    for (auto& file : dir.findChildFiles(File::findFiles, false, "*.wav")) {
        auto id = file.getFileNameWithoutExtension();
        if (id.length() != 32 || !id.containsOnly("0123456789abcdef") ||
            std::any_of(entries.begin(), entries.end(), [&](auto& idea) { return idea.id == id; }))
            continue;
        Idea found;
        found.id = id;
        found.created = file.getLastModificationTime().toMilliseconds();
        found.tuning = "Standard";
        append(found);
    }
    load();
}
bool Ideas::save(const std::vector<Idea>& rows) const {
    Array<var> all;
    for (auto& x : rows) {
        Array<var> peaks;
        for (float p : x.peaks)
            peaks.add(p);
        all.add(object({{"id", x.id},
                        {"vznik", x.created},
                        {"frekvence", x.rate},
                        {"delka", x.length},
                        {"poznamka", x.note},
                        {"ladeni", x.tuning},
                        {"hvezda", x.star},
                        {"tempo", x.bpm},
                        {"dobyVTaktu", x.beats},
                        {"jednotka", x.unit},
                        {"prvniDoba", x.firstBeat},
                        {"nahled", peaks}}));
    }
    auto index = folder.getChildFile("napady.json");
    if (index.existsAsFile() && !JSON::parse(index).getProperty("napady", {}).isArray())
        if (!index.copyFileTo(folder.getNonexistentChildFile("napady-zaloha", ".json", true)))
            return false;
    return atomicJson(index, object({{"verze", 1}, {"napady", all}}));
}
bool Ideas::append(Idea x) {
    auto source = waveReader(wave(x.id));
    if (!source || source->lengthInSamples <= 0)
        return false;
    x.rate = roundToInt(source->sampleRate);
    x.length = source->lengthInSamples;
    constexpr int bins = 96;
    // Map the recorded float WAV without loading a second copy of a long take.
    std::unique_ptr<MemoryMappedAudioFormatReader> mapped(
        WavAudioFormat().createMemoryMappedReader(wave(x.id)));
    if (!mapped || !mapped->mapEntireFile())
        return false;
    for (int i = 0; i < bins; ++i) {
        Range<float> level;
        Frame at = x.length * i / bins, to = x.length * (i + 1) / bins;
        mapped->readMaxLevels(at, std::max<Frame>(1, to - at), &level, 1);
        x.peaks.push_back(std::min(1.f, std::max(std::abs(level.getStart()), std::abs(level.getEnd()))));
    }
    auto next = entries;
    next.insert(next.begin(), std::move(x));
    if (!save(next))
        return false;
    entries = std::move(next);
    return true;
}
bool Ideas::update(const Idea& x) {
    auto next = entries;
    for (auto& existing : next)
        if (existing.id == x.id) {
            existing = x;
            if (!save(next))
                return false;
            entries = std::move(next);
            return true;
        }
    return false;
}
bool Ideas::remove(const String& id, const std::function<bool(const File&)>& trash) {
    auto next = entries;
    if (std::erase_if(next, [&](const Idea& entry) { return entry.id == id; }) == 0)
        return false;
    if (!trash(wave(id)))
        return false;
    entries = std::move(next);
    // The loader ignores missing WAVs even if replacing the catalog fails.
    return save(entries);
}
std::optional<Song> Ideas::toSong(const Idea& x, const File& dir, const String& title,
                                  const String& trackName) const {
    Song s;
    s.title = title;
    s.grid.rate = x.rate;
    s.grid.bpm = x.bpm > 0 ? x.bpm : 112;
    s.grid.beats = x.beats;
    s.grid.unit = x.unit;
    s.tuning = x.tuning;
    Track guitar;
    guitar.id = Uuid().toString();
    guitar.name = trackName;
    guitar.kind = "kytara";
    s.tracks = {guitar};
    const String path = "Zvuk/" + x.id + ".wav";
    if (dir.exists())
        return {};
    auto staging = dir.getSiblingFile("." + Uuid().toString());
    struct Remove {
        File dir;
        ~Remove() { dir.deleteRecursively(); }
    } guard{staging};
    auto dest = staging.getChildFile(path);
    if (dest.getParentDirectory().createDirectory().failed())
        return {};
    TemporaryFile copy(dest);
    if (!wave(x.id).copyFileTo(copy.getFile()) || !commitTemporary(copy))
        return {};
    Frame bar = s.barFrame(1);
    Frame offset = x.bpm > 0 ? (bar - x.firstBeat % bar) % bar : 0;
    s.assets = {{x.id, path, x.length, 1}};
    s.clips = {{Uuid().toString(), guitar.id, x.id, offset, 0, x.length}};
    s.bars = std::max(1, int(s.grid.beatAt(offset + x.length - 1) / s.grid.beats) + 1);
    if (!s.save(staging) || dir.exists() || !staging.moveFileTo(dir))
        return {};
    return s;
}
std::optional<TappedTempo> measureTaps(const std::vector<Frame>& taps, int rate) {
    if (taps.size() != 8 || rate <= 0)
        return {};
    for (size_t i = 1; i < taps.size(); ++i)
        if (taps[i] <= taps[i - 1])
            return {};
    double interval = double(taps.back() - taps.front()) / 7;
    double bpm = 60. * rate / interval;
    if (bpm < 30 || bpm > 300)
        return {};
    return TappedTempo{bpm, taps.front()};
}
} // namespace rocker
