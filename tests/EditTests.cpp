#include "song/Edit.h"
#include <catch2/catch_test_macros.hpp>
#include <random>

namespace {
using namespace rocker;
using namespace juce;
Song fixture() {
    Song song;
    song.title = String(u8"Střih");
    song.tracks = {{"g", "Guitar", "kytara"}, {"d", "Drums", "bici"}};
    Frame length = song.grid.beat(32);
    song.assets = {{"a", "Zvuk/a.wav", length + 173, 1}};
    song.clips = {{"g1", "g", "a", 0, 173, length}, {"d1", "d", "a", 0, 0, length}};
    return song;
}
TEST_CASE("A-01 bar range at fractional beat lengths moves, repeats and restores source offsets") {
    auto song = fixture();
    const auto original = song;
    auto bar = [&](int n) { return song.grid.beat(n * 4); };
    const rocker::Span pick{bar(4), bar(6)};
    EditHistory h;
    h.reset(song);
    REQUIRE(moveSpan(song, "g", pick, bar(8)));
    h.remember(song, "move");
    auto part = copySpan(song, "g", {bar(8), bar(8) + pick.length()});
    REQUIRE(part.clips.size() == 1);
    CHECK(part.clips.front().offset == 173 + pick.from);
    REQUIRE(copySpan(song, "g", pick).clips.empty());
    REQUIRE(pasteAt(song, "g", bar(8) + pick.length(), part));
    h.remember(song, "repeat");
    CHECK(song.assets == original.assets);
    auto final = song.json();
    CHECK(h.step(song, false) == "repeat");
    CHECK(h.step(song, false) == "move");
    REQUIRE(JSON::toString(song.json()) == JSON::toString(original.json()));
    h.step(song, true);
    h.step(song, true);
    REQUIRE(JSON::toString(song.json()) == JSON::toString(final));
    REQUIRE_FALSE(h.remember(song, "view"));
    h.step(song, false);
    REQUIRE(splitAt(song, "d", bar(1)));
    h.remember(song, "split");
    CHECK_FALSE(h.canStep(true));
}
TEST_CASE("A-01 one thousand seeded edits undo and redo in a single session") {
    auto song = fixture();
    EditHistory history;
    history.reset(song);
    auto initial = JSON::toString(song.json());
    std::mt19937 rng(913775);
    auto integer = [&](int n) { return int(rng() % n); };
    int edits = 0;
    for (int attempt = 0; edits < 1000 && attempt < 10000; ++attempt) {
        auto c = song.clips[integer((int)song.clips.size())];
        const auto track = c.track;
        rocker::Span span{c.position, c.position + c.length};
        auto position = song.grid.beat(integer(64));
        switch (integer(6)) {
        case 0:
            splitAt(song, track, c.position + c.length / 2, c.id);
            break;
        case 1:
            moveSpan(song, track, span, position, c.id);
            break;
        case 2:
            pasteAt(song, track, position, copySpan(song, track, span, c.id));
            break;
        case 3:
            if (song.clips.size() > 2)
                eraseSpan(song, track, span, c.id);
            break;
        case 4:
            trimEdge(song, c.id, true, c.position + integer(2001) - 1000);
            break;
        case 5:
            trimEdge(song, c.id, false, span.to + integer(2001) - 1000);
            break;
        }
        if (history.remember(song, "edit")) {
            ++edits;
            bool bad = false;
            REQUIRE(Song::parse(song.json(), bad));
            REQUIRE_FALSE(bad);
        }
    }
    REQUIRE(edits == 1000);
    auto final = JSON::toString(song.json());
    for (int i = 0; i < 1000; ++i)
        REQUIRE(history.step(song, false) == "edit");
    CHECK_FALSE(history.canStep(false));
    REQUIRE(JSON::toString(song.json()) == initial);
    for (int i = 0; i < 1000; ++i)
        REQUIRE(history.step(song, true) == "edit");
    CHECK_FALSE(history.canStep(true));
    REQUIRE(JSON::toString(song.json()) == final);
}
TEST_CASE("Trim clamps to recorded material and paste handles intersecting edits and gaps") {
    auto s = fixture();
    auto end = s.clips[0].length;
    REQUIRE(trimEdge(s, "g1", true, 1200));
    CHECK(s.clips[0].offset == 1373);
    REQUIRE(trimEdge(s, "g1", true, -100000));
    CHECK(s.clips[0].position == 0);
    REQUIRE(trimEdge(s, "g1", false, 400));
    REQUIRE(trimEdge(s, "g1", false, end * 4));
    CHECK(s.clips[0].length == end);
    REQUIRE(eraseSpan(s, "g", {200, 400}));
    auto f = copySpan(s, "g", {100, 500});
    REQUIRE(f.clips.size() == 2);
    CHECK(f.clips.back().position == 300);
    REQUIRE(pasteAt(s, "d", 200, f));
    CHECK(copySpan(s, "d", {300, 500}).clips.empty());
    CHECK(snapBeat(s.grid, s.grid.beat(5) + 183) == s.grid.beat(5));
}
} // namespace

TEST_CASE("A-02 linked chorus, crop, following audio, undo and independent punch") {
    using namespace rocker;
    using namespace juce;
    auto song=fixture();
    song.bars=20;
    song.assets[0].length=song.barFrame(20)+173;
    for(auto& c:song.clips) c.length=song.barFrame(20);
    auto assets=song.assets;
    REQUIRE(song.splitSection(8));
    REQUIRE(song.splitSection(16));
    REQUIRE(song.repeatSection(1,2));
    REQUIRE(song.sections.size()==3);
    REQUIRE(song.order[1].section==song.order[2].section);
    REQUIRE(song.startBar(3)==24);
    auto phase=[&](int bar) {
        Frame at=song.barFrame(bar);
        for(auto& clip:song.clips)
            if (clip.track=="g" && at>=clip.position && at<clip.position+clip.length)
                return clip.offset+at-clip.position;
        return Frame(-1);
    };
    auto chorus=phase(8);
    CHECK(phase(16)==chorus);
    auto following=phase(24);
    EditHistory history;
    history.reset(song);
    auto full=song;
    REQUIRE(song.resizeOccurrence(2,6));
    history.remember(song,"crop");
    CHECK(song.startBar(3)==22);
    CHECK(phase(22)==following);
    CHECK(song.order[1].length==8);
    CHECK(song.assets==assets);
    REQUIRE(history.step(song,false)=="crop");
    CHECK(song==full);
    REQUIRE(history.step(song,true)=="crop");
    REQUIRE(song.resizeOccurrence(2,8));
    CHECK(song==full);
    Frame punch=song.barFrame(18), end=song.barFrame(19);
    REQUIRE(song.addAttempt({"pass","g","a",song.barFrame(15),0,song.barFrame(5)},punch,end,16));
    REQUIRE(song.groups.size()==2);
    CHECK(song.groups.front().from==song.barFrame(10));
    REQUIRE(song.detachSection(2));
    REQUIRE_FALSE(song.linked(1));
    REQUIRE(song.addAttempt({"second","g","a",punch,0,end-punch},punch,end,17));
    REQUIRE(song.groups[0].attempts.size()==1);
    REQUIRE(song.groups[1].attempts.size()==2);
    REQUIRE(song.resizeOccurrence(2,6));
    bool damaged=false;
    auto loaded=Song::parse(song.json(),damaged);
    REQUIRE(loaded);
    REQUIRE_FALSE(damaged);
    CHECK(*loaded==song);
    CHECK(song.assets==assets);
}
