#pragma once
#include <array>
#include <span>
namespace rocker {
class Pitch {
  public:
    // Input is low-pass averaged to about 12 kHz by the device callback.
    double detect(std::span<const float> samples, double rate) const noexcept;
    static double frequency(int midi) noexcept;
    static double cents(double hz, int note) noexcept;
};
} // namespace rocker
