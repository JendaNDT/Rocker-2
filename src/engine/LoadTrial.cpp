#include "LoadTrial.h"
#include <cmath>
#include <numbers>

namespace rocker {
using namespace juce;
bool makeLoadTrial(Song& song, const File& directory, int rate, int guitarCount) {
    if (directory.getChildFile("Zvuk").createDirectory().failed())
        return false;
    song = {};
    song.grid = {rate, 4, 4, 120};
    song.bars = 300;
    const int phrase = rate * 10;
    // Different source files exercise separate playback and cabinet state for all sixteen tracks.
    for (int lane = 0; lane < 16; ++lane) {
        const bool guitar = lane < guitarCount;
        const String id(lane + 1);
        Track track{id, id, guitar ? "kytara" : "bici"};
        track.amp.enabled = guitar;
        track.amp.values = presets[lane % presets.size()];
        song.tracks.push_back(track);
        Asset asset{id, "Zvuk/" + id + ".wav", phrase, 1};
        auto writer = waveWriter(directory.getChildFile(asset.path), rate, 1);
        if (!writer)
            return false;
        // Repeat a half-second toned pulse to keep preparation quick at any device rate.
        AudioBuffer<float> beat(1, rate / 2);
        double frequency = (guitar ? 82.4 : 50.) * std::exp2((lane % 7) / 12.);
        for (int i = 0; i < beat.getNumSamples(); ++i) {
            const double t = double(i) / rate;
            beat.setSample(0, i,
                           float(0.022 * std::exp(-(guitar ? 7 : 30) * t) *
                                 std::sin(2 * std::numbers::pi * frequency * t)));
        }
        for (int pos = 0; pos < phrase; pos += beat.getNumSamples())
            if (!writer->writeFromAudioSampleBuffer(beat, 0,
                                                    std::min(beat.getNumSamples(), phrase - pos)))
                return false;
        song.assets.push_back(asset);
        for (int repeat = 0; repeat < 60; ++repeat)
            song.clips.push_back({id + "-" + String(repeat), id, id, Frame(repeat) * phrase, 0, phrase});
    }
    return true;
}
} // namespace rocker
