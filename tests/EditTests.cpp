#include "song/Edit.h"
#include <catch2/catch_test_macros.hpp>
#include <random>

namespace {
using namespace rocker;
using namespace juce;
Song fixture() {
    Song song;
    song.title = String(u8"Střih");
    song.tracks={{"g","Guitar","kytara"},{"d","Drums","bici"}};
    Frame length=song.grid.beat(32);
    song.assets={{"a","Zvuk/a.wav",length+173,1}};
    song.clips={{"g1","g","a",0,173,length},{"d1","d","a",0,0,length}};
    return song;
}
TEST_CASE("A-01 bar range at fractional beat lengths moves, repeats and restores source offsets") {
    auto song=fixture(); const auto original=song;
    auto bar=[&](int n){return song.grid.beat(n*4);};
    const Span pick{bar(4),bar(6)};
    EditHistory h;h.reset(song);
    REQUIRE(moveSpan(song,"g",pick,bar(8)));
    h.remember(song,"move");
    auto part=copySpan(song,"g",{bar(8),bar(8)+pick.length()});
    REQUIRE(part.clips.size()==1);
    CHECK(part.clips.front().offset==173+pick.from);
    REQUIRE(copySpan(song,"g",pick).clips.empty());
    REQUIRE(pasteAt(song,"g",bar(8)+pick.length(),part));
    h.remember(song,"repeat");
    CHECK(song.assets==original.assets);
    auto final=song.json();
    CHECK(h.step(song,false)=="repeat");
    CHECK(h.step(song,false)=="move");
    REQUIRE(JSON::toString(song.json())==JSON::toString(original.json()));
    h.step(song,true);h.step(song,true);
    REQUIRE(JSON::toString(song.json())==JSON::toString(final));
    REQUIRE_FALSE(h.remember(song,"view"));
    h.step(song,false);
    REQUIRE(splitAt(song,"d",bar(1))); h.remember(song,"split");
    CHECK_FALSE(h.canStep(true));
}
TEST_CASE("A-01 one thousand seeded edits undo and redo in a single session") {
    auto song=fixture();EditHistory history;history.reset(song);
    auto initial=JSON::toString(song.json());
    std::mt19937 rng(913775);
    auto integer=[&](int n){return int(rng()%n);};
    int edits=0;
    for(int attempt=0;edits<1000 && attempt<10000;++attempt){
        auto c=song.clips[integer((int)song.clips.size())];
        const auto track=c.track;
        Span span{c.position,c.position+c.length};
        auto position=song.grid.beat(integer(64));
        switch(integer(6)){
            case 0:splitAt(song,track,c.position+c.length/2,c.id);break;
            case 1:moveSpan(song,track,span,position,c.id);break;
            case 2:pasteAt(song,track,position,copySpan(song,track,span,c.id));break;
            case 3:if(song.clips.size()>2)eraseSpan(song,track,span,c.id);break;
            case 4:trimEdge(song,c.id,true,c.position+integer(2001)-1000);break;
            case 5:trimEdge(song,c.id,false,span.to+integer(2001)-1000);break;
        }
        if(history.remember(song,"edit")){
            ++edits;bool bad=false;REQUIRE(Song::parse(song.json(),bad));REQUIRE_FALSE(bad);
        }
    }
    REQUIRE(edits==1000);
    auto final=JSON::toString(song.json());
    for(int i=0;i<1000;++i) REQUIRE(history.step(song,false)=="edit");
    CHECK_FALSE(history.canStep(false));
    REQUIRE(JSON::toString(song.json())==initial);
    for(int i=0;i<1000;++i) REQUIRE(history.step(song,true)=="edit");
    CHECK_FALSE(history.canStep(true));
    REQUIRE(JSON::toString(song.json())==final);
}
TEST_CASE("Trim clamps to recorded material and paste handles intersecting edits and gaps"){
    auto s=fixture();
    auto end=s.clips[0].length;
    REQUIRE(trimEdge(s,"g1",true,1200));
    CHECK(s.clips[0].offset==1373);
    REQUIRE(trimEdge(s,"g1",true,-100000));
    CHECK(s.clips[0].position==0);
    REQUIRE(trimEdge(s,"g1",false,400));
    REQUIRE(trimEdge(s,"g1",false,end*4));
    CHECK(s.clips[0].length==end);
    REQUIRE(eraseSpan(s,"g",{200,400}));
    auto f=copySpan(s,"g",{100,500});
    REQUIRE(f.clips.size()==2);
    CHECK(f.clips.back().position==300);
    REQUIRE(pasteAt(s,"d",200,f));
    CHECK(copySpan(s,"d",{300,500}).clips.empty());
    CHECK(snapBeat(s.grid,s.grid.beat(5)+183)==s.grid.beat(5));
}
}
