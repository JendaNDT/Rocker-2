#include "storage/AtomicFile.h"
#include "Song.h"
#include "SectionLabels.h"
#include <cmath>
#include <set>
namespace rocker {
using namespace juce;
Frame BeatGrid::beat(Frame index) const {
    return std::llround(index * (60.0 * rate * 4 / (bpm * unit)));
}
Frame BeatGrid::beatAt(Frame sample) const {
    Frame index = static_cast<Frame>(std::floor(sample / (60.0 * rate * 4 / (bpm * unit))));
    if (beat(index + 1) <= sample)
        return index + 1;
    return beat(index) > sample ? index - 1 : index;
}
var object(std::initializer_list<std::pair<Identifier, var>> fields) {
    auto* out = new DynamicObject;
    var result(out);
    for (const auto& [key, value] : fields)
        out->setProperty(key, value);
    return result;
}
namespace {
var field(const var& v, const char* key, var fallback = {}) { return v.getProperty(key, fallback); }
Array<var> items(const var& v, const char* key) {
    auto a = field(v, key);
    if (auto* array = a.getArray())
        return *array;
    return {};
}
var clipJson(const Clip& c) {
    return object({{"id", c.id},
                   {"trackId", c.track},
                   {"assetId", c.asset},
                   {"position", c.position},
                   {"sourceStart", c.offset},
                   {"length", c.length}});
}
bool safeWave(const String& p) {
    return p.startsWith("Zvuk/") && p.substring(5).isNotEmpty() &&
           !p.substring(5).containsAnyOf("/\\:") && (p.endsWithIgnoreCase(".wav") || p.endsWithIgnoreCase(".mid"));
}
} // namespace
int TakeGroup::cycle() const {
    int result = active;
    do {
        result = (result + 1) % (1 + (int)attempts.size());
    } while (result && attempts[result - 1].discarded);
    return result;
}
bool TakeGroup::discard() {
    if (!active)
        return false;
    attempts[active - 1].discarded = true;
    active = cycle();
    return true;
}
TakeGroup* Song::groupAt(const String& track, Frame from, Frame to) {
    for (auto& group : groups)
        if (group.track == track && group.from == from && group.to == to)
            return &group;
    return nullptr;
}
bool Song::addAttempt(Clip pass, Frame from, Frame to, Frame created, bool discarded) {
    if (from < 0 || to <= from || pass.position > from || pass.length < to - pass.position)
        return false;
    if (!order.empty()) {
        extendOrder(to);
        int bar = 0;
        for (const auto& o : order) {
            int startBar = bar;
            Frame start = barFrame(startBar);
            bar += o.length;
            Frame a = std::max(start, from), b = std::min(barFrame(bar), to);
            if (b <= a)
                continue;
            Frame shift = barFrame(o.crop) - start;
            a = translate(a, startBar, o.crop);
            b = translate(b, startBar, o.crop);
            auto& local = sections.at(o.section).groups;
            auto it = std::find_if(local.begin(), local.end(), [&](auto& g) {
                return g.track == pass.track && g.from == a && g.to == b;
            });
            if (it == local.end()) {
                local.push_back({Uuid().toString(), pass.track, a, b});
                it = std::prev(local.end());
            }
            auto copy = pass;
            copy.id = Uuid().toString();
            copy.position += shift;
            it->attempts.push_back({Uuid().toString(), created, discarded, {copy}});
            if (!discarded)
                it->active = (int)it->attempts.size();
        }
        projectOrder();
        return true;
    }
    auto* group = groupAt(pass.track, from, to);
    if (!group)
        group = &groups.emplace_back(TakeGroup{Uuid().toString(), pass.track, from, to});
    group->attempts.push_back({Uuid().toString(), created, discarded, {std::move(pass)}});
    if (!discarded)
        group->active = (int)group->attempts.size();
    return true;
}
StringArray Song::members(const String& id) const {
    auto track = std::find_if(tracks.begin(), tracks.end(), [&](auto& t) { return t.id == id; });
    if (track == tracks.end() || track->group.isEmpty())
        return {id};
    StringArray list;
    for (auto& t : tracks)
        if (t.group == track->group)
            list.add(t.id);
    return list;
}
bool Song::linkedTrack(const String& a, const String& b) const {
    return a == b || members(a).contains(b);
}
Frame Song::end() const {
    Frame end = grid.beat(Frame(bars) * grid.beats);
    for (auto& c : clips)
        end = std::max(end, c.position + c.length);
    for (const auto& group : groups)
        end = std::max(end, group.to);
    return end;
}
var Song::json() const {
    Array<var> ts, files, marks;
    for (auto& t : tracks)
        ts.add(object({{"id", t.id},
                       {"nazev", t.name},
                       {"druh", t.kind},
                       {"vstup", object({{"nazvy", var(t.inputs)}})},
                       {"nahravat", t.armed},
                       {"skupina", t.group},
                       {"skupinaDb", t.groupDb},{"sadaMidi",t.kit},
                       {"amp", t.kind == "kytara" ? t.amp.json() : var()},
                       {"mix", object({{"mute", t.mute},
                                       {"solo", t.solo},
                                       {"gainDb", t.gainDb},
                                       {"pan", t.pan},
                                       {"vstupPres", t.inputOver}})}}));
    auto encode = [](const Section& sec) {
        Array<var> cs;
        for (auto& c : sec.clips)
            cs.add(clipJson(c));
        Array<var> groupsJson;
        for (const auto& group : sec.groups) {
            Array<var> attempts;
            for (const auto& attempt : group.attempts) {
                Array<var> recorded;
                for (const auto& clip : attempt.clips)
                    recorded.add(clipJson(clip));
                attempts.add(object({{"id", attempt.id},
                                     {"vznik", attempt.created},
                                     {"kos", attempt.discarded},
                                     {"klipy", recorded}}));
            }
            groupsJson.add(object({{"id", group.id},
                                   {"stopa", group.track},
                                   {"od", group.from},
                                   {"do", group.to},
                                   {"aktivni", group.active},
                                   {"varianty", attempts}}));
        }
        return object({{"id", sec.id},
                       {"nazev", sec.name},
                       {"barva", sec.colour},
                       {"delkaTaktu", sec.bars},
                       {"klipy", cs},
                       {"pokusy", groupsJson}});
    };
    Array<var> definitions, sequence;
    if (order.empty()) {
        definitions.add(encode({"s1", cs::verseName, "sloka", bars, clips, groups}));
        sequence.add(
            object({{"id", "i1"}, {"sectionId", "s1"}, {"cropStartBars", 0}, {"lengthBars", bars}}));
    } else {
        for (const auto& [id, section] : sections)
            definitions.add(encode(section));
        for (const auto& o : order)
            sequence.add(object({{"id", o.id},
                                 {"sectionId", o.section},
                                 {"cropStartBars", o.crop},
                                 {"lengthBars", o.length}}));
    }
    for (auto& a : assets)
        files.add(object({{"id", a.id},
                          {"soubor", a.path},
                          {"kanaly", a.channels},
                          {"delka", a.length},
                          {"sampleRate", grid.rate},
                          {"vznik", "nahravka"}}));
    for (auto m : dropouts)
        marks.add(m);
    return object({{"schemaVersion", 1},
                   {"nazev", title},
                   {"sampleRate", grid.rate},
                   {"tempoBpm", grid.bpm},
                   {"masterDb", masterDb},
                   {"metrum", object({{"dobyVTaktu", grid.beats}, {"jednotka", grid.unit}})},
                   {"ladeni", object({{"id", tuning}, {"komorniA", 440}})},
                   {"stopy", ts},
                   {"dily", definitions},
                   {"poradi", sequence},
                   {"usporadano", !order.empty()},
                   {"zvuky", files},
                   {"vypadky", marks}});
}
std::optional<Song> Song::parse(const var& v, bool& damaged) {
    damaged = false;
    auto version = field(v, "schemaVersion", 0);
    if (!v.isObject() || !version.isInt() || (int)version < 0 || (int)version > 1)
        return {};
    Song s;
    s.title = field(v, "nazev").toString().trim();
    s.grid.rate = field(v, "sampleRate", 48000);
    s.masterDb = jlimit(-96.f, 6.f, (float)field(v, "masterDb", 0));
    s.grid.bpm = field(v, "tempoBpm", 112.);
    auto meter = field(v, "metrum");
    s.grid.beats = field(meter, "dobyVTaktu", 4);
    s.grid.unit = field(meter, "jednotka", 4);
    if (s.title.isEmpty() || s.grid.rate < 8000 || s.grid.rate > 384000 || !std::isfinite(s.grid.bpm) ||
        s.grid.bpm < 30 || s.grid.bpm > 300 || s.grid.beats < 1 || s.grid.beats > 16 ||
        (s.grid.unit != 2 && s.grid.unit != 4 && s.grid.unit != 8 && s.grid.unit != 16))
        return {};
    s.tuning = field(field(v, "ladeni"), "id", "Standard").toString();
    std::set<String> tracks, assets, legacyMix;
    for (auto& t : items(v, "stopy")) {
        Track x{field(t, "id").toString(), field(t, "nazev").toString(),
                field(t, "druh", "jine").toString()};
        if (x.id.isEmpty() || !tracks.insert(x.id).second)
            return {};
        for (auto& n : items(field(t, "vstup"), "nazvy"))
            if (n.isString() && x.inputs.size() < 2)
                x.inputs.add(n.toString());
        x.amp = GuitarSound::read(field(t, "amp"));
        x.armed = field(t, "nahravat");
        x.kit=jlimit(0,1,(int)field(t,"sadaMidi",0));
        x.group = field(t, "skupina").toString();
        x.groupDb = jlimit(-96.f, 6.f, (float)field(t, "skupinaDb", 0));
        x.mute = field(field(t, "mix"), "mute");
        x.solo = field(field(t, "mix"), "solo");
        auto mix = field(t, "mix");
        if (field(mix, "gainDb").isVoid())
            legacyMix.insert(x.id);
        x.gainDb = jlimit(-96.f, 6.f, (float)field(mix, "gainDb", 0));
        x.pan = jlimit(-1.f, 1.f, (float)field(mix, "pan", 0));
        x.inputOver = field(mix, "vstupPres", -1);
        s.tracks.push_back(x);
    }
    for (auto& a : items(v, "zvuky")) {
        Asset x{field(a, "id").toString(), field(a, "soubor").toString(), field(a, "delka"),
                field(a, "kanaly", 1)};
        if (x.id.isEmpty() || !safeWave(x.path) || x.length < 0 || x.channels < 1 || x.channels > 2 ||
            !assets.insert(x.id).second) {
            damaged = true;
            continue;
        }
        s.assets.push_back(x);
    }
    auto definitions = items(v, "dily"), sequence = items(v, "poradi");
    if ((int)version == 1 && definitions.isEmpty())
        return {};
    bool arranged = (bool)field(v, "usporadano") || definitions.size() > 1 || sequence.size() > 1;
    if (definitions.size() == 1 && sequence.size() == 1) {
        const auto& d = definitions.getFirst();
        const auto& o = sequence.getFirst();
        arranged |= field(d, "id").toString() != "s1" || field(d, "barva").toString() != "sloka" ||
                    field(d, "nazev").toString() != "Sloka" || (int)field(o, "cropStartBars") != 0 ||
                    (int)field(o, "lengthBars") != (int)field(d, "delkaTaktu");
    }
    for (const auto& section : definitions) {
        std::set<String> clips;
        Section sec{field(section, "id").toString(), field(section, "nazev").toString(),
                    field(section, "barva").toString(), field(section, "delkaTaktu", 8)};
        if (sec.id.isEmpty() || sec.id.containsChar('/') || sec.bars < 1 || sec.bars > 1000000 ||
            s.sections.contains(sec.id))
            return {};
        auto parseClip = [&](const var& c, bool pass = false) -> std::optional<Clip> {
            Clip x{field(c, "id").toString(),      field(c, "trackId").toString(),
                   field(c, "assetId").toString(), field(c, "position"),
                   field(c, "sourceStart"),        field(c, "length")};
            auto asset =
                std::find_if(s.assets.begin(), s.assets.end(), [&](auto& a) { return a.id == x.asset; });
            if (x.id.isEmpty() || !tracks.contains(x.track) || asset == s.assets.end() ||
                (x.position < 0 && !pass) || x.position < -Frame(s.grid.rate) * 86400 * 30 ||
                x.position > Frame(s.grid.rate) * 86400 * 30 || x.length <= 0 || x.offset < 0 ||
                x.offset > asset->length || x.length > asset->length - x.offset ||
                !clips.insert(x.id).second) {
                damaged = true;
                return {};
            }
            return x;
        };
        for (auto& c : items(section, "klipy"))
            if (auto clip = parseClip(c))
                sec.clips.push_back(*clip);
        std::set<String> groupIds, attemptIds;
        for (const auto& raw : items(section, "pokusy")) {
            TakeGroup g{field(raw, "id").toString(), field(raw, "stopa").toString(), field(raw, "od", -1),
                        field(raw, "do", -1)};
            if (g.id.isEmpty() || !groupIds.insert(g.id).second || !tracks.contains(g.track) ||
                g.from < 0 || g.to <= g.from || g.to > Frame(s.grid.rate) * 86400 * 30) {
                damaged = true;
                continue;
            }
            for (const auto& entry : items(raw, "varianty")) {
                Attempt attempt{field(entry, "id").toString(), field(entry, "vznik"),
                                field(entry, "kos")};
                for (auto& c : items(entry, "klipy"))
                    if (auto clip = parseClip(c, true); clip && clip->track == g.track)
                        attempt.clips.push_back(*clip);
                if (attempt.clips.empty() || attempt.id.isEmpty() ||
                    !attemptIds.insert(attempt.id).second) {
                    damaged = true;
                    continue;
                }
                g.attempts.push_back(std::move(attempt));
            }
            g.active = field(raw, "aktivni", 0);
            if (g.active < 0 || g.active > (int)g.attempts.size() ||
                (g.active && g.attempts[g.active - 1].discarded)) {
                damaged = true;
                g.active = 0;
            }
            sec.groups.push_back(std::move(g));
        }
        if (arranged)
            s.sections.emplace(sec.id, sec);
        else {
            s.clips = std::move(sec.clips);
            s.groups = std::move(sec.groups);
            s.bars = sec.bars;
        }
    }
    if (arranged) {
        std::set<String> ids;
        int total = 0;
        for (const auto& raw : sequence) {
            Occurrence o{field(raw, "id").toString(), field(raw, "sectionId").toString(),
                         field(raw, "cropStartBars", 0), field(raw, "lengthBars", 0)};
            auto def = s.sections.find(o.section);
            if (o.id.isEmpty() || o.id.containsChar('/') || !ids.insert(o.id).second ||
                def == s.sections.end() || o.crop < 0 || o.length < 1 ||
                o.length > def->second.bars - o.crop || o.length > 1000000 - total)
                return {};
            total += o.length;
            s.order.push_back(std::move(o));
        }
        if (s.order.empty())
            return {};
        s.projectOrder();
    }
    // Earlier versions duplicated mono audio at unity into each speaker.
    std::set<String> stereoAssets, recorded, stereoTracks;
    for (const auto& asset : s.assets)
        if (asset.channels == 2)
            stereoAssets.insert(asset.id);
    auto inspect = [&](const std::vector<Clip>& clips) {
        for (auto& clip : clips) {
            recorded.insert(clip.track);
            if (stereoAssets.contains(clip.asset))
                stereoTracks.insert(clip.track);
        }
    };
    if (!legacyMix.empty()) {
        inspect(s.clips);
        for (auto& group : s.groups)
            for (auto& attempt : group.attempts)
                inspect(attempt.clips);
        for (auto& [id, section] : s.sections) {
            inspect(section.clips);
            for (auto& group : section.groups)
                for (auto& attempt : group.attempts)
                    inspect(attempt.clips);
        }
        for (auto& track : s.tracks)
            if (legacyMix.contains(track.id) && recorded.contains(track.id) && track.inputs.size() < 2 &&
                !stereoTracks.contains(track.id))
                track.gainDb = 3.0103f;
    }
    for (auto m : items(v, "vypadky"))
        if ((Frame)m >= 0)
            s.dropouts.push_back((Frame)m);
    return s;
}
bool Song::save(const File& dir) const { return atomicJson(dir.getChildFile("skladba.json"), json()); }
std::optional<Song> Song::load(const File& dir, bool& damaged) {
    return parse(JSON::parse(dir.getChildFile("skladba.json")), damaged);
}
File Song::latestVersion(const File& dir) {
    auto files = dir.getChildFile("Verze").findChildFiles(File::findFiles, false, "*.json");
    files.sort();
    for (int i = files.size(); --i >= 0;) {
        bool bad = false;
        if (parse(JSON::parse(files[i]), bad) && !bad)
            return files[i];
    }
    return {};
}
bool Song::archive(const File& dir) const {
    auto versions = dir.getChildFile("Verze");
    auto name = String(Time::currentTimeMillis()) + "-" + Uuid().toString() + ".json";
    if (!atomicJson(versions.getChildFile(name), json()))
        return false;
    auto files = versions.findChildFiles(File::findFiles, false, "*.json");
    files.sort();
    for (int i = 0; i < files.size() - 50; ++i)
        files[i].deleteFile();
    return true;
}
bool ViewState::save(const File& d) const {
    return atomicJson(d.getChildFile("zobrazeni.json"), object({{"ukazatel", cursor},
                                                                {"stopa", selected},
                                                                {"takty", zoom},
                                                                {"zdalky", distant},
                                                                {"klik", click},
                                                                {"odpocet", countIn},
                                                                {"strihej", editing},
                                                                {"michej", mixing},
                                                                {"cilTaktu", targetBars},
                                                                {"ulozenePokusy", savedTakes},
                                                                {"vyberOd", selectionStart},
                                                                {"vyberDo", selectionEnd},
                                                                {"prvniTakt", firstBar},
                                                                {"predstih", leadBars},
                                                                {"dohra", tailBars},
                                                                {"vybranyKlip", selectedClip}}));
}
ViewState ViewState::load(const File& d) {
    auto v = JSON::parse(d.getChildFile("zobrazeni.json"));
    return {std::max<Frame>(0, field(v, "ukazatel")),
            std::max(0, (int)field(v, "stopa")),
            jlimit(1, 64, (int)field(v, "takty", 8)),
            field(v, "zdalky", false),
            field(v, "klik", true),
            field(v, "odpocet", true),
            field(v, "strihej", false),
            std::max<Frame>(0, field(v, "vyberOd")),
            std::max<Frame>(0, field(v, "vyberDo")),
            juce::jlimit<Frame>(0, 999999, field(v, "prvniTakt")),
            field(v, "vybranyKlip").toString(),
            jlimit(0, 8, (int)field(v, "predstih", 2)),
            jlimit(0, 8, (int)field(v, "dohra", 1)),
            (bool)field(v, "michej", false),
            jlimit(0, 999, (int)field(v, "cilTaktu", 0)),
            std::max(0, (int)field(v, "ulozenePokusy", 0))};
}
std::unique_ptr<AudioFormatWriter> waveWriter(const File& file, int rate, int channels) {
    std::unique_ptr<OutputStream> stream = file.createOutputStream();
    if (!stream)
        return {};
    return WavAudioFormat().createWriterFor(
        stream, AudioFormatWriterOptions{}
                    .withSampleRate(rate)
                    .withNumChannels(channels)
                    .withBitsPerSample(32)
                    .withSampleFormat(AudioFormatWriterOptions::SampleFormat::floatingPoint));
}
std::unique_ptr<AudioFormatReader> waveReader(const File& file) {
    auto stream = file.createInputStream();
    if (!stream)
        return {};
    return std::unique_ptr<AudioFormatReader>(WavAudioFormat().createReaderFor(stream.release(), true));
}
} // namespace rocker
