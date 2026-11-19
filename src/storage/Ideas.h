#pragma once
#include "song/Song.h"

namespace rocker {
struct Idea {
    juce::String id, note, tuning;
    juce::int64 created = 0;
    int rate = 48000;
    Frame length = 0, firstBeat = 0;
    double bpm = 0;
    bool star = false;
    std::vector<float> peaks;
};
class Ideas {
  public:
    explicit Ideas(juce::File root);
    void load();
    void recover();
    juce::File wave(const juce::String& id) const;
    bool append(Idea);
    bool update(const Idea&);
    const std::vector<Idea>& all() const { return entries; }
    std::optional<Song> toSong(const Idea&, const juce::File& dir, const juce::String& title,
                               const juce::String& trackName) const;

  private:
    bool save(const std::vector<Idea>&) const;
    juce::File folder;
    std::vector<Idea> entries;
};
struct TappedTempo {
    double bpm;
    Frame firstBeat;
};
std::optional<TappedTempo> measureTaps(const std::vector<Frame>&, int rate);
} // namespace rocker
