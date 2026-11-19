#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace rocker {
struct VoiceParameters {
    std::uint8_t low = 12, mid = 12, high = 12, compression = 0, room = 0;
    constexpr std::uint64_t pack() const noexcept {
        return std::uint64_t(low) | std::uint64_t(mid) << 8 | std::uint64_t(high) << 16 |
               std::uint64_t(compression) << 24 | std::uint64_t(room) << 32;
    }
    static constexpr VoiceParameters unpack(std::uint64_t word) noexcept {
        return {std::uint8_t(word), std::uint8_t(word >> 8), std::uint8_t(word >> 16),
                std::uint8_t(word >> 24), std::uint8_t(word >> 32)};
    }
    bool operator==(const VoiceParameters&) const = default;
};
inline constexpr std::array<VoiceParameters, 3> voicePresets{{{}, {9, 14, 15, 55, 18}, {15, 14, 9, 47, 6}}};
struct VoiceSound {
    VoiceParameters values;
    int preset = 0;
    bool operator==(const VoiceSound&) const = default;
};
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
class VoiceCommand {
  public:
    static constexpr std::uint64_t enabled = std::uint64_t{1} << 63;
    static constexpr std::uint64_t reset = std::uint64_t{1} << 62;
    void publish(const VoiceSound& sound, bool recall) noexcept;
    std::uint64_t consume() noexcept;

  private:
    std::atomic<std::uint64_t> word{voicePresets[0].pack()};
};

class Voice {
  public:
    void prepare(int sampleRate);
    void configure(VoiceParameters) noexcept;
    void reset() noexcept;
    float process(float input) noexcept;

  private:
    struct Reflection {
        std::vector<float> ring;
        size_t index = 0, valid = 0;
        float process(float input) noexcept;
        void reset() noexcept { index = valid = 0; }
    };
    struct Gains {
        float low = 1, mid = 1, high = 1, compression = 0, room = 0;
    } target, current;
    std::array<Reflection, 3> reflections;
    float low = 0, wide = 0, detector = 0;
    float lowPole = 0, widePole = 0, attack = 0, release = 0, smoothing = 0;
};
} // namespace rocker
