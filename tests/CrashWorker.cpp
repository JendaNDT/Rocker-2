#include "engine/Audio.h"
#include <iostream>
using namespace rocker;
using namespace juce;
int main(int argc, char** argv) {
    if (argc != 3)
        return 2;
    const String action(argv[1]);
    File dir(String::fromUTF8(argv[2]));
    Song s;
    s.title = "Crash test";
    s.tracks = {{"t", "Drums", "bici", {"in"}, true}};
    if (action == "save") {
        for (int i = 0;; ++i) {
            s.title = i % 2 ? "A" : "B";
            s.tracks[0].name = s.title;
            if (!s.save(dir))
                return 3;
            if (i == 0)
                std::cout << "READY" << std::endl;
        }
    }
    if (action == "record") {
        if (!s.save(dir))
            return 4;
        Performance p(s, {}, 9341, true, false, 0);
        if (!p.arm(s, dir, {"in"}, 0))
            return 5;
        Audio io;
        io.prepare(1, s.grid.rate);
        io.timeline.attach(p);
        std::array<float, 480> input;
        const float* in[]{input.data()};
        std::cout << "READY" << std::endl;
        auto start = std::chrono::steady_clock::now();
        for (int k = 0;; ++k) {
            for (int i = 0; i < 480; ++i)
                input[i] = (float)(480 * k + i);
            io.process(in, 1, nullptr, 0, 480);
            std::this_thread::sleep_until(start + std::chrono::milliseconds(10LL * (k + 1)));
        }
    }
    if (action == "recover") {
        bool bad = false;
        auto song = Song::load(dir, bad);
        if (!song || bad)
            return 6;
        auto files = dir.getChildFile("Zachrana").findChildFiles(File::findFiles, false, "*.json");
        if (files.size() != 1 || !recoverTake(*song, dir, files[0]) || files[0].exists() ||
            song->clips.size() != 1 || song->clips[0].position != 9341)
            return 7;
        auto reader = waveReader(dir.getChildFile(song->assets[0].path));
        if (!reader || reader->lengthInSamples < 19 * 48000 || !reader->usesFloatingPointData)
            return 8;
        AudioBuffer<float> data(1, 8192);
        for (Frame at = 0; at < reader->lengthInSamples; at += data.getNumSamples()) {
            int n = (int)std::min<Frame>(data.getNumSamples(), reader->lengthInSamples - at);
            if (!reader->read(&data, 0, n, at, true, false))
                return 9;
            for (int j = 0; j < n; ++j)
                if (data.getSample(0, j) != (float)(at + j))
                    return 10;
        }
        std::cout << reader->lengthInSamples << std::endl;
        return 0;
    }
    return 2;
}
