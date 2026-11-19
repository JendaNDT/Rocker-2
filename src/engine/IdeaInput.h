#pragma once
#include "song/Song.h"
#include <atomic>
#include <thread>

namespace rocker {
// The audio thread owns the write cursor. Readers can copy an earlier interval without pausing it.
class IdeaInput {
  public:
    explicit IdeaInput(int rate, bool insurance = true);
    void push(const float*, int) noexcept;
    Frame end() const { return published.load(); }
    Frame earliest() const { return std::max(resetAt.load(), end() - Frame(rate) * (history ? 120 : 2)); }
    bool hasHistory() const { return history; }
    void forget() { resetAt.store(end()); }
    bool write(juce::AudioFormatWriter&, Frame from, Frame to) const;
    const int rate;

  private:
    const int capacity;
    const bool history;
    std::unique_ptr<std::atomic<float>[]> ring;
    std::atomic<Frame> published{0}, writingTo{0}, resetAt{0};
};
// Takes a stable reference to the device buffer, including across disconnection.
class IdeaRecording {
  public:
    IdeaRecording(std::shared_ptr<IdeaInput>, const juce::File&, bool retrospective);
    ~IdeaRecording();
    bool valid() const { return worker.joinable(); }
    void requestStop();
    bool finish();
    Frame recorded() const {
        return std::max<Frame>(0, (limit.load() >= 0 ? limit.load() : source->end()) - begin);
    }
    const juce::File file;

  private:
    std::shared_ptr<IdeaInput> source;
    Frame begin;
    std::atomic<Frame> limit{-1};
    bool ok = false;
    std::thread worker;
};
} // namespace rocker
