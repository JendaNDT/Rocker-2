#pragma once
#include "MidiDrums.h"
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
    std::unique_ptr<juce::TemporaryFile> preview;
    std::unique_ptr<juce::MemoryMappedAudioFormatReader> reader;
    std::vector<float> peaks;
    float peak(Frame start, int length) const noexcept;
};
struct AudioLibrary {
    std::map<juce::String, std::shared_ptr<const MappedWave>> data;
    auto waveFor(const Track& track, const juce::String& id) const {
        return data.find(track.kind == "midi" ? id + "#" + juce::String(track.kit) : id);
    }
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
    void selectAttempt(const juce::String& group, int index);
    void setPunch(size_t track, Frame from, Frame to, int left, int right, bool clean);
    void mix(float* const*, int outputs, int size, Frame at, const float* const* input = nullptr,
             int inputs = 0, int inputOffset = 0) noexcept;
    struct Meter {
        float left = 0, right = 0, maximum = 0;
        Frame loudest = -1, over = -1;
    };
    Meter meter() const noexcept;
    float trackPeak(size_t index) const noexcept;
    Frame tailFrames() const { return tail; }

  private:
    std::atomic<float> master{1}, leftPeak{0}, rightPeak{0}, maxPeak{0};
    std::atomic<Frame> loudest{-1}, over{-1};
    std::vector<Frame> cuts;
    int cutFade = 1;
    struct Overlay {
        juce::String id;
        Frame from = 0, to = 0;
        std::vector<std::vector<PlaybackClip>> variants;
        std::atomic<int> active{0};
        std::vector<float> weights;
        int target = -1, remaining = 0;
    };
    struct Strip {
        std::vector<std::unique_ptr<Overlay>> groups;
        Frame punchFrom = 0, punchTo = 0;
        bool cleanPunch = false;
        int livePorts[2]{-1, -1}, fade = 1;
        std::vector<PlaybackClip> clips;
        std::vector<Amp> amps;
        std::atomic<bool> audible{true};
        std::atomic<float> gain[2]{{1}, {1}}, peak{0};
        AmpCommand command;
        std::uint64_t applied = 0;
        Frame next = -1;
        float blockPeak = 0;
        int channels = 1;
        void read(const std::vector<PlaybackClip>&, float (&block)[2][256], int count,
                  Frame at) const noexcept;
        void process(float* const*, int outputs, int count, Frame at, const float* const*, int,
                     int) noexcept;
    };
    std::vector<std::unique_ptr<Strip>> strips;
    bool hasClips = false;
    Frame tail = 0;
};
bool renderSong(const Song&, const AudioLibrary&, const juce::File& destination, Frame begin = 0,
                Frame end = -1);
struct MidiCapture {
    HitQueue queue;
    juce::File path;
    std::unique_ptr<juce::FileOutputStream> stream;
    std::atomic<Frame> duration{0};
    Frame flushed = 0;
    bool drain(int rate, bool final);
};
struct Take {
    std::unique_ptr<MidiCapture> midi;
    Asset asset;
    juce::String track;
    int left = -1, right = -1;
    Frame inputOver = -1;
    std::unique_ptr<CaptureQueue> queue;
    std::unique_ptr<juce::AudioFormatWriter> writer;
};
class Performance {
  public:
    Performance(const Song&, const AudioLibrary&, Frame start, bool recording, bool click,
                int countBeats);
    ~Performance();
    bool arm(const Song&, const juce::File&, const juce::StringArray& ports, int delay,
             bool useWorker = true, int midiDelay = 0);
    void process(const float* const*, int, float* const*, int, int) noexcept;
    void hits(std::span<const Hit>, Frame devicePosition, bool overflow) noexcept;
    void finishDisk();
    bool drain();
    void storeInputMarks(Song&) const;
    BeatGrid grid;
    Playback playback;
    Frame start, end, clock;
    bool recording, click, syncPulse = false;
    std::atomic<Frame> cursor, drop{-1};
    std::atomic<bool> finished{false}, failed{false};
    juce::File journal;
    int compensation = 0;
    Frame punchFrom = 0, punchTo = 0;
    bool monitorPunch = false, cleanPunch = false;
    bool isPunch() const { return punchTo > punchFrom; }

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
    void process(const float* const*, int, float* const*, int, int, std::span<const Hit> hits = {},
                 Frame position = 0, bool overflow = false) noexcept;

  private:
    std::atomic<Performance*> active{nullptr};
    std::atomic<bool> reading{false};
};
std::optional<std::pair<int, int>> syncOffsets(juce::AudioFormatReader&, const BeatGrid&, int delay);
bool recoverTake(Song&, const juce::File& directory, const juce::File& journal);
} // namespace rocker
