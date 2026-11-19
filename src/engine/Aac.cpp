#include "Export.h"
#if JUCE_MAC
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace rocker {
using namespace juce;
#if JUCE_MAC
namespace {
struct Url {
    CFURLRef value = nullptr;
    explicit Url(const File& file) {
        const auto& name = file.getFullPathName();
        value = CFURLCreateFromFileSystemRepresentation(
            kCFAllocatorDefault, reinterpret_cast<const UInt8*>(name.toRawUTF8()),
            name.getNumBytesAsUTF8(), false);
    }
    ~Url() {
        if (value)
            CFRelease(value);
    }
};
struct ExtendedFile {
    ExtAudioFileRef file = nullptr;
    ~ExtendedFile() {
        if (file)
            ExtAudioFileDispose(file);
    }
    bool finish() { return ExtAudioFileDispose(std::exchange(file, nullptr)) == noErr; }
};
struct PacketFile {
    AudioFileID file = nullptr;
    ~PacketFile() {
        if (file)
            AudioFileClose(file);
    }
};
bool preserveLength(CFURLRef url, Frame frames) {
    PacketFile destination;
    if (AudioFileOpenURL(url, kAudioFileReadWritePermission, kAudioFileM4AType, &destination.file) !=
        noErr)
        return false;
    AudioFilePacketTableInfo table{};
    UInt32 size = sizeof(table);
    if (AudioFileGetProperty(destination.file, kAudioFilePropertyPacketTableInfo, &size, &table) !=
        noErr)
        return false;
    if (table.mNumberValidFrames == frames)
        return true;
    const auto remainder = table.mNumberValidFrames + table.mRemainderFrames - frames;
    if (remainder < 0 || remainder > std::numeric_limits<SInt32>::max())
        return false;
    table.mRemainderFrames = static_cast<SInt32>(remainder);
    table.mNumberValidFrames = frames;
    return AudioFileSetProperty(destination.file, kAudioFilePropertyPacketTableInfo, sizeof(table),
                                &table) == noErr;
}
} // namespace
#endif

bool encodeAac(const File& pcm, const File& destination) {
#if JUCE_MAC
    const auto source = waveReader(pcm);
    if (!source || source->sampleRate != 44100 || source->numChannels != 2)
        return false;
    Url target(destination);
    AudioStreamBasicDescription compressed{};
    compressed.mSampleRate = source->sampleRate;
    compressed.mChannelsPerFrame = source->numChannels;
    compressed.mFormatID = kAudioFormatMPEG4AAC;
    UInt32 bytes = sizeof(compressed);
    if (!target.value || AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, nullptr, &bytes,
                                               &compressed) != noErr)
        return false;
    ExtendedFile audio;
    if (ExtAudioFileCreateWithURL(target.value, kAudioFileM4AType, &compressed, nullptr,
                                 kAudioFileFlags_EraseFile, &audio.file) != noErr)
        return false;
    AudioStreamBasicDescription client{};
    client.mSampleRate = source->sampleRate;
    client.mFormatID = kAudioFormatLinearPCM;
    client.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    client.mBytesPerFrame = 2 * sizeof(float);
    client.mBytesPerPacket = client.mBytesPerFrame;
    client.mFramesPerPacket = 1;
    client.mChannelsPerFrame = 2;
    client.mBitsPerChannel = 32;
    if (ExtAudioFileSetProperty(audio.file, kExtAudioFileProperty_ClientDataFormat,
                                sizeof(client), &client) != noErr)
        return false;
    AudioConverterRef converter = nullptr;
    bytes = sizeof(converter);
    if (ExtAudioFileGetProperty(audio.file, kExtAudioFileProperty_AudioConverter, &bytes,
                                &converter) != noErr || converter == nullptr)
        return false;
    UInt32 bitrate = 256000;
    if (AudioConverterSetProperty(converter, kAudioConverterEncodeBitRate, sizeof(bitrate), &bitrate) !=
        noErr)
        return false;
    CFArrayRef refresh = nullptr;
    if (ExtAudioFileSetProperty(audio.file, kExtAudioFileProperty_ConverterConfig, sizeof(refresh),
                                &refresh) != noErr)
        return false;
    constexpr int block = 2048;
    juce::AudioBuffer<float> input(2, block);
    std::array<float, 2 * block> samples{};
    for (Frame at = 0; at < source->lengthInSamples; at += block) {
        auto count = int(std::min<Frame>(block, source->lengthInSamples - at));
        if (!source->read(&input, 0, count, at, true, true))
            return false;
        for (int i = 0; i < count; ++i) {
            samples[2 * i] = input.getSample(0, i);
            samples[2 * i + 1] = input.getSample(1, i);
        }
        AudioBufferList list{1, {{2, UInt32(2 * count * sizeof(float)), samples.data()}}};
        if (ExtAudioFileWrite(audio.file, UInt32(count), &list) != noErr)
            return false;
    }
    return audio.finish() && preserveLength(target.value, source->lengthInSamples);
#else
    juce::ignoreUnused(pcm, destination);
    return false;
#endif
}
} // namespace rocker
