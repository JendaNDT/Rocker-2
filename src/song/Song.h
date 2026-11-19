#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include "GuitarSound.h"
#include "dsp/Voice.h"
#include <map>
#include <optional>
#include <vector>

namespace rocker {
using Frame = juce::int64;
struct BeatGrid {
    bool operator==(const BeatGrid&) const = default;
    int rate = 48000, beats = 4, unit = 4;
    double bpm = 112;
    Frame beat(Frame index) const;
    Frame beatAt(Frame sample) const;
};
struct Track {
    bool operator==(const Track&) const = default;
    juce::String id, name, kind;
    juce::StringArray inputs;
    bool armed = false, mute = false, solo = false;
    GuitarSound amp;
    float gainDb = 0, pan = 0;
    Frame inputOver = -1;
    juce::String group;
    float groupDb = 0;
    int kit = 0;
    VoiceSound voice;
};
struct Clip {
    bool operator==(const Clip&) const = default;
    juce::String id, track, asset;
    Frame position = 0, offset = 0, length = 0;
};
struct Attempt {
    bool operator==(const Attempt&) const = default;
    juce::String id;
    Frame created = 0;
    bool discarded = false;
    std::vector<Clip> clips;
};
struct TakeGroup {
    bool operator==(const TakeGroup&) const = default;
    juce::String id, track;
    Frame from = 0, to = 0;
    std::vector<Attempt> attempts;
    int active = 0; // Zero selects the original track material.
    int cycle() const;
    bool discard();
};
struct Asset {
    bool operator==(const Asset&) const = default;
    juce::String id, path;
    Frame length = 0;
    int channels = 1;
};
struct Section {
    bool operator==(const Section&) const = default;
    juce::String id, name, colour;
    int bars = 1;
    std::vector<Clip> clips;
    std::vector<TakeGroup> groups;
};
struct Occurrence {
    bool operator==(const Occurrence&) const = default;
    juce::String id, section;
    int crop = 0, length = 1;
};
struct Song {
    bool operator==(const Song&) const = default;
    juce::String title, tuning = "Standard";
    BeatGrid grid;
    std::vector<Track> tracks;
    std::vector<Clip> clips;
    std::vector<Asset> assets;
    std::vector<TakeGroup> groups;
    TakeGroup* groupAt(const juce::String& track, Frame from, Frame to);
    bool addAttempt(Clip pass, Frame from, Frame to, Frame created, bool discarded = false);
    std::vector<Frame> dropouts;
    int bars = 8;
    float masterDb = 0;
    std::map<juce::String, Section> sections;
    std::vector<Occurrence> order;
    Frame barFrame(int bar) const { return grid.beat(Frame(bar) * grid.beats); }
    Frame translate(Frame value, int fromBar, int toBar) const;
    int startBar(int index) const;
    int occurrenceAt(int bar) const;
    bool linked(int index) const;
    void initOrder();
    void projectOrder();
    bool splitSection(int bar);
    bool repeatSection(int index, int insert);
    bool detachSection(int index);
    bool resizeOccurrence(int index, int length);
    void commitClips();
    void commitSingleGroup(const TakeGroup&);
    void commitGroup(const TakeGroup&, Frame previousFrom = -1, Frame previousTo = -1);
    void extendOrder(Frame);
    Frame end() const;
    bool linkedTrack(const juce::String&, const juce::String&) const;
    juce::StringArray members(const juce::String&) const;
    juce::var json() const;
    static std::optional<Song> parse(const juce::var&, bool& damaged);
    bool save(const juce::File&) const;
    static std::optional<Song> load(const juce::File&, bool& damaged);
    static juce::File latestVersion(const juce::File&);
    bool archive(const juce::File&) const;
};
struct ViewState {
    Frame cursor = 0;
    int selected = 0, zoom = 8;
    bool distant = false, click = true, countIn = true;
    bool editing = false;
    Frame selectionStart = 0, selectionEnd = 0, firstBar = 0;
    juce::String selectedClip;
    int leadBars = 2, tailBars = 1;
    bool mixing = false;
    int targetBars = 0, savedTakes = 0;
    bool rhythm = false, playLoop = false;
    bool save(const juce::File&) const;
    static ViewState load(const juce::File&);
};
juce::var object(std::initializer_list<std::pair<juce::Identifier, juce::var>>);
std::unique_ptr<juce::AudioFormatWriter> waveWriter(const juce::File&, int rate, int channels);
std::unique_ptr<juce::AudioFormatReader> waveReader(const juce::File&);
} // namespace rocker
