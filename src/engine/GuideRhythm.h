#pragma once
#include "MidiDrums.h"

namespace rocker {
class GuideRhythm {
  public:
    GuideRhythm(const BeatGrid& grid, Frame start);
    float sample(Frame position) noexcept;
    void seek(Frame position) noexcept;
  private:
    BeatGrid steps;
    bool eighths;
    DrumPlayer kit;
    Frame index = 0, nextHit = 0;
};
} // namespace rocker
