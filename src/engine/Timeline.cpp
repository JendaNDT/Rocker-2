#include "Timeline.h"
#include <cmath>
#include <numbers>
namespace rocker {
using namespace juce;
static_assert(std::atomic<Frame>::is_always_lock_free && std::atomic<Performance*>::is_always_lock_free);
CaptureQueue::CaptureQueue(int capacity,int channels):buffer(channels,capacity){buffer.clear();}
int CaptureQueue::available() const noexcept {return (int)(written.load()-read.load());}
bool CaptureQueue::push(const float* l,const float* r,int count) noexcept {
    auto w=written.load(std::memory_order_relaxed);
    const int cap=buffer.getNumSamples();
    if(count>cap-(w-read.load(std::memory_order_acquire)))return false;
    const float* inputs[]{l,r};
    for(int offset=0;offset<count;){
        int at=(int)((w+offset)%cap), n=std::min(count-offset,cap-at);
        for(int c=0;c<buffer.getNumChannels();++c){
            if(inputs[c])std::copy_n(inputs[c]+offset,n,buffer.getWritePointer(c,at));
            else buffer.clear(c,at,n);
        }
        offset+=n;
    }
    written.store(w+count,std::memory_order_release);return true;
}
bool CaptureQueue::drain(AudioFormatWriter& writer,int rate) {
    Frame pos=read.load(std::memory_order_relaxed),end=written.load(std::memory_order_acquire);
    while(pos<end){
        int at=(int)(pos%buffer.getNumSamples());
        int n=(int)std::min<Frame>({end-pos,buffer.getNumSamples()-at,rate-(pos-flushed)});
        if(!writer.writeFromAudioSampleBuffer(buffer,at,n))return false;
        pos+=n;
        if(pos-flushed>=rate){if(!writer.flush())return false;flushed=pos;}
        read.store(pos,std::memory_order_release);
    }
    return true;
}
bool AudioLibrary::load(const Song& s,const File& dir) {
    bool ok=true;
    for(auto& a:s.assets){
        if(data.contains(a.id))continue;
        auto reader=waveReader(dir.getChildFile(a.path));
        if(!reader || reader->sampleRate!=s.grid.rate || reader->lengthInSamples<a.length || a.length>std::numeric_limits<int>::max()){ok=false;continue;}
        auto buffer=std::make_shared<AudioBuffer<float>>(a.channels,(int)a.length);
        if(!reader->read(buffer.get(),0,(int)a.length,0,true,true)){ok=false;continue;}
        data[a.id]=buffer;
    }
    return ok;
}
Playback::Playback(const Song& s,const AudioLibrary& lib) {
    const bool solo=std::any_of(s.tracks.begin(),s.tracks.end(),[](auto& t){return t.solo;});
    for(auto& c:s.clips){
        auto t=std::find_if(s.tracks.begin(),s.tracks.end(),[&](auto& track){return track.id==c.track;});
        auto it=lib.data.find(c.asset);
        if(t!=s.tracks.end() && !t->mute && (!solo || t->solo) && it!=lib.data.end())clips.push_back({it->second,c.position,c.offset,c.length});
    }
}
void Playback::mix(float* const* out,int no,int size,Frame at) const noexcept {
    for(const auto& c:clips){
        Frame from=std::max(at,c.at),to=std::min(at+size,c.at+c.length);
        if(to<=from)continue;
        for(int channel=0;channel<std::min(no,2);++channel)if(out[channel]){
            auto* dst=out[channel]+(from-at);
            auto* src=c.wave->getReadPointer(std::min(channel,c.wave->getNumChannels()-1),(int)(c.offset+from-c.at));
            for(Frame i=0;i<to-from;++i)dst[i]+=src[i];
        }
    }
}
Performance::Performance(const Song& s,const AudioLibrary& library,Frame from,bool rec,bool metronome,int count):
    grid(s.grid),playback(s,library),start(from),end(rec?std::numeric_limits<Frame>::max():s.end()),clock(from-s.grid.beat(count)),recording(rec),click(metronome),cursor(clock){
    // Anchor count-in to the punch position; normal beats follow the song grid.
    beatIndex=count?-count:grid.beatAt(from);
    nextBeat=count?clock:grid.beat(beatIndex);
    if(nextBeat<clock)nextBeat=grid.beat(++beatIndex);
}
Performance::~Performance(){finishDisk();}
bool Performance::arm(const Song& s,const File& dir,const StringArray& ports,int delay) {
    if(dir.getChildFile("Zvuk").createDirectory().failed())return false;
    Array<var> entries;
    for(auto& t:s.tracks)if(t.armed){
        Take take;
        take.track=t.id;
        take.left=ports.indexOf(t.inputs[0]);take.right=ports.indexOf(t.inputs[1]);
        if(take.left<0 || (t.inputs.size()==2 && take.right<0))return false;
        take.asset.id=Uuid().toString();take.asset.path="Zvuk/"+take.asset.id+".wav";take.asset.channels=t.inputs.size();
        take.writer=waveWriter(dir.getChildFile(take.asset.path),grid.rate,take.asset.channels);
        if(!take.writer)return false;
        take.queue=std::make_unique<CaptureQueue>(grid.rate*10,take.asset.channels);
        entries.add(object({{"id",take.asset.id},{"stopa",t.id}}));
        takes.push_back(std::move(take));
    }
    if(takes.empty())return false;
    compensation=std::max(0,delay);
    journal=dir.getChildFile("Zachrana").getChildFile(Uuid().toString()+".json");
    if(!atomicJson(journal,object({{"poloha",start},{"kompenzace",compensation},{"sampleRate",grid.rate},{"stopy",entries}})))return false;
    worker=std::thread([this]{
        while(!stopping.load()){
            if(!drain()){failed=true;finished=true;return;}
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        if(!drain())failed=true;
    });
    return true;
}
bool Performance::drain(){for(auto& t:takes)if(!t.queue->drain(*t.writer,grid.rate))return false;return true;}
void Performance::finishDisk(){
    stopping=true;
    if(worker.joinable())worker.join();
    for(auto& t:takes)t.writer.reset();
}
void Performance::process(const float* const* in,int ni,float* const* out,int no,int frames) noexcept {
    if(finished.load())return;
    const int n=(int)(clock+frames>end?end-clock:frames);
    int skip=(int)std::clamp<Frame>(start-clock,0,n);
    if(n>skip){
        float* outputs[]{no>0 && out[0]?out[0]+skip:nullptr,no>1 && out[1]?out[1]+skip:nullptr};
        playback.mix(outputs,std::min(2,no),n-skip,clock+skip);
        for(auto& t:takes){
            auto channel=[&](int c){return c>=0 && c<ni && in[c]?in[c]+skip:nullptr;};
            if(!t.queue->push(channel(t.left),channel(t.right),n-skip)){drop=clock+skip;finished=true;break;}
        }
    }
    for(int i=0;i<n;++i){
        Frame now=clock+i;
        if(now==nextBeat){
            accent=beatIndex%grid.beats==0;
            clickAge=(click || now<start)?0:-1;
            ++beatIndex;
            if(now<start && beatIndex<=0){
                nextBeat=start+grid.beat(beatIndex);
                if(beatIndex==0){beatIndex=grid.beatAt(start);nextBeat=grid.beat(beatIndex);if(nextBeat<start)nextBeat=grid.beat(++beatIndex);}
            }else nextBeat=grid.beat(beatIndex);
        }
        if(clickAge>=0){
            float pulse=(float)(0.16*std::exp(-clickAge/(grid.rate*.0025))*std::cos(2*std::numbers::pi*(accent?1600:1000)*clickAge/grid.rate));
            for(int c=0;c<std::min(2,no);++c)if(out[c])out[c][i]+=pulse;
            if(++clickAge>=grid.rate/80)clickAge=-1;
        }
    }
    clock+=n;cursor.store(clock);
    if(clock>=end)finished=true;
}
void Timeline::detach() noexcept {
    active.store(nullptr);
    while(reading.load())std::this_thread::yield();
}
void Timeline::process(const float* const* in,int ni,float* const* out,int no,int frames) noexcept {
    reading.store(true);
    if(auto* p=active.load())p->process(in,ni,out,no,frames);
    reading.store(false);
}
bool recoverTake(Song& song,const File& dir,const File& journal){
    auto record=JSON::parse(journal);
    auto entries=record.getProperty("stopy",{});
    if(!entries.isArray() || !record.hasProperty("poloha") || (int)record.getProperty("sampleRate",0)!=song.grid.rate)return false;
    const Frame at=record.getProperty("poloha",-1),delay=record.getProperty("kompenzace",-1);
    if(at<0 || delay<0)return false;
    auto next=song;
    for(auto& e:*entries.getArray()){
        auto id=e.getProperty("id",{}).toString(), track=e.getProperty("stopa",{}).toString();
        if(id.length()!=32 || !id.containsOnly("0123456789abcdefABCDEF"))return false;
        if(std::none_of(next.tracks.begin(),next.tracks.end(),[&](auto& t){return t.id==track;}))return false;
        if(std::any_of(next.assets.begin(),next.assets.end(),[&](auto& a){return a.id==id;}))continue;
        auto path="Zvuk/"+id+".wav";
        auto reader=waveReader(dir.getChildFile(path));
        if(!reader || reader->sampleRate!=song.grid.rate)return false;
        auto length=reader->lengthInSamples;
        if(length<=delay)continue;
        next.assets.push_back({id,path,length,(int)reader->numChannels});
        next.clips.push_back({id,track,id,at,delay,length-delay});
    }
    next.bars=std::max(next.bars,(int)(next.grid.beatAt(std::max<Frame>(0,next.end()-1))/next.grid.beats+1));
    if(!next.save(dir))return false;
    song=std::move(next);
    return journal.deleteFile();
}
}
