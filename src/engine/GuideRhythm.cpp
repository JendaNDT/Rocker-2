#include "GuideRhythm.h"

namespace rocker {
GuideRhythm::GuideRhythm(const BeatGrid& grid, Frame start)
    : steps(grid), eighths(grid.beats == 4 && grid.unit == 4) {
    if (eighths)
        steps.bpm *= 2;
    kit.prepare(grid.rate);
    seek(start);
}
void GuideRhythm::seek(Frame position) noexcept {
    kit.reset();
    index = steps.beatAt(position);
    nextHit = steps.beat(index);
    if (nextHit < position)
        nextHit = steps.beat(++index);
}
float GuideRhythm::sample(Frame position) noexcept {
    if (position == nextHit) {
        if (eighths) {
            const auto slot = index % 8;
            if (slot == 0 || slot == 4)
                kit.trigger(0, 36, 92);
            if (slot == 2 || slot == 6)
                kit.trigger(0, 38, 84);
            kit.trigger(0, 42, 57);
        } else {
            const auto beat = index % steps.beats;
            if (beat == 0)
                kit.trigger(0, 36, 92);
            if (beat + 1 == steps.beats)
                kit.trigger(0, 38, 84);
            kit.trigger(0, 42, 57);
        }
        nextHit = steps.beat(++index);
    }
    return .24f * kit.sample();
}
} // namespace rocker
