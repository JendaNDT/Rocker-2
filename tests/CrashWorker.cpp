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
    if (action == "midi") {
        s.tracks = {{"t", "Module", "midi", {}, true}};
        if (!s.save(dir))
            return 11;
        Performance p(s, {}, 0, true, false, 0);
        if (!p.arm(s, dir, {}, 0, true, 173))
            return 12;
        // Announce only after the first checkpoint, and freeze at the same known frame until killed.
        constexpr int blocks = 1139;
        for (int k = 0; k < blocks; ++k) {
            Hit h{480LL * k + 173, 38, 85};
            p.hits(std::span<const Hit>(&h, 1), 480LL * k, false);
            p.process(nullptr, 0, nullptr, 0, 480);
            if (k % 90 == 0)
                Thread::sleep(4);
        }
        const auto logs = dir.getChildFile("Zachrana").findChildFiles(File::findFiles, false, "*.hits");
        if (logs.size() != 1)
            return 16;
        const auto deadline = Time::getMillisecondCounterHiRes() + 3000;
        bool checkpoint = false;
        while (!checkpoint && Time::getMillisecondCounterHiRes() < deadline) {
            const auto lines = StringArray::fromLines(logs[0].loadFileAsString());
            checkpoint = std::any_of(lines.begin(), lines.end(), [&](const String& line) {
                return line.startsWith("end ") && line.substring(4).getLargeIntValue() >= (blocks - 100) * 480LL;
            });
            if (!checkpoint)
                Thread::sleep(5);
        }
        if (!checkpoint)
            return 16;
        std::cout << "READY" << std::endl;
        for (;;)
            Thread::sleep(1000);
    }
    if (action == "recoverMidi") {
        bool bad = false;
        auto song = Song::load(dir, bad);
        auto files = dir.getChildFile("Zachrana").findChildFiles(File::findFiles, false, "*.json");
        if (!song || bad || files.size() != 1 || !recoverTake(*song, dir, files[0]) ||
            song->clips.size() != 1)
            return 13;
        auto hits = readMidi(dir.getChildFile(song->assets[0].path), 48000);
        if (!hits || hits->size() < 1039 || hits->size() > 1139 || song->clips[0].offset != 173)
            return 14;
        for (size_t i = 0; i < hits->size(); ++i)
            if ((*hits)[i].frame != Frame(i) * 480 + 173 || (*hits)[i].note != 38)
                return 15;
        return 0;
    }
    if (action == "loop") {
        s.tracks = {{"t", "Loop crash", "jine", {"in"}, true}};
        if (!s.save(dir))
            return 17;
        constexpr Frame length = 48000;
        Performance p(s, {}, 0, true, false, 0, false, LoopSpan{0, length});
        if (!p.arm(s, dir, {"in"}, 0))
            return 18;
        std::array<float, 480> data{};
        const float* in[]{data.data()};
        for (int k = 0; k < 250; ++k) {
            for (int i = 0; i < 480; ++i)
                data[i] = float(k * 480 + i);
            p.process(in, 1, nullptr, 0, 480);
            if ((k % 45) == 0)
                Thread::sleep(4);
        }
        const auto waves = dir.getChildFile("Zvuk").findChildFiles(File::findFiles, false, "*.wav");
        if (waves.size() != 1)
            return 19;
        const auto deadline = Time::getMillisecondCounterHiRes() + 3000;
        bool ready = false;
        while (!ready && Time::getMillisecondCounterHiRes() < deadline) {
            auto check = waveReader(waves[0]);
            ready = check && check->lengthInSamples > 2 * length;
            if (!ready)
                Thread::sleep(5);
        }
        if (!ready)
            return 20;
        std::cout << "READY" << std::endl;
        for (;;)
            Thread::sleep(1000);
    }
    if (action == "recoverLoop") {
        bool bad = false;
        auto song = Song::load(dir, bad);
        const auto logs = dir.getChildFile("Zachrana").findChildFiles(File::findFiles, false, "*.json");
        if (!song || bad || logs.size() != 1 || !recoverTake(*song, dir, logs[0]) ||
            song->assets.size() != 1 || song->groups.size() != 1)
            return 21;
        auto& group = song->groups.front();
        if (group.attempts.size() != 3 || group.active != 2 || !group.attempts[2].discarded)
            return 22;
        bool restoredBad = false;
        auto again = Song::load(dir, restoredBad);
        if (restoredBad || !again || again->groups != song->groups)
            return 23;
        return 0;
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
