#include "Export.h"
#include "Import.h"
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

namespace rocker {
using Microsoft::WRL::ComPtr;
namespace {
class MediaRuntime {
  public:
    MediaRuntime() {
        // Windows N can omit Media Foundation. The imports are delay-loaded.
        static const bool installed =
            LoadLibraryExW(L"mfplat.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32) &&
            LoadLibraryExW(L"mf.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32) &&
            LoadLibraryExW(L"mfreadwrite.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!installed)
            return;
        auto status = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        com = SUCCEEDED(status);
        active = (com || status == RPC_E_CHANGED_MODE) && SUCCEEDED(MFStartup(MF_VERSION));
    }
    ~MediaRuntime() {
        if (active)
            MFShutdown();
        if (com)
            CoUninitialize();
    }
    bool active = false, com = false;
};
ComPtr<IMFMediaType> encoderType() {
    ComPtr<IMFCollection> all;
    if (FAILED(
            MFTranscodeGetAudioOutputAvailableTypes(MFAudioFormat_MP3, MFT_ENUM_FLAG_ALL, nullptr, &all)))
        return {};
    DWORD count = 0;
    if (FAILED(all->GetElementCount(&count)))
        return {};
    for (DWORD i = 0; i < count; ++i) {
        ComPtr<IUnknown> entry;
        ComPtr<IMFMediaType> type;
        if (SUCCEEDED(all->GetElement(i, &entry)) && SUCCEEDED(entry.As(&type)) &&
            MFGetAttributeUINT32(type.Get(), MF_MT_AUDIO_SAMPLES_PER_SECOND, 0) == 44100 &&
            MFGetAttributeUINT32(type.Get(), MF_MT_AUDIO_NUM_CHANNELS, 0) == 2 &&
            MFGetAttributeUINT32(type.Get(), MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0) == 40000)
            return type;
    }
    return {};
}
ComPtr<IMFSourceReader> source(const juce::File& f, bool force16=false) {
    ComPtr<IMFSourceReader> reader;
    if (FAILED(MFCreateSourceReaderFromURL(f.getFullPathName().toWideCharPointer(), nullptr, &reader)))
        return {};
    ComPtr<IMFMediaType> type;
    if (FAILED(MFCreateMediaType(&type)) || FAILED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
        FAILED(type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)) ||
        (force16 && FAILED(type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16))) ||
        FAILED(reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, type.Get())))
        return {};
    return reader;
}
} // namespace
bool decodeSystemAudio(const juce::File& file,const juce::File& target) {
    MediaRuntime runtime;
    if(!runtime.active) return false;
    auto reader=source(file,true); ComPtr<IMFMediaType> raw;
    if(!reader || FAILED(reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,&raw))) return false;
    int channels=(int)MFGetAttributeUINT32(raw.Get(),MF_MT_AUDIO_NUM_CHANNELS,0);
    int rate=(int)MFGetAttributeUINT32(raw.Get(),MF_MT_AUDIO_SAMPLES_PER_SECOND,0);
    if(channels<1 || channels>2 || rate<8000 || rate>384000 || MFGetAttributeUINT32(raw.Get(),MF_MT_AUDIO_BITS_PER_SAMPLE,0)!=16) return false;
    auto writer=waveWriter(target,rate,channels); if(!writer) return false;
    while(true) {
        DWORD flags=0;ComPtr<IMFSample> sample;
        if(FAILED(reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,nullptr,&flags,nullptr,&sample))) return false;
        if(sample) {
            ComPtr<IMFMediaBuffer> data;BYTE* bytes=nullptr;DWORD size=0;
            if(FAILED(sample->ConvertToContiguousBuffer(&data)) || FAILED(data->Lock(&bytes,nullptr,&size))) return false;
            int n=(int)(size/(2*channels));
            juce::AudioBuffer<float> block(channels,n);
            for(int c=0;c<channels;++c) for(int i=0;i<n;++i)
                block.setSample(c,i,float(reinterpret_cast<const int16_t*>(bytes)[i*channels+c])/32768.f);
            data->Unlock();
            if(!writer->writeFromAudioSampleBuffer(block,0,n)) return false;
        }
        if(flags & MF_SOURCE_READERF_ENDOFSTREAM) return writer->flush();
    }
}
bool mp3Available() {
    MediaRuntime runtime;
    return runtime.active && encoderType().Get() != nullptr;
}
bool encodeMp3(const juce::File& pcm, const juce::File& dest) {
    MediaRuntime runtime;
    if (!runtime.active)
        return false;
    auto reader = source(pcm);
    auto encoded = encoderType();
    ComPtr<IMFSinkWriter> writer;
    ComPtr<IMFMediaType> raw;
    if (!reader || !encoded ||
        FAILED(reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &raw)) ||
        FAILED(MFCreateSinkWriterFromURL(dest.getFullPathName().toWideCharPointer(), nullptr, nullptr,
                                         &writer)))
        return false;
    DWORD stream = 0;
    if (FAILED(writer->AddStream(encoded.Get(), &stream)) ||
        FAILED(writer->SetInputMediaType(stream, raw.Get(), nullptr)) || FAILED(writer->BeginWriting()))
        return false;
    while (true) {
        DWORD flags = 0;
        ComPtr<IMFSample> sample;
        if (FAILED(reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr,
                                      &sample)))
            return false;
        if (sample && FAILED(writer->WriteSample(stream, sample.Get())))
            return false;
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            break;
    }
    return SUCCEEDED(writer->Finalize());
}
std::optional<Frame> decodedMp3Frames(const juce::File& file) {
    MediaRuntime runtime;
    if (!runtime.active)
        return {};
    auto reader = source(file);
    ComPtr<IMFMediaType> raw;
    if (!reader || FAILED(reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &raw)))
        return {};
    auto align = MFGetAttributeUINT32(raw.Get(), MF_MT_AUDIO_BLOCK_ALIGNMENT, 0);
    if (!align)
        return {};
    Frame length = 0;
    while (true) {
        DWORD flags = 0, bytes = 0;
        ComPtr<IMFSample> sample;
        if (FAILED(reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr,
                                      &sample)))
            return {};
        if (sample && FAILED(sample->GetTotalLength(&bytes)))
            return {};
        length += bytes / align;
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            return length;
    }
}
} // namespace rocker
#else
namespace rocker {
bool decodeSystemAudio(const juce::File&, const juce::File&) {return false;}
bool mp3Available() {
    return false;
}
bool encodeMp3(const juce::File&, const juce::File&) {
    return false;
}
std::optional<Frame> decodedMp3Frames(const juce::File&) {
    return {};
}
} // namespace rocker
#endif
