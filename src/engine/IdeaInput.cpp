#include "IdeaInput.h"
#include "storage/AtomicFile.h"

namespace rocker {
static_assert(std::atomic<float>::is_always_lock_free);
IdeaInput::IdeaInput(int sampleRate, bool insurance)
    : rate(sampleRate), capacity(sampleRate * (insurance ? 125 : 7)), history(insurance),
      ring(std::make_unique<std::atomic<float>[]>(capacity)) {}
void IdeaInput::push(const float* values, int count) noexcept {
    if (!values || count <= 0)
        return;
    auto pos = published.load(std::memory_order_relaxed);
    writingTo.store(pos + count);
    for (int offset = 0; offset < count;) {
        int index = int((pos + offset) % capacity);
        int n = std::min(capacity - index, count - offset);
        for (int i = 0; i < n; ++i)
            ring[index + i].store(values[offset + i], std::memory_order_release);
        offset += n;
    }
    published.store(pos + count);
}
bool IdeaInput::write(juce::AudioFormatWriter& writer, Frame from, Frame to) const {
    if (from < 0 || from > to || to > end())
        return false;
    if(from==to) return true;
    juce::AudioBuffer<float> samples(1, 4096);
    while (from < to) {
        int index = int(from % capacity);
        int n = int(std::min<Frame>({to - from, capacity - index, samples.getNumSamples()}));
        for (int i = 0; i < n; ++i)
            samples.setSample(0, i, ring[index + i].load(std::memory_order_acquire));
        if (writingTo.load() - capacity > from || !writer.writeFromAudioSampleBuffer(samples, 0, n))
            return false;
        from += n;
    }
    return true;
}
IdeaRecording::IdeaRecording(std::shared_ptr<IdeaInput> input, const juce::File& path, bool history)
    : file(path), source(std::move(input)) {
    auto end = source->end();
    begin = std::max(source->earliest(), end - Frame(source->rate) * (history ? 120 : 2));
    if (history)
        limit.store(end);
    if (file.getParentDirectory().createDirectory().failed() || (history && end <= begin))
        return;
    auto partial = file.getSiblingFile(file.getFileNameWithoutExtension() + ".partial.wav");
    auto temporary = std::make_unique<juce::TemporaryFile>(file, partial);
    auto writer = waveWriter(temporary->getFile(), source->rate, 1);
    if (!writer)
        return;
    worker = std::thread([this, temp = std::move(temporary), out = std::move(writer)]() mutable {
        Frame read = begin, flushed = begin;
        while (true) {
            auto available = source->end();
            auto last = limit.load();
            auto to = last < 0 ? available : last;
            if (!source->write(*out, read, to))
                return;
            read = to;
            if (read - flushed >= source->rate) {
                if (!out->flush())
                    return;
                flushed = read;
            }
            if (last >= 0)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (read <= begin || !out->flush())
            return;
        out.reset();
        ok = commitTemporary(*temp);
    });
}
void IdeaRecording::requestStop() {
    Frame expected = -1;
    limit.compare_exchange_strong(expected, source->end());
}
bool IdeaRecording::finish() {
    if (worker.joinable()) {
        requestStop();
        worker.join();
    }
    return ok;
}
IdeaRecording::~IdeaRecording() {
    finish();
}
} // namespace rocker
