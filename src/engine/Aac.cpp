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
bool check(OSStatus status, const char* action, String& detail) {
    if (status == noErr)
        return true;
    detail = String(action) + ": " + String(status);
    return false;
}
bool verifyLength(CFURLRef url, Frame frames, String& detail) {
    PacketFile destination;
    if (!check(AudioFileOpenURL(url, kAudioFileReadPermission, kAudioFileM4AType, &destination.file),
               "Open finished M4A", detail))
        return false;
    AudioFilePacketTableInfo table{};
    UInt32 size = sizeof(table);
    if (!check(AudioFileGetProperty(destination.file, kAudioFilePropertyPacketTableInfo, &size, &table),
               "Read AAC packet table", detail))
        return false;
    if (table.mNumberValidFrames == frames)
        return true;
    detail = "AAC packet table length: " + String(table.mNumberValidFrames) + " / " + String(frames);
    return false;
}
} // namespace
#endif

bool encodeAac(const File& pcm, const File& destination, String& detail) {
#if JUCE_MAC
    const auto source = waveReader(pcm);
    if (!source || source->sampleRate != 44100 || source->numChannels != 2) {
        detail = "Read stereo 44.1 kHz PCM";
        return false;
    }
    Url target(destination);
    AudioStreamBasicDescription compressed{};
    compressed.mSampleRate = source->sampleRate;
    compressed.mChannelsPerFrame = source->numChannels;
    compressed.mFormatID = kAudioFormatMPEG4AAC;
    UInt32 bytes = sizeof(compressed);
    if (!target.value) {
        detail = "Create destination URL";
        return false;
    }
    if (!check(AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, nullptr, &bytes, &compressed),
               "Configure AAC file format", detail))
        return false;
    ExtendedFile audio;
    if (!check(ExtAudioFileCreateWithURL(target.value, kAudioFileM4AType, &compressed, nullptr,
                                       kAudioFileFlags_EraseFile, &audio.file),
               "Create M4A", detail))
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
    if (!check(ExtAudioFileSetProperty(audio.file, kExtAudioFileProperty_ClientDataFormat,
                                       sizeof(client), &client), "Configure PCM input", detail))
        return false;
    AudioConverterRef converter = nullptr;
    bytes = sizeof(converter);
    if (!check(ExtAudioFileGetProperty(audio.file, kExtAudioFileProperty_AudioConverter, &bytes,
                                       &converter), "Find AAC converter", detail) || converter == nullptr)
        return false;
    UInt32 bitrate = 256000;
    if (!check(AudioConverterSetProperty(converter, kAudioConverterEncodeBitRate, sizeof(bitrate),
                                         &bitrate), "Choose AAC bitrate", detail))
        return false;
    CFArrayRef refresh = nullptr;
    if (!check(ExtAudioFileSetProperty(audio.file, kExtAudioFileProperty_ConverterConfig, sizeof(refresh),
                                       &refresh), "Refresh AAC converter", detail))
        return false;
    constexpr int block = 2048;
    juce::AudioBuffer<float> input(2, block);
    std::array<float, 2 * block> samples{};
    for (Frame at = 0; at < source->lengthInSamples; at += block) {
        auto count = int(std::min<Frame>(block, source->lengthInSamples - at));
        if (!source->read(&input, 0, count, at, true, true)) {
            detail = "Read PCM block";
            return false;
        }
        for (int i = 0; i < count; ++i) {
            samples[2 * i] = input.getSample(0, i);
            samples[2 * i + 1] = input.getSample(1, i);
        }
        AudioBufferList list{1, {{2, UInt32(2 * count * sizeof(float)), samples.data()}}};
        if (!check(ExtAudioFileWrite(audio.file, UInt32(count), &list), "Write AAC block", detail))
            return false;
    }
    if (!audio.finish()) {
        detail = "Finish M4A";
        return false;
    }
    return verifyLength(target.value, source->lengthInSamples, detail);
#else
    juce::ignoreUnused(pcm, destination, detail);
    return false;
#endif
}
} // namespace rocker
