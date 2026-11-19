#include "storage/AtomicFile.h"
#include "Song.h"
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
bool safeWave(const String& p) {
    return p.startsWith("Zvuk/") && p.substring(5).isNotEmpty() &&
           !p.substring(5).containsAnyOf("/\\:") && p.endsWithIgnoreCase(".wav");
}
} // namespace
Frame Song::end() const {
    Frame end = grid.beat(Frame(bars) * grid.beats);
    for (auto& c : clips)
        end = std::max(end, c.position + c.length);
    return end;
}
var Song::json() const {
    Array<var> ts, cs, files, marks;
    for (auto& t : tracks)
        ts.add(object({{"id", t.id},
                       {"nazev", t.name},
                       {"druh", t.kind},
                       {"vstup", object({{"nazvy", var(t.inputs)}})},
                       {"nahravat", t.armed},
                       {"amp", t.kind == "kytara" ? t.amp.json() : var()},
                       {"mix", object({{"mute", t.mute}, {"solo", t.solo}})}}));
    for (auto& c : clips)
        cs.add(object({{"id", c.id},
                       {"trackId", c.track},
                       {"assetId", c.asset},
                       {"position", c.position},
                       {"sourceStart", c.offset},
                       {"length", c.length}}));
    for (auto& a : assets)
        files.add(object({{"id", a.id},
                          {"soubor", a.path},
                          {"kanaly", a.channels},
                          {"delka", a.length},
                          {"sampleRate", grid.rate},
                          {"vznik", "nahravka"}}));
    for (auto m : dropouts)
        marks.add(m);
    auto section = object({{"id", "s1"},
                           {"nazev", "Sloka"},
                           {"barva", "sloka"},
                           {"delkaTaktu", bars},
                           {"klipy", cs},
                           {"pokusy", Array<var>{}}});
    return object(
        {{"schemaVersion", 1},
         {"nazev", title},
         {"sampleRate", grid.rate},
         {"tempoBpm", grid.bpm},
         {"metrum", object({{"dobyVTaktu", grid.beats}, {"jednotka", grid.unit}})},
         {"ladeni", object({{"id", tuning}, {"komorniA", 440}})},
         {"stopy", ts},
         {"dily", Array<var>{section}},
         {"poradi",
          Array<var>{
              object({{"id", "i1"}, {"sectionId", "s1"}, {"cropStartBars", 0}, {"lengthBars", bars}})}},
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
    s.grid.bpm = field(v, "tempoBpm", 112.);
    auto meter = field(v, "metrum");
    s.grid.beats = field(meter, "dobyVTaktu", 4);
    s.grid.unit = field(meter, "jednotka", 4);
    if (s.title.isEmpty() || s.grid.rate < 8000 || s.grid.rate > 384000 || !std::isfinite(s.grid.bpm) ||
        s.grid.bpm < 30 || s.grid.bpm > 300 || s.grid.beats < 1 || s.grid.beats > 16 ||
        (s.grid.unit != 2 && s.grid.unit != 4 && s.grid.unit != 8 && s.grid.unit != 16))
        return {};
    s.tuning = field(field(v, "ladeni"), "id", "Standard").toString();
    std::set<String> tracks, assets, clips;
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
        x.mute = field(field(t, "mix"), "mute");
        x.solo = field(field(t, "mix"), "solo");
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
    auto sections = items(v, "dily");
    if ((int)version == 1 && sections.size() != 1)
        return {};
    auto section = sections.getFirst();
    s.bars = jlimit(1, 1000000, (int)field(section, "delkaTaktu", 8));
    for (auto& c : items(section, "klipy")) {
        Clip x{field(c, "id").toString(), field(c, "trackId").toString(), field(c, "assetId").toString(),
               field(c, "position"),      field(c, "sourceStart"),        field(c, "length")};
        auto asset =
            std::find_if(s.assets.begin(), s.assets.end(), [&](auto& a) { return a.id == x.asset; });
        if (x.id.isEmpty() || !tracks.contains(x.track) || asset == s.assets.end() || x.position < 0 ||
            x.position > Frame(s.grid.rate) * 86400 * 30 || x.length <= 0 || x.offset < 0 ||
            x.offset > asset->length || x.length > asset->length - x.offset ||
            !clips.insert(x.id).second) {
            damaged = true;
            continue;
        }
        s.clips.push_back(x);
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
                                                                {"vyberOd", selectionStart},
                                                                {"vyberDo", selectionEnd},
                                                                {"prvniTakt", firstBar},
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
            field(v, "vybranyKlip").toString()};
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
