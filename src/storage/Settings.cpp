#include "storage/AtomicFile.h"
#include "Settings.h"
namespace rocker {
juce::File Settings::location() {
    auto folder = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    folder = folder.getChildFile("Application Support");
#endif
    return folder.getChildFile("Rocker 2").getChildFile("nastaveni.json");
}
juce::String Settings::inputKey(const DeviceChoice& d, int channel) {
    return juce::String(static_cast<int>(d.driver)) + "/" + juce::String(channel) + "/" + d.input;
}
juce::String Settings::calibrationKey(const DeviceChoice& d) {
    juce::Array<juce::var> parts{(int)d.driver, d.input, d.output, d.rate, d.buffer};
    return juce::JSON::toString(juce::var(parts), true);
}
Settings Settings::load(const juce::File& file) {
    Settings result;
    if (!file.existsAsFile() || file.getSize() > 4 * 1024 * 1024)
        return result;
    auto data = juce::JSON::parse(file);
    if (!data.isObject())
        return result;
    auto integer = [&](const char* key, int fallback, int low, int high) {
        auto v = data.getProperty(key, {});
        return v.isInt() && static_cast<int>(v) >= low && static_cast<int>(v) <= high
                   ? static_cast<int>(v)
                   : fallback;
    };
    auto string = [&](const char* key) {
        auto v = data.getProperty(key, {});
        return v.isString() ? v.toString() : juce::String{};
    };
    result.device.driver = static_cast<Driver>(integer("driver", 0, 0, static_cast<int>(Driver::alsa)));
    result.device.input = string("input");
    result.laptopInput = string("laptopInput");
    result.device.output = string("output");
    result.device.rate = integer("rate", 48000, 1, 1000000);
    result.device.buffer = integer("buffer", 128, 1, 1000000);
    const auto visited = data.getProperty("visited", false);
    result.visited = visited.isBool() && static_cast<bool>(visited);
    result.insurance = data.getProperty("pojistka", true);
    result.midiDevice=string("midiDevice");
    auto configurations=data.getProperty("midiSetups",{});
    if(auto* obj=configurations.getDynamicObject()) for(auto& entry:obj->getProperties()){
        auto& s=result.midiSetups[entry.name.toString()];
        s.enabled=entry.value.getProperty("enabled",true);s.coupled=entry.value.getProperty("both",false);
        auto pads=entry.value.getProperty("pads",{});
        if(auto* list=pads.getArray())for(const auto& pair:*list)
            if(pair.isArray() && pair.size()==2 && pair[0].isInt() && pair[1].isInt() &&
              juce::isPositiveAndBelow(int(pair[0]),128) && juce::isPositiveAndBelow(int(pair[1]),128))s.pads[int(pair[0])]=int(pair[1]);
    }
    auto midiCorrections=data.getProperty("midiCorrections",{});
    if(auto* obj=midiCorrections.getDynamicObject())for(auto& e:obj->getProperties())
        if(e.value.isInt())result.midiCorrections[e.name.toString()]=juce::jlimit(-48000,48000,int(e.value));
    result.pedalKey = integer("pedal", 0, 0, 0x7fffffff);
    auto kits = data.getProperty("bici", {});
    if (auto* obj = kits.getDynamicObject())
        for (auto& entry : obj->getProperties()) {
            DrumSetup kit;
            auto ports = entry.value.getProperty("ports", {}),
                 names = entry.value.getProperty("names", {});
            if (!ports.isArray() || !names.isArray() || ports.size() != names.size())
                continue;
            for (int i = 0; i < ports.size(); ++i)
                if (ports[i].isString() && names[i].isString() &&
                    !kit.ports.contains(ports[i].toString())) {
                    kit.ports.add(ports[i].toString());
                    kit.names.add(names[i].toString());
                }
            kit.stereo = kit.ports.size() == 2 && bool(entry.value.getProperty("stereo", false));
            kit.wide = entry.value.getProperty("wide", true);
            if (!kit.ports.isEmpty())
                result.drumSetups[entry.name.toString()] = std::move(kit);
        }
    auto aliases = data.getProperty("aliases", {});
    if (auto* obj = aliases.getDynamicObject())
        for (auto& item : obj->getProperties())
            if (item.value.isString())
                result.aliases[item.name.toString()] = item.value.toString();
    auto sounds = data.getProperty("guitarPresets", {});
    if (auto* list = sounds.getArray())
        for (auto& item : *list)
            if (item.isObject() && item.getProperty("predvolba", {}).isString())
                result.guitarPresets.push_back(GuitarSound::read(item));
    result.lastSong = string("lastSong");
    result.tuningName = string("tuningName");
    auto tuningList = data.getProperty("tunings", {});
    if (auto* list = tuningList.getArray())
        for (const auto& item : *list) {
            auto name = item.getProperty("name", {}).toString().trim().substring(0, 48);
            auto notes = Tuning::parse(item.getProperty("notes", {}).toString());
            if (name.isNotEmpty() && !notes.empty())
                result.tunings.push_back({name, std::move(notes)});
        }
    auto measurements = data.getProperty("calibrations", {});
    if (auto* obj = measurements.getDynamicObject())
        for (auto& entry : obj->getProperties())
            if (entry.value.isObject()) {
                auto samples = entry.value.getProperty("samples", {}),
                     reported = entry.value.getProperty("reported", {});
                if (samples.isInt() && reported.isInt() && (int)samples >= 0 && (int)samples <= 500000 &&
                    (int)reported >= 0 && (int)reported <= 500000)
                    result.latency[entry.name.toString()] = {(int)samples, (int)reported};
            }
    return result;
}
bool Settings::save(const juce::File& file) const {
    auto* object = new juce::DynamicObject;
    juce::var data(object);
    object->setProperty("schemaVersion", 1);
    object->setProperty("visited", visited);
    object->setProperty("pojistka", insurance);
    object->setProperty("driver", static_cast<int>(device.driver));
    object->setProperty("input", device.input);
    object->setProperty("laptopInput", laptopInput);
    object->setProperty("output", device.output);
    object->setProperty("buffer", device.buffer);
    object->setProperty("rate", device.rate);
    object->setProperty("midiDevice",midiDevice);
    auto* configurations=new juce::DynamicObject;
    object->setProperty("midiSetups",juce::var(configurations));
    for(auto& [key,s]:midiSetups){
        auto* item=new juce::DynamicObject;configurations->setProperty(key,juce::var(item));
        item->setProperty("enabled",s.enabled);item->setProperty("both",s.coupled);
        juce::Array<juce::var> pads;
        for(auto [from,to]:s.pads)pads.add(juce::var(juce::Array<juce::var>{from,to}));
        item->setProperty("pads",pads);
    }
    auto* corrections=new juce::DynamicObject;object->setProperty("midiCorrections",juce::var(corrections));
    for(auto& [key,value]:midiCorrections)corrections->setProperty(key,value);
    object->setProperty("pedal", pedalKey);
    auto* kits = new juce::DynamicObject;
    object->setProperty("bici", juce::var(kits));
    for (auto& [key, kit] : drumSetups) {
        auto* entry = new juce::DynamicObject;
        kits->setProperty(key, juce::var(entry));
        entry->setProperty("ports", juce::var(kit.ports));
        entry->setProperty("names", juce::var(kit.names));
        entry->setProperty("stereo", kit.stereo);
        entry->setProperty("wide", kit.wide);
    }
    auto* names = new juce::DynamicObject;
    object->setProperty("aliases", juce::var(names));
    for (auto& [key, name] : aliases)
        names->setProperty(key, name);
    juce::Array<juce::var> sounds;
    for (const auto& preset : guitarPresets)
        sounds.add(preset.json());
    object->setProperty("guitarPresets", sounds);
    object->setProperty("lastSong", lastSong);
    object->setProperty("tuningName", tuningName);
    juce::Array<juce::var> list;
    for (auto& tuning : tunings) {
        auto* entry = new juce::DynamicObject;
        list.add(juce::var(entry));
        entry->setProperty("name", tuning.name);
        juce::StringArray notes;
        for (int note : tuning.notes)
            notes.add(Tuning::noteName(note));
        entry->setProperty("notes", notes.joinIntoString(" "));
    }
    object->setProperty("tunings", juce::var(list));
    auto* measurements = new juce::DynamicObject;
    object->setProperty("calibrations", juce::var(measurements));
    for (auto& [key, calibration] : latency) {
        auto* record = new juce::DynamicObject;
        measurements->setProperty(key, juce::var(record));
        record->setProperty("samples", calibration.samples);
        record->setProperty("reported", calibration.reported);
    }
    return atomicJson(file, data);
}
} // namespace rocker
