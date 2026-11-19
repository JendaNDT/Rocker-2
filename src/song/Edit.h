#pragma once
#include "Song.h"

namespace rocker {
struct Span {
    Frame from = 0, to = 0;
    bool valid() const { return from >= 0 && to > from; }
    Frame length() const { return to - from; }
};
struct Fragment {
    Frame length = 0;
    std::vector<Clip> clips;
    juce::StringArray sourceTracks;
};
// Edits address source frames. Only the named track and intersecting material change.
Fragment copySpan(const Song&, const juce::String& track, Span, const juce::String& clipId = {});
bool eraseSpan(Song&, const juce::String& track, Span, const juce::String& clipId = {});
bool splitAt(Song&, const juce::String& track, Frame pos, const juce::String& clipId = {});
bool pasteAt(Song&, const juce::String& track, Frame pos, const Fragment&);
bool moveSpan(Song&, const juce::String& track, Span, Frame target, const juce::String& clipId = {});
std::optional<Span> trimEdge(Song&, const juce::String& clipId, bool left, Frame target);
Frame snapBeat(const BeatGrid&, Frame);
class EditHistory {
  public:
    void reset(const Song&);
    bool remember(const Song&, const juce::String& action);
    juce::String step(Song&, bool forward);
    bool canStep(bool forward) const;
    bool matches(const Song& s) const { return !states.empty() && states[index].song == s; }

  private:
    struct State {
        Song song;
        juce::String action;
    };
    std::vector<State> states;
    size_t index = 0;
};
} // namespace rocker
