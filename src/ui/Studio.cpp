#include "Studio.h"
#include "Strings.h"
#include <BinaryData.h>
namespace rocker {
namespace s = style;
using namespace juce;
namespace {
using Rect = Rectangle<int>;
float width(const Font& f,const String& t) { return GlyphArrangement::getStringWidth(f,t); }
String shortened(const Font& font, String text, int available) {
    if (width(font,text) <= available) return text;
    while (text.isNotEmpty() && width(font,text+String(u8"…")) > available) text = text.dropLastCharacters(1);
    return text+String(u8"…");
}
bool builtIn(const String& name) {
    return name.containsIgnoreCase("built-in") || name.containsIgnoreCase("internal mic")
        || name.containsIgnoreCase("macbook") || name.containsIgnoreCase("realtek")
        || name.containsIgnoreCase("microphone array") || name.containsIgnoreCase(String(u8"mikrofonní pole"));
}
enum ButtonStyle { plain, primary, recording, newSong, clipButton, tunerButton, selected };
class Action final : public Button {
public:
    Action(const String& title, int kind, String key, Theme& theme)
        : Button(title), mode(kind), shortcut(std::move(key)), skin(theme) { setWantsKeyboardFocus(true); }
    Rect titleArea() const {
        auto area=getLocalBounds().reduced(s::inset,0);
        if (mode==recording || mode==newSong || mode==tunerButton) area.removeFromLeft(s::icon+s::gap);
        if (shortcut.isNotEmpty()) area.removeFromRight(s::keyWidth);
        return area;
    }
    Font titleFont() const { return skin.font(mode==recording || mode==newSong ? s::heading : s::body,false,true); }
    void paintButton(Graphics& g,bool over,bool down) override {
        auto bounds=getLocalBounds().toFloat().reduced(s::hairline);
        const bool active = mode==primary || mode==selected;
        g.setColour(mode==clipButton ? s::clipping : active ? s::chalk : (down || over ? s::barGrid : s::raised));
        g.fillRoundedRectangle(bounds,s::radius);
        g.setColour(hasKeyboardFocus(true) ? s::selection : s::line);
        g.drawRoundedRectangle(bounds,s::radius,hasKeyboardFocus(true)?s::stroke:s::hairline);
        g.setColour(active || mode==clipButton ? s::background : s::chalk);
        auto area=getLocalBounds().reduced(s::inset,0);
        auto symbol=area.withWidth(s::icon).withSizeKeepingCentre(s::icon,s::icon).toFloat();
        if (mode==recording) {
            g.setColour(s::record); g.fillEllipse(symbol.reduced(s::stroke)); g.setColour(s::chalk);
        } else if (mode==newSong) {
            g.drawLine(symbol.getX(),symbol.getCentreY(),symbol.getRight(),symbol.getCentreY(),s::stroke);
            g.drawLine(symbol.getCentreX(),symbol.getY(),symbol.getCentreX(),symbol.getBottom(),s::stroke);
        } else if (mode==tunerButton) {
            auto fork=symbol.reduced(s::grid,0);
            g.drawLine(fork.getX(),fork.getY(),fork.getX(),fork.getCentreY(),s::stroke);
            g.drawLine(fork.getRight(),fork.getY(),fork.getRight(),fork.getCentreY(),s::stroke);
            g.drawLine(fork.getX(),fork.getCentreY(),fork.getRight(),fork.getCentreY(),s::stroke);
            g.drawLine(fork.getCentreX(),fork.getCentreY(),fork.getCentreX(),fork.getBottom(),s::stroke);
        }
        g.setFont(titleFont());
        g.drawText(getName(),titleArea(),mode==recording || mode==newSong ? Justification::centredLeft : Justification::centred,false);
        if (shortcut.isNotEmpty()) {
            g.setFont(skin.font(s::caption,true)); g.setColour(s::muted);
            g.drawText(shortcut,area.removeFromRight(s::keyWidth),Justification::centredRight,false);
        }
    }
private:
    int mode; String shortcut; Theme& skin;
};
class InputRow final : public Component {
public:
    InputRow(Theme& theme, Connection& connection, int index, const String& name,
             std::function<void(String)> rename) : skin(theme), link(connection), channel(index),
             clip(cs::clip,clipButton,{},theme) {
        addAndMakeVisible(editor);
        editor.setName(cs::channel(index));
        editor.setFont(skin.font(s::body));
        editor.setTextToShowWhenEmpty(cs::nameHint,s::muted);
        editor.setText(name,false);
        editor.onTextChange=[this,rename] { rename(editor.getText()); };
        addChildComponent(clip);
        clip.onClick=[this] { link.audio.clearClip(channel); update(); };
        update();
    }
    void update() {
        reading=link.audio.read(channel);
        clip.setVisible(reading.clipped);
        repaint();
    }
    void resized() override {
        auto area=getLocalBounds().reduced(s::inset,0).withSizeKeepingCentre(getWidth()-2*s::inset,s::button);
        area.removeFromLeft(s::levelWidth);
        editor.setBounds(area.removeFromLeft(s::nameWidth));
        clip.setBounds(area.removeFromRight(s::clipWidth));
    }
    void paint(Graphics& g) override {
        g.fillAll(s::panel);
        auto area=getLocalBounds().reduced(s::inset,0);
        g.setFont(skin.font(s::body,true));
        g.setColour(s::chalk);
        g.drawText(cs::channel(channel),area.removeFromLeft(s::levelWidth),Justification::centredLeft);
        area.removeFromLeft(s::nameWidth+s::gutter);
        area.removeFromRight(s::clipWidth+s::gap);
        auto value=area.removeFromRight(s::levelWidth);
        const String label=reading.held>0 ? String(decibels(reading.held),1).replaceCharacter('.',',')+cs::db : String(cs::silence);
        g.drawText(label,value,Justification::centred,false);
        auto meter=area.withSizeKeepingCentre(area.getWidth(),s::gap);
        g.setColour(s::raised); g.fillRect(meter);
        auto extent=[&](float level) { return roundToInt(meter.getWidth()*jlimit(0.0f,1.0f,(decibels(level)+60.0f)/60.0f)); };
        g.setColour(reading.clipped?s::clipping:s::ready);
        g.fillRect(meter.withWidth(extent(reading.peak)));
        if (reading.held>0) {
            g.setColour(s::chalk);
            g.fillRect(meter.withX(meter.getX()+std::max(0,extent(reading.held)-s::stroke)).withWidth(s::stroke));
        }
        g.setColour(s::line); g.fillRect(getLocalBounds().removeFromBottom(s::hairline));
    }
    bool fits() const {
        return width(skin.font(s::body,true),cs::channel(channel))<=s::levelWidth
            && width(skin.font(s::body,true),"-144.0 dBFS")<=s::levelWidth
            && width(clip.titleFont(),clip.getName())<=clip.titleArea().getWidth()
            && editor.getRight()+s::gutter+s::levelWidth<clip.getX();
    }
private:
    Theme& skin; Connection& link; int channel; Reading reading{};
    TextEditor editor; Action clip;
};
}
Theme::Theme() {
    regular=Typeface::createSystemTypefaceFor(BinaryData::Inter_ttf,BinaryData::Inter_ttfSize);
    heavy=Typeface::createSystemTypefaceFor(BinaryData::InterSemiBold_ttf,BinaryData::InterSemiBold_ttfSize);
    digits=Typeface::createSystemTypefaceFor(BinaryData::Mono_ttf,BinaryData::Mono_ttfSize);
    setColour(ComboBox::textColourId,s::chalk);
    setColour(PopupMenu::backgroundColourId,s::panel);
    setColour(PopupMenu::textColourId,s::chalk);
    setColour(PopupMenu::highlightedBackgroundColourId,s::raised);
    setColour(PopupMenu::highlightedTextColourId,s::chalk);
    setColour(TextEditor::backgroundColourId,s::raised);
    setColour(TextEditor::textColourId,s::chalk);
    setColour(TextEditor::outlineColourId,s::line);
    setColour(TextEditor::focusedOutlineColourId,s::selection);
    setColour(TextEditor::highlightColourId,s::selection.withAlpha(s::selectionOpacity));
    setColour(CaretComponent::caretColourId,s::chalk);
    setColour(ScrollBar::thumbColourId,s::faint);
}
Font Theme::font(float size,bool mono,bool bold) const {
    auto face=mono?digits:(bold?heavy:regular);
    return Font(FontOptions(face).withPointHeight(size).withFallbackEnabled(false));
}
Font Theme::getPopupMenuFont() { return font(s::body); }
Font Theme::getComboBoxFont(ComboBox& c) { return font(s::body,c.getProperties().contains("number")); }
void Theme::positionComboBoxText(ComboBox& c,Label& l) {
    l.setBounds(c.getLocalBounds().reduced(s::gap,0).withTrimmedRight(s::icon));
    l.setFont(getComboBoxFont(c));
    l.setBorderSize({});
}
void Theme::drawComboBox(Graphics& g,int w,int h,bool,int,int,int,int,ComboBox& c) {
    auto r=Rectangle<float>(0,0,(float)w,(float)h).reduced(s::hairline);
    g.setColour(s::raised); g.fillRoundedRectangle(r,s::radius);
    g.setColour(c.hasKeyboardFocus(true)?s::selection:s::line); g.drawRoundedRectangle(r,s::radius,s::hairline);
    Path p; auto x=w-s::inset; auto y=h/2;
    p.startNewSubPath(x-s::grid,y-s::stroke); p.lineTo(x,y+s::stroke); p.lineTo(x+s::grid,y-s::stroke);
    g.setColour(s::chalk); g.strokePath(p,PathStrokeType(s::stroke));
}
Studio::Studio(DeviceSystem& sys,File path)
    : devices(sys),settingsFile(std::move(path)),settings(Settings::load(settingsFile)),link(sys),
      page(settings.visited?Page::home:Page::device) {
    setLookAndFeel(&skin); setWantsKeyboardFocus(true);
    viewport.setViewedComponent(&rows,false); viewport.setScrollBarsShown(true,false);
    addChildComponent(viewport);
    devices.changed=[this] { refresh(true); };
    refresh(false);
    setSize(s::minWidth,s::minHeight);
    startTimerHz(30);
}
Studio::~Studio() {
    stopTimer(); cancelPendingUpdate(); devices.changed={};
    controls.clear(); viewport.setViewedComponent(nullptr,false); setLookAndFeel(nullptr);
}
void Studio::persist() {
    if (!settings.save(settingsFile)) message=cs::writeError;
}
void Studio::refresh(bool notify) {
    link.rescan(settings.device);
    laptop=link.online && (builtIn(link.current.input) || (settings.laptopInput.isNotEmpty() && settings.laptopInput==link.current.input));
    if (laptop && link.current.driver!=Driver::core && link.current.driver!=Driver::shared
        && settings.device.input.isEmpty()) {
        for (const auto& p:link.ports) {
            if (p.driver!=Driver::shared || !p.inputs.contains(link.current.input)) continue;
            const auto previous=link.current;
            auto shared=previous;
            shared.driver=Driver::shared;
            if (!link.select(shared)) link.select(previous);
            break;
        }
    }
    if (notify) {
        message=cs::unplugged;
        if (page!=Page::home) page=Page::device;
    }
    triggerAsyncUpdate();
}
void Studio::choose(DeviceChoice d) {
    const bool ok=link.select(d);
    message=ok ? String{} : String(cs::failed);
    if (ok) { settings.device=link.current; persist(); }
    laptop=builtIn(link.current.input) || (link.current.input.isNotEmpty() && settings.laptopInput==link.current.input);
    triggerAsyncUpdate();
}
void Studio::finish() {
    show(Page::home);
    settings.visited=true;
    if (link.online) settings.device=link.current;
    message=cs::saved; persist();
}
void Studio::show(Page value) {
    link.audio.stopTone(); page=value; answer=-1; played=false; message={};
    triggerAsyncUpdate();
}
void Studio::status(String value) { message=std::move(value); triggerAsyncUpdate(); }
void Studio::timerCallback() {
    if (page==Page::inputs)
        for (auto* c : rows.getChildren()) if (auto* row=dynamic_cast<InputRow*>(c)) row->update();
}
void Studio::handleAsyncUpdate() { rebuild(); }
void Studio::resized() { rebuild(); }
void Studio::paint(Graphics& g) {
    g.fillAll(s::background);
    g.setColour(s::panel); g.fillRect(getLocalBounds().removeFromTop(s::header));
    for (auto r:panels) { g.setColour(s::panel); g.fillRoundedRectangle(r.toFloat(),s::radius); }
    g.setColour(s::line); g.fillRect(0,s::header,getWidth(),s::hairline);
    g.fillRect(0,getHeight()-s::footer,getWidth(),s::hairline);
    for (const auto& item:textLines) {
        g.setColour(item.ink); g.setFont(item.font);
        g.drawText(item.value,item.bounds,item.right?Justification::centredRight:Justification::centredLeft,false);
    }
}
void Studio::text(String value,Rect r,int size,Colour ink,bool mono,bool bold,bool right) {
    auto f=skin.font(size,mono,bold);
    if (size==s::section) f.setExtraKerningFactor(s::tracking);
    textLines.push_back({std::move(value),r,f,ink,right});
}
void Studio::button(const String& name,Rect r,Handler handler,int look,String key) {
    auto* b=new Action(name,look,std::move(key),skin);
    controls.add(b); addAndMakeVisible(b); b->setBounds(r); b->onClick=std::move(handler);
}
void Studio::combo(const String& title,Rect r,const StringArray& items,int selectedIndex,
                   std::function<void(int)> change,bool mono) {
    text(title,r.removeFromLeft(s::labelWidth),s::body,s::muted);
    auto* box=new ComboBox;
    controls.add(box); addAndMakeVisible(box); box->setName(title); box->setBounds(r.withWidth(s::fieldWidth));
    if (mono) { box->getProperties().set("number",true); box->lookAndFeelChanged(); }
    const auto font=skin.getComboBoxFont(*box);
    for (int i=0;i<items.size();++i)
        box->addItem(shortened(font,items[i],box->getWidth()-s::icon-2*s::gap),i+1);
    box->setTextWhenNothingSelected(cs::noPort);
    box->setSelectedItemIndex(selectedIndex,dontSendNotification);
    box->setEnabled(items.size()>1);
    box->setTooltip(items[selectedIndex]);
    box->onChange=[box,change] { change(box->getSelectedItemIndex()); };
}
void Studio::rebuild() {
    controls.clear(); textLines.clear(); panels.clear(); viewport.setVisible(false);
    if (getWidth()<=0) return;
    auto area=getLocalBounds().reduced(s::margin,0);
    area=area.withSizeKeepingCentre(std::min(area.getWidth(),s::maxWidth),area.getHeight());
    auto header=area.removeFromTop(s::header);
    auto brandFont=skin.font(s::counter,false,true);
    const int brandWidth=roundToInt(std::ceil(width(brandFont,cs::brand)))+s::gap;
    text(cs::brand,header.removeFromLeft(brandWidth),s::counter,s::chalk,false,true);
    text(cs::version,header.removeFromLeft(s::keyWidth),s::counter,s::muted,true);
    auto indicator=link.online ? String(cs::device)+String(u8"  ·  ")+cs::drivers[(int)link.current.driver]+"  /  "+link.current.output
                               : String(cs::disconnected);
    text(shortened(skin.font(s::caption),indicator,header.getWidth()),header,s::caption,s::muted,false,false,true);
    if (page==Page::home) makeHome(area); else makeWizard(area);
    repaint();
}
void Studio::makeHome(Rect area) {
    auto footer=area.removeFromBottom(s::footer).reduced(0,(s::footer-s::button)/2);
    button(cs::setup,footer.removeFromLeft(s::nameWidth),[this]{show(Page::device);});
    footer.removeFromLeft(s::gap);
    button(cs::tuner,footer.removeFromLeft(s::nameWidth),[this]{status(cs::coming(1));},tunerButton,"T");
    text(message,footer.reduced(s::inset,0),s::body,message==cs::saved?s::ready:s::warning,false,false,true);
    area.reduce(0,s::gutter);
    auto actions=area.removeFromTop(s::recordButton);
    int card=(actions.getWidth()-s::inset)/2;
    button(cs::record,actions.removeFromLeft(card),[this]{status(cs::coming(8));},recording,"R");
    actions.removeFromLeft(s::inset);
    button(cs::song,actions,[this]{status(cs::coming(2));},newSong,"N");
    area.removeFromTop(s::gutter);
    for (int i=0;i<2;++i) {
        auto p=area.removeFromTop((area.getHeight()-(i==0?s::gap:0))/(2-i));
        panels.push_back(p); p.reduce(s::gutter,s::inset);
        auto h=p.removeFromTop(s::transport);
        text(String(i?cs::ideas:cs::resume).toUpperCase(),h,s::section,s::muted,false,true);
        auto b=h.removeFromRight(s::nameWidth).withSizeKeepingCentre(s::nameWidth,s::button);
        if (i) button(cs::library,b,[this]{status(cs::coming(8));});
        else button(cs::resume,b,[this]{status(cs::coming(2));},plain,"Enter");
        text(i?cs::emptyIdeas:cs::emptySongs,p,s::heading,s::chalk);
        area.removeFromTop(s::gap);
    }
}
void Studio::makeWizard(Rect area) {
    auto footer=area.removeFromBottom(s::footer).reduced(0,(s::footer-s::button)/2);
    button(cs::home,footer.removeFromLeft(s::nameWidth),[this]{finish();});
    auto primaryArea=footer.removeFromRight(s::nameWidth);
    const bool last=page==Page::inputs;
    auto advance=[this] { if (page==Page::inputs) finish(); else show(static_cast<Page>((int)page+1)); };
    button(last?cs::finish:cs::next,primaryArea,advance,primary);
    footer.removeFromRight(s::gap);
    button(cs::skip,footer.removeFromRight(s::levelWidth),advance);
    if (page!=Page::device) {
        footer.removeFromRight(s::gap);
        button(cs::back,footer.removeFromRight(s::levelWidth),[this]{show(static_cast<Page>((int)page-1));});
    }
    area.reduce(0,s::gutter);
    auto heading=area.removeFromTop(s::target);
    text(String(cs::steps).toUpperCase(),heading.removeFromLeft(s::levelWidth),s::section,s::muted,false,true);
    text(String((int)page)+" / 3",heading,s::body,s::muted,true);
    text(cs::titles[(int)page-1],area.removeFromTop(s::recordButton),s::title,s::chalk,false,true);
    area.removeFromTop(s::gap);
    auto notice=area.removeFromBottom(s::transport);
    text(message,notice,s::body,s::warning);
    panels.push_back(area); area.reduce(s::gutter,s::inset);
    if (page==Page::device) devicePage(area);
    if (page==Page::tone) tonePage(area);
    if (page==Page::inputs) inputPage(area);
}
void Studio::devicePage(Rect area) {
    auto row=[&] { auto r=area.removeFromTop(s::button); area.removeFromTop(s::gap); return r; };
    StringArray labels;
    int index=-1;
    for (auto& p:link.ports) {
        if (p.driver==link.current.driver) index=labels.size();
        labels.add(cs::drivers[(int)p.driver]);
    }
    combo(cs::driver,row(),labels,index,[this](int i){choose({link.ports.at(i).driver});});
    if (index>=0) {
        const auto& p=link.ports[(size_t)index];
        if (!p.inputs.isEmpty()) combo(p.separate?cs::inputPort:cs::bothPort,row(),p.inputs,p.inputs.indexOf(link.current.input),
            [this,names=p.inputs](int i){auto d=link.current; d.input=names[i]; choose(d);});
        if ((p.separate || p.inputs.isEmpty()) && !p.outputs.isEmpty())
            combo(cs::outputPort,row(),p.outputs,p.outputs.indexOf(link.current.output),
                [this,names=p.outputs](int i){auto d=link.current; d.output=names[i]; choose(d);});
        if (link.online) {
            StringArray rates,buffers;
            for (auto n:link.caps.rates) rates.add(cs::frequency(n));
            for (auto n:link.caps.buffers) buffers.add(cs::bufferSize(n));
            combo(cs::rate,row(),rates,link.caps.rates.indexOf(link.current.rate),
                  [this](int i){auto d=link.current; d.rate=(int)link.caps.rates[i]; choose(d);},true);
            combo(cs::buffer,row(),buffers,link.caps.buffers.indexOf(link.current.buffer),
                  [this](int i){auto d=link.current; d.buffer=link.caps.buffers[i]; choose(d);},true);
        }
    }
    auto actions=row();
    button(cs::refresh,actions.removeFromLeft(s::nameWidth),[this]{refresh(true);});
    actions.removeFromLeft(s::gap);
    if (link.online && link.current.driver!=Driver::asio && link.current.input.isNotEmpty())
        button(laptop?cs::card:cs::laptop,actions.removeFromLeft(s::fieldWidth),[this] {
            bool enable=!laptop;
            if (enable) for (auto& p:link.ports) if (p.driver==Driver::shared) {
                DeviceChoice d=link.current; d.driver=Driver::shared; choose(d); break;
            }
            laptop=enable; settings.laptopInput=enable?link.current.input:String{}; persist(); triggerAsyncUpdate();
        });
    text(!link.online?cs::connect:(laptop?cs::fallback:cs::hints[(int)link.current.driver]),area.removeFromTop(s::button),
         s::body,laptop || !link.online ? s::warning:s::muted);
    if (link.online) text(cs::bufferHelp,area.removeFromTop(s::button),s::caption,s::muted);
}
void Studio::tonePage(Rect area) {
    const auto outputs=link.caps.outputChannels;
    text(outputs==0?cs::noOutput:(outputs==1?cs::mono:cs::stereo),area.removeFromTop(s::transport));
    if (outputs==0) return;
    auto action=area.removeFromTop(s::transport);
    button(played?cs::again:cs::play,action.removeFromLeft(s::labelWidth),[this]{
        link.audio.playTone(); played=true; answer=-1; triggerAsyncUpdate();
    },primary);
    text(cs::toneInfo,action.reduced(s::inset,0),s::caption,s::muted,true);
    if (!played) return;
    area.removeFromTop(s::gutter);
    text(cs::question,area.removeFromTop(s::recordButton),s::heading,s::chalk,false,true);
    auto options=area.removeFromTop(s::button);
    for (int i=0;i<3;++i) {
        button(cs::answers[i],options.removeFromLeft(s::clipWidth),[this,i]{answer=i;triggerAsyncUpdate();},i==answer?selected:plain);
        options.removeFromLeft(s::gap);
    }
    if (answer>=0) text(cs::advice[answer],area.removeFromTop(s::recordButton),s::body,answer==0?s::ready:s::warning);
}
void Studio::inputPage(Rect area) {
    text(cs::inputHelp,area.removeFromTop(s::button));
    text(cs::clipHelp,area.removeFromTop(s::button),s::caption,s::muted);
    if (link.caps.inputs.isEmpty()) {text(cs::noInputs,area,s::body,s::warning); return;}
    viewport.setBounds(area); viewport.setVisible(true);
    const int count=link.caps.inputs.size();
    const int w=area.getWidth()-viewport.getScrollBarThickness();
    rows.setSize(w,count*s::row);
    for (int i=0;i<count;++i) {
        const auto key=Settings::inputKey(link.current,i);
        auto* row=new InputRow(skin,link,i,settings.aliases[key],[this,key](String name){
            settings.aliases[key]=name; persist();
        });
        controls.add(row); rows.addAndMakeVisible(row); row->setBounds(0,i*s::row,w,s::row);
    }
}
bool Studio::keyPressed(const KeyPress& key) {
    if (page!=Page::home) {
        if (key==KeyPress::escapeKey) {finish();return true;}
        return false;
    }
    if (key==KeyPress::returnKey) { status(cs::coming(2)); return true; }
    switch (key.getKeyCode()) {
        case 'R': case 'U': status(cs::coming(8)); return true;
        case 'N': status(cs::coming(2)); return true;
        case 'T': status(cs::coming(1)); return true;
        default: return false;
    }
}
StringArray Studio::layoutErrors() const {
    StringArray errors;
    for (auto& l:textLines)
        if (width(l.font,l.value)>l.bounds.getWidth() || l.font.getHeight()>l.bounds.getHeight()
            || !getLocalBounds().contains(l.bounds)) errors.add(l.value);
    for (auto* c:controls) {
        if (auto* a=dynamic_cast<Action*>(c))
            if (width(a->titleFont(),a->getName())>a->titleArea().getWidth()
                || a->titleFont().getHeight()>a->getHeight() || !getLocalBounds().contains(a->getBounds())) errors.add(a->getName());
        if (auto* b=dynamic_cast<ComboBox*>(c))
            if (width(const_cast<Theme&>(skin).getComboBoxFont(*b),b->getText())>b->getWidth()-s::icon-2*s::gap)
                errors.add(b->getName());
        if (auto* row=dynamic_cast<InputRow*>(c)) if (!row->fits()) errors.add(cs::input);
    }
    return errors;
}
}
