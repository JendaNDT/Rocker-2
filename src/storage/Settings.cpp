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
Settings Settings::load(const juce::File& file) {
    Settings result;
    if (!file.existsAsFile() || file.getSize() > 4 * 1024 * 1024) return result;
    auto data = juce::JSON::parse(file);
    if (!data.isObject()) return result;
    auto integer = [&](const char* key, int fallback, int low, int high) {
        auto v = data.getProperty(key, {});
        return v.isInt() && static_cast<int>(v) >= low && static_cast<int>(v) <= high ? static_cast<int>(v) : fallback;
    };
    auto string = [&](const char* key) {
        auto v = data.getProperty(key, {});
        return v.isString() ? v.toString() : juce::String{};
    };
    result.device.driver = static_cast<Driver>(integer("driver", 0, 0, static_cast<int>(Driver::alsa)));
    result.device.input = string("input");
    result.device.output = string("output");
    result.device.rate = integer("rate", 48000, 1, 1000000);
    result.device.buffer = integer("buffer", 128, 1, 1000000);
    const auto visited = data.getProperty("visited", false);
    result.visited = visited.isBool() && static_cast<bool>(visited);
    auto aliases = data.getProperty("aliases", {});
    if (auto* obj = aliases.getDynamicObject())
        for (auto& item : obj->getProperties())
            if (item.value.isString()) result.aliases[item.name.toString()] = item.value.toString();
    return result;
}
bool Settings::save(const juce::File& file) const {
    if (file.getParentDirectory().createDirectory().failed()) return false;
    auto* object = new juce::DynamicObject;
    juce::var data(object);
    object->setProperty("schemaVersion", 1);
    object->setProperty("visited", visited);
    object->setProperty("driver", static_cast<int>(device.driver));
    object->setProperty("input", device.input);
    object->setProperty("output", device.output);
    object->setProperty("buffer", device.buffer);
    object->setProperty("rate", device.rate);
    auto* names = new juce::DynamicObject;
    object->setProperty("aliases", juce::var(names));
    for (auto& [key, name] : aliases) names->setProperty(key, name);
    juce::TemporaryFile temp(file);
    return temp.getFile().replaceWithText(juce::JSON::toString(data))
        && temp.overwriteTargetFileWithTemporary();
}
}
