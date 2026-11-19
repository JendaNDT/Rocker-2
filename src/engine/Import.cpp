#include "Import.h"
#include "storage/AtomicFile.h"
#include <samplerate.h>

namespace rocker {
using namespace juce;
bool supportsImport(const File& file) {
    auto ext = file.getFileExtension().toLowerCase();
#if JUCE_WINDOWS
    if (ext == ".m4a" || ext == ".aac")
        return true;
#endif
    return StringArray{".wav", ".aif", ".aiff", ".flac", ".ogg", ".oga", ".opus", ".mp3"}.contains(ext);
}
std::optional<Asset> importAudio(const File& input, const File& project, int rate, String* detail) {
    auto fail = [&](String reason) -> std::optional<Asset> {
        if (detail) *detail = std::move(reason);
        return std::nullopt;
    };
    if (!supportsImport(input) || !input.existsAsFile() || rate <= 0)
        return fail("Invalid source or project rate");
    TemporaryFile decoded(".wav");
    auto media = input;
    if (input.hasFileExtension("m4a;aac")) {
        if (!decodeSystemAudio(input, decoded.getFile()))
            return fail("System decoder");
        media = decoded.getFile();
    }
    AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> reader(formats.createReaderFor(media));
    if (!reader || reader->numChannels < 1 || reader->numChannels > 2 || reader->lengthInSamples <= 0 ||
        reader->sampleRate < 8000 || reader->sampleRate > 384000)
        return fail("Unsupported stream");
    int channels = int(reader->numChannels);
    Asset a;
    a.id = Uuid().toString();
    a.path = "Zvuk/" + a.id + ".wav";
    a.channels = channels;
    a.length = std::llround(reader->lengthInSamples * double(rate) / reader->sampleRate);
    auto dest = project.getChildFile(a.path);
    if (dest.getParentDirectory().createDirectory().failed())
        return fail("Destination directory");
    TemporaryFile tmp(dest);
    auto writer = waveWriter(tmp.getFile(), rate, channels);
    if (!writer)
        return fail("WAV writer");
    constexpr int block = 4096;
    AudioBuffer<float> read(channels, block), write(channels, block);
    std::vector<float> source(block * channels), converted(block * channels);
    double ratio = rate / reader->sampleRate;
    int error = 0;
    std::unique_ptr<SRC_STATE, decltype(&src_delete)> converter(
        ratio == 1 ? nullptr : src_new(SRC_SINC_BEST_QUALITY, channels, &error), src_delete);
    if (error)
        return fail(src_strerror(error));
    Frame written = 0;
    for (Frame at = 0; at < reader->lengthInSamples;) {
        int count = int(std::min<Frame>(block, reader->lengthInSamples - at));
        if (!reader->read(&read, 0, count, at, true, channels == 2))
            return fail("Source read");
        at += count;
        if (!converter) {
            if (!writer->writeFromAudioSampleBuffer(read, 0, count))
                return fail("Native-rate write");
            written += count;
            continue;
        }
        for (int i = 0; i < count; ++i)
            for (int c = 0; c < channels; ++c)
                source[i * channels + c] = read.getSample(c, i);
        int used = 0;
        while (true) {
            SRC_DATA data{};
            // A zero-frame drain must not alias the output when the allocator places buffers next to each other.
            data.data_in = source.data() + (used < count ? used * channels : 0);
            data.input_frames = count - used;
            data.data_out = converted.data();
            data.output_frames = block;
            data.src_ratio = ratio;
            data.end_of_input = at == reader->lengthInSamples;
            if ((error = src_process(converter.get(), &data)) != 0)
                return fail(src_strerror(error));
            used += int(data.input_frames_used);
            int n = int(std::min<Frame>(data.output_frames_gen, a.length - written));
            for (int c = 0; c < channels; ++c)
                for (int i = 0; i < n; ++i)
                    write.setSample(c, i, converted[i * channels + c]);
            if (n > 0 && !writer->writeFromAudioSampleBuffer(write, 0, n))
                return fail("Converted write");
            written += n;
            if (used == count && data.output_frames_gen < block)
                break;
            if (data.output_frames_gen == 0 && data.input_frames_used == 0)
                return fail("No converter progress");
        }
    }
    if (written == a.length - 1) {
        write.clear();
        if (!writer->writeFromAudioSampleBuffer(write, 0, 1))
            return fail("Final sample write");
        ++written;
    }
    if (written != a.length || !writer->flush())
        return fail("Final length or flush: " + String(written) + " / " + String(a.length));
    writer.reset();
    return commitTemporary(tmp) ? std::optional<Asset>(a) : fail("Publish WAV");
}
} // namespace rocker
