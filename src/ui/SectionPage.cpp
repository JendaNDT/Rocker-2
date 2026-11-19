#include "Studio.h"
#include "Strings.h"

namespace rocker {
using namespace juce;
namespace s=style;
namespace {
const StringArray colours{"uvod","sloka","refren","most","solo","zaver"};
int palette(const String& key) { return std::max(0,colours.indexOf(key)); }
Colour sectionInk(int index) {
    const Colour inks[]{s::intro,s::verse,s::chorus,s::bridge,s::solo,s::ending};
    return inks[index];
}
}
void Studio::selectOccurrence(int index) {
    if (index<0 || (song.order.empty() ? index!=0 : index>=(int)song.order.size())) return;
    selectedOccurrence=index;
    askingRepair=soundDeckOpen=browsingVersions=false;
    int start=song.startBar(index), length=song.order.empty()?song.bars:song.order[index].length;
    sectionDraft=song.order.empty()?cs::verse:song.sections.at(song.order[index].section).name;
    pick({song.barFrame(start),song.barFrame(start+length)});
    view.cursor=view.selectionStart;
    triggerAsyncUpdate();
}
void Studio::tapeAction(TapeAction action,int value) {
    if (performance || selectedOccurrence<0) return;
    int index=selectedOccurrence;
    song.initOrder();
    if (index>=(int)song.order.size()) return;
    auto& o=song.order[index];
    auto& section=song.sections.at(o.section);
    String label;
    switch(action) {
    case TapeAction::split:
        if(!song.splitSection(cutBar)) return;
        index=song.occurrenceAt(cutBar);
        label=cs::sectionSplit;
        break;
    case TapeAction::repeat:
        if(!song.repeatSection(index,value)) return;
        index=value;
        label=cs::sectionRepeat;
        break;
    case TapeAction::detach:
        if(!song.detachSection(index)) return;
        label=cs::sectionDetach;
        break;
    case TapeAction::restore:
    case TapeAction::resize:
        if(!song.resizeOccurrence(index,action==TapeAction::restore ? section.bars-o.crop : jlimit(1,section.bars-o.crop,value))) return;
        label=cs::sectionResize;
        break;
    case TapeAction::name:
        if(sectionDraft.trim().isEmpty()) return;
        section.name=sectionDraft.trim();
        label=cs::sectionName;
        break;
    case TapeAction::colour:
        section.colour=colours[value];
        label=cs::sectionColour;
        break;
    case TapeAction::move: {
        int destination=value>index ? value-1 : value;
        if(destination==index || !isPositiveAndBelow(destination,(int)song.order.size())) return;
        auto item=o;
        song.order.erase(song.order.begin()+index);
        song.order.insert(song.order.begin()+destination,item);
        song.projectOrder();
        index=destination;
        label=cs::sectionMove;
        break;
    }
    }
    selectOccurrence(index);
    reveal(view.cursor);
    saveSong(label);
    view.save(projectDir);
    status(label);
}
void Studio::confirmRepair(bool independent) {
    askingRepair=false;
    if(independent) {
        for(int i=0;i<(int)song.order.size();++i) {
            int start=song.startBar(i);
            if(selection().from<song.barFrame(start+song.order[i].length) && selection().to>song.barFrame(start))
                song.detachSection(i);
        }
        saveSong(cs::sectionDetach);
    }
    selectedOccurrence=-1;
    startProject(true,true);
}
void Studio::sectionDeck(Rect area) {
    auto top=area.removeFromTop(s::button);
    auto actions=area.removeFromTop(s::button+s::grid).withTrimmedTop(s::grid);
    auto add=[&](String name,Handler fn,bool enabled=true) {
        button(name,actions.removeFromLeft(s::clipWidth),std::move(fn))->setEnabled(enabled && !performance);
        actions.removeFromLeft(s::grid);
    };
    if (askingRepair) {
        text(cs::repeatWarning,top,s::heading);
        add(cs::sharedRepair,[this]{confirmRepair(false);});
        add(cs::ownRepair,[this]{confirmRepair(true);});
        add(cs::sectionCancel,[this]{askingRepair=false;triggerAsyncUpdate();});
        return;
    }
    if(selectedOccurrence<0 || (!song.order.empty() && selectedOccurrence>=(int)song.order.size())) return;
    int index=selectedOccurrence, start=song.startBar(index);
    int length=song.order.empty()?song.bars:song.order[index].length;
    int full=song.order.empty()?length:song.sections.at(song.order[index].section).bars-song.order[index].crop;
    field(cs::sectionName,top.removeFromLeft(s::fieldWidth),sectionDraft);
    button(cs::sectionRename,top.removeFromLeft(s::clipWidth),[this]{tapeAction(TapeAction::name);});
    top.removeFromLeft(s::gap);
    combo(cs::sectionColour,top,cs::sectionNames,song.order.empty()?1:palette(song.sections.at(song.order[index].section).colour),
          [this](int i){tapeAction(TapeAction::colour,i);});
    add(cs::sectionSplit,[this]{tapeAction(TapeAction::split);},cutBar>start && cutBar<start+length);
    add(cs::sectionRepeat,[this,index]{tapeAction(TapeAction::repeat,index+1);});
    add(cs::sectionDetach,[this]{tapeAction(TapeAction::detach);},song.linked(index));
    add(cs::sectionRestore,[this]{tapeAction(TapeAction::restore);},length<full);
    text(String(length)+cs::outOf+String(full)+" "+cs::totalBars,actions,s::body);
}
void Studio::drawSections(Graphics& g) {
    auto axis=sectionStrip.withTrimmedLeft(s::trackHead);
    Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(axis);
    auto draw=[&](int i,int bar,int length,int full,const String& name,const String& colour,bool linked) {
        int a=pixelAt(song.barFrame(bar)), b=pixelAt(song.barFrame(bar+length));
        auto box=axis.withLeft(std::max(axis.getX(),a)).withRight(std::min(axis.getRight(),b)).reduced(s::hairline);
        if (box.isEmpty()) return;
        auto ink=sectionInk(palette(colour));
        auto tape=box.toFloat();
        Path p;
        p.startNewSubPath(tape.getX(),tape.getY());
        p.lineTo(tape.getRight(),tape.getY());
        p.lineTo(tape.getRight(),tape.getBottom()-s::tapeCut);
        p.lineTo(tape.getRight()-s::tapeCut,tape.getBottom());
        p.lineTo(tape.getX(),tape.getBottom());
        p.closeSubPath();
        g.setColour(ink);g.fillPath(p);
        if(selectedOccurrence==i) { g.setColour(s::selection);g.strokePath(p,PathStrokeType(s::stroke)); }
        if(length<full) {
            auto stripe=box.removeFromRight(std::min(s::icon,box.getWidth()/3));
            Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(stripe);
            g.setColour(s::panel);
            for(int x=stripe.getX()-stripe.getHeight();x<stripe.getRight();x+=s::grid)
                g.drawLine((float)x,(float)stripe.getBottom(),(float)(x+stripe.getHeight()),(float)stripe.getY(),s::hairline);
        }
        g.setColour(s::background);g.setFont(skin.font(s::body,false,true));
        g.drawFittedText(name+(linked?" "+cs::linkedMark:String{})+"   "+String(length)+(length<full?cs::outOf+String(full):String{}),
                         box.reduced(s::grid,0),Justification::centredLeft,1);
    };
    int bar=0;
    if(song.order.empty()) draw(0,0,song.bars,song.bars,cs::verse,"sloka",false);
    for(int i=0;i<(int)song.order.size();++i) {
        const auto& o=song.order[i];
        const auto& sec=song.sections.at(o.section);
        draw(i,bar,o.length,sec.bars-o.crop,sec.name,sec.colour,song.linked(i));
        bar+=o.length;
    }
    if(dragged && (drag==Drag::tapeEdge || drag==Drag::tapeMove)) {
        g.setColour(s::chalk);
        g.fillRect(pixelAt(song.barFrame(targetBar)),axis.getY(),s::stroke,axis.getHeight());
    }
}
void Studio::sectionDown(const MouseEvent& e) {
    Frame at=frameAt(e.x);
    int bar=(int)(song.grid.beatAt(at)/song.grid.beats);
    cutBar=bar+(at-song.barFrame(bar)>=song.barFrame(bar+1)-at);
    int index=song.order.empty()?0:song.occurrenceAt(bar);
    int prior=cutBar>0?song.occurrenceAt(cutBar-1):-1;
    bool edge=std::abs(e.x-pixelAt(song.barFrame(cutBar)))<=s::grid && prior>=0 &&
        song.startBar(prior)+song.order[prior].length==cutBar;
    if (edge) index=prior;
    if(index<0 || bar>song.bars) return;
    selectOccurrence(index);
    dragged=false;
    copyingSection=e.mods.isCtrlDown() || e.mods.isCommandDown();
    drag=edge && !copyingSection?Drag::tapeEdge:Drag::tapeMove;
    targetBar=cutBar;
    if (e.mods.isPopupMenu()) {
        drag=Drag::none;
        PopupMenu menu;
        menu.addItem(1,cs::sectionRestore);
        menu.addItem(2,cs::sectionDetach,song.linked(index));
        Component::SafePointer<Studio> safe(this);
        menu.showMenuAsync(PopupMenu::Options{}.withTargetComponent(this),[safe,index](int action){
            if(safe && safe->selectedOccurrence==index && action)
                safe->tapeAction(action==1?TapeAction::restore:TapeAction::detach);
        });
    }
}
}
