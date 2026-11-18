#pragma once
#include "song/Song.h"
#include <atomic>
#include <thread>

namespace rocker {
class CaptureQueue {
  public:
    CaptureQueue(int capacity, int channels);
    bool push(const float* left, const float* right, int count) noexcept;
    bool drain(juce::AudioFormatWriter&, int rate);
    bool hasRoom(int frames) const noexcept;

  private:
    juce::AudioBuffer<float> buffer;
    std::atomic<Frame> read{0}, written{0};
    Frame flushed = 0;
};
struct MappedWave {
    static constexpr int bin = 256;
    std::unique_ptr<juce::MemoryMappedAudioFormatReader> reader;
    std::vector<float> peaks;
    float peak(Frame start, int length) const noexcept;
};
struct AudioLibrary {
    std::map<juce::String, std::shared_ptr<const MappedWave>> data;
    bool load(const Song&, const juce::File&);
};
struct PlaybackClip {
    std::shared_ptr<const MappedWave> wave;
    Frame at, offset, length;

};
class Playback {
  public:
    Playback(const Song&, const AudioLibrary&);
    void updateMix(const Song&);
    void prefetch(Frame from, Frame to) const;
    bool empty() const { return !hasClips; }
    void setSound(size_t track, const GuitarSound&, bool recall);
    void mix(float* const*, int outputs, int size, Frame at) noexcept;
    Frame tailFrames() const { return tail; }

  private:
    struct Strip {
        std::vector<PlaybackClip> clips;
        std::vector<Amp> amps;
        std::atomic<bool> audible{true};
        std::atomic<std::uint64_t> command{0};
        std::atomic<unsigned> recalls{0};
        std::uint64_t applied = 0;
        unsigned previousRecall = 0;
        Frame next = -1;
        int channels = 1;
        void read(float (&block)[2][256], int count, Frame at) const noexcept;
        void process(float* const*, int outputs, int count, Frame at) noexcept;
    };
    std::vector<std::unique_ptr<Strip>> strips;
    bool hasClips = false;
    Frame tail = 0;
};
bool renderSong(const Song&, const AudioLibrary&, const juce::File& destination);
struct Take {
    Asset asset;
    juce::String track;
    int left = -1, right = -1;
    std::unique_ptr<CaptureQueue> queue;
    std::unique_ptr<juce::AudioFormatWriter> writer;
};
class Performance {
  public:
    Performance(const Song&, const AudioLibrary&, Frame start, bool recording, bool click,
                int countBeats);
    ~Performance();
    bool arm(const Song&, const juce::File&, const juce::StringArray& ports, int delay,
             bool useWorker = true);
    void process(const float* const*, int, float* const*, int, int) noexcept;
    void finishDisk();
    bool drain();
    BeatGrid grid;
    Playback playback;
    Frame start, end, clock;
    bool recording, click, syncPulse = false;
    std::atomic<Frame> cursor, drop{-1};
    std::atomic<bool> finished{false}, failed{false};
    juce::File journal;
    int compensation = 0;

  private:
    std::vector<Take> takes;
    std::thread worker, readAhead;
    std::atomic<bool> stopping{false};
    Frame nextBeat, beatIndex;
    int clickAge = -1;
    bool accent = false;
};
// Only the device callback reads the published pointer. The controller waits
// for a current callback to exit before releasing a performance.
class Timeline {
  public:
    void attach(Performance& p) noexcept { active.store(&p); }
    void detach() noexcept;
    void process(const float* const*, int, float* const*, int, int) noexcept;

  private:
    std::atomic<Performance*> active{nullptr};
    std::atomic<bool> reading{false};
};
std::optional<std::pair<int, int>> syncOffsets(juce::AudioFormatReader&, const BeatGrid&, int delay);
bool recoverTake(Song&, const juce::File& directory, const juce::File& journal);
} // namespace rocker
