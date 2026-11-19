#include "Export.h"
#include "storage/AtomicFile.h"
#include <cstring>

namespace rocker {
// Media Foundation emits whole MPEG frames. Add an Info frame that tells gapless
// players how many decoded samples belong to the song and preserves every audio frame.
bool tagMp3Length(const juce::File& file, Frame samples) {
    using namespace juce;
    auto input = file.createInputStream();
    if (!input || input->getTotalLength() < 10 || samples <= 0)
        return false;
    uint8_t hdr[10]{};
    if (input->read(hdr, 10) != 10)
        return false;
    Frame start = 0;
    if (std::memcmp(hdr, "ID3", 3) == 0) {
        start = 10;
        int bytes = 0;
        for (int i = 6; i < 10; ++i)
            bytes = (bytes << 7) | (hdr[i] & 0x7f);
        start += bytes + (hdr[3] == 4 && (hdr[5] & 16) ? 10 : 0);
    }
    Frame pos = start, frames = 0, end = input->getTotalLength();
    while (pos + 4 <= end) {
        if (!input->setPosition(pos) || input->read(hdr, 4) != 4)
            return false;
        // MPEG-1 Layer III, 320 kb/s, 44.1 kHz, two channels.
        if (hdr[0] != 255 || (hdr[1] & 254) != 250 || (hdr[2] & 252) != 224 || (hdr[3] >> 6) == 3)
            break;
        int bytes = 144 * 320000 / 44100 + ((hdr[2] >> 1) & 1);
        if (pos + bytes > end)
            return false;
        if (pos == start) {
            char tag[4]{};
            input->setPosition(pos + 36 + ((hdr[1] & 1) ? 0 : 2));
            if (input->read(tag, 4) == 4 &&
                (!std::memcmp(tag, "Info", 4) || !std::memcmp(tag, "Xing", 4))) {
                start = pos + bytes;
                pos = start;
                continue;
            }
        }
        ++frames;
        pos += bytes;
    }
    constexpr int delay = 576;
    Frame padding = frames * 1152 - samples - delay;
    if (!frames || frames > UINT32_MAX || padding < 0 || padding > 4095 ||
        pos - start > UINT32_MAX - 1044)
        return false;
    std::array<uint8_t, 144 * 320000 / 44100> info{};
    info[0] = 255;
    info[1] = 251;
    info[2] = 224;
    std::memcpy(info.data() + 36, "Info", 4);
    info[43] = 3; // Frame count and stream size follow the flags.
    auto put32 = [&](int offset, Frame value) {
        for (int i = 0; i < 4; ++i)
            info[offset + i] = uint8_t(value >> (24 - 8 * i));
    };
    put32(44, frames);
    put32(48, pos - start + int(info.size()));
    std::memcpy(info.data() + 52, "Rocker2MF", 9);
    constexpr int timing = 52 + 21;
    info[timing] = delay >> 4;
    info[timing + 1] = uint8_t((delay << 4) | (padding >> 8));
    info[timing + 2] = uint8_t(padding);
    TemporaryFile output(file);
    auto out = output.getFile().createOutputStream();
    if (!out || !out->write(info.data(), info.size()) || !input->setPosition(start) ||
        out->writeFromInputStream(*input, pos - start) != pos - start)
        return false;
    out->flush();
    bool ok = out->getStatus().wasOk();
    out.reset();
    input.reset();
    return ok && commitTemporary(output);
}
} // namespace rocker
