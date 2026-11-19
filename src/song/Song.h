#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>
#include <optional>
#include <vector>

namespace rocker {
using Frame = juce::int64;
struct BeatGrid {
    int rate = 48000, beats = 4, unit = 4;
    double bpm = 112;
    Frame beat(Frame index) const;
    Frame beatAt(Frame sample) const;
};
struct Track {
    juce::String id, name, kind;
    juce::StringArray inputs;
    bool armed = false, mute = false, solo = false;
};
struct Clip {
    juce::String id, track, asset;
    Frame position = 0, offset = 0, length = 0;
};
struct Asset {
    juce::String id, path;
    Frame length = 0;
    int channels = 1;
};
struct Song {
    juce::String title, tuning = "Standard";
    BeatGrid grid;
    std::vector<Track> tracks;
    std::vector<Clip> clips;
    std::vector<Asset> assets;
    std::vector<Frame> dropouts;
    int bars = 8;
    Frame end() const;
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
    bool save(const juce::File&) const;
    static ViewState load(const juce::File&);
};
juce::var object(std::initializer_list<std::pair<juce::Identifier, juce::var>>);
std::unique_ptr<juce::AudioFormatWriter> waveWriter(const juce::File&, int rate, int channels);
std::unique_ptr<juce::AudioFormatReader> waveReader(const juce::File&);
} // namespace rocker
