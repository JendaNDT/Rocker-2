#include "Export.h"
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
        static const bool installed = LoadLibraryExW(L"mfplat.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)
            && LoadLibraryExW(L"mf.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)
            && LoadLibraryExW(L"mfreadwrite.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!installed) return;
        auto status=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
        com=SUCCEEDED(status);
        active=(com || status==RPC_E_CHANGED_MODE) && SUCCEEDED(MFStartup(MF_VERSION));
    }
    ~MediaRuntime() { if(active) MFShutdown(); if(com) CoUninitialize(); }
    bool active=false, com=false;
};
ComPtr<IMFMediaType> encoderType() {
    ComPtr<IMFCollection> all;
    if(FAILED(MFTranscodeGetAudioOutputAvailableTypes(MFAudioFormat_MP3,MFT_ENUM_FLAG_ALL,nullptr,&all))) return {};
    DWORD count=0;
    if(FAILED(all->GetElementCount(&count))) return {};
    for(DWORD i=0;i<count;++i) {
        ComPtr<IUnknown> entry; ComPtr<IMFMediaType> type;
        if(SUCCEEDED(all->GetElement(i,&entry)) && SUCCEEDED(entry.As(&type)) &&
           MFGetAttributeUINT32(type.Get(),MF_MT_AUDIO_SAMPLES_PER_SECOND,0)==44100 &&
           MFGetAttributeUINT32(type.Get(),MF_MT_AUDIO_NUM_CHANNELS,0)==2 &&
           MFGetAttributeUINT32(type.Get(),MF_MT_AUDIO_AVG_BYTES_PER_SECOND,0)==40000) return type;
    }
    return {};
}
ComPtr<IMFSourceReader> source(const juce::File& f) {
    ComPtr<IMFSourceReader> reader;
    if(FAILED(MFCreateSourceReaderFromURL(f.getFullPathName().toWideCharPointer(),nullptr,&reader))) return {};
    ComPtr<IMFMediaType> type;
    if(FAILED(MFCreateMediaType(&type)) || FAILED(type->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio)) ||
        FAILED(type->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_PCM)) ||
        FAILED(reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,nullptr,type.Get()))) return {};
    return reader;
}
}
bool mp3Available() {
    MediaRuntime runtime;
    return runtime.active && encoderType().Get()!=nullptr;
}
bool encodeMp3(const juce::File& pcm,const juce::File& dest) {
    MediaRuntime runtime;
    if(!runtime.active) return false;
    auto reader=source(pcm);
    auto encoded=encoderType();
    ComPtr<IMFSinkWriter> writer;
    ComPtr<IMFMediaType> raw;
    if(!reader || !encoded || FAILED(reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,&raw)) ||
        FAILED(MFCreateSinkWriterFromURL(dest.getFullPathName().toWideCharPointer(),nullptr,nullptr,&writer))) return false;
    DWORD stream=0;
    if(FAILED(writer->AddStream(encoded.Get(),&stream)) || FAILED(writer->SetInputMediaType(stream,raw.Get(),nullptr))
       || FAILED(writer->BeginWriting())) return false;
    while(true) {
        DWORD flags=0;
        ComPtr<IMFSample> sample;
        if(FAILED(reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,nullptr,&flags,nullptr,&sample))) return false;
        if(sample && FAILED(writer->WriteSample(stream,sample.Get()))) return false;
        if(flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
    }
    return SUCCEEDED(writer->Finalize());
}
std::optional<Frame> decodedMp3Frames(const juce::File& file) {
    MediaRuntime runtime;
    if(!runtime.active) return {};
    auto reader=source(file);
    ComPtr<IMFMediaType> raw;
    if(!reader || FAILED(reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,&raw))) return {};
    auto align=MFGetAttributeUINT32(raw.Get(),MF_MT_AUDIO_BLOCK_ALIGNMENT,0);
    if(!align) return {};
    Frame length=0;
    while(true) {
        DWORD flags=0, bytes=0;
        ComPtr<IMFSample> sample;
        if(FAILED(reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,nullptr,&flags,nullptr,&sample))) return {};
        if(sample && FAILED(sample->GetTotalLength(&bytes))) return {};
        length+=bytes/align;
        if(flags & MF_SOURCE_READERF_ENDOFSTREAM) return length;
    }
}
}
#else
namespace rocker {
bool mp3Available() { return false; }
bool encodeMp3(const juce::File&, const juce::File&) { return false; }
std::optional<Frame> decodedMp3Frames(const juce::File&) { return {}; }
}
#endif
