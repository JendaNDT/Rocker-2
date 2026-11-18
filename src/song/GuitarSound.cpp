#include "GuitarSound.h"
namespace rocker {
namespace {
struct Parameter {
    const char* key;
    std::uint8_t AmpParameters::* field;
    int low = 0, high = 100;
};
constexpr Parameter parameters[]{
    {"typ", &AmpParameters::shape, 0, 2},  {"zkresleni", &AmpParameters::drive},
    {"basy", &AmpParameters::bass},        {"stredy", &AmpParameters::mid},
    {"vysky", &AmpParameters::treble},     {"jas", &AmpParameters::presence},
    {"hlasitost", &AmpParameters::volume}, {"brana", &AmpParameters::gate, 24, 84},
    {"ozvena", &AmpParameters::echo},      {"dozvuk", &AmpParameters::room}};
} // namespace
juce::var GuitarSound::json() const {
    auto* obj = new juce::DynamicObject;
    juce::var result(obj);
    obj->setProperty("predvolba", name);
    obj->setProperty("zapnuto", enabled);
    obj->setProperty("upraveno", modified);
    obj->setProperty("reprobox", "rocker-ir-1");
    for (auto p : parameters)
        obj->setProperty(p.key, int(values.*p.field));
    return result;
}
GuitarSound GuitarSound::read(const juce::var& data) {
    GuitarSound s;
    if (!data.isObject())
        return s;
    auto title = data.getProperty("predvolba", {});
    if (title.isString() && title.toString().trim().isNotEmpty())
        s.name = title.toString().trim().substring(0, 64);
    s.enabled = data.getProperty("zapnuto", false);
    s.modified = data.getProperty("upraveno", false);
    for (auto p : parameters) {
        auto value = data.getProperty(p.key, {});
        if (value.isInt())
            s.values.*p.field = static_cast<std::uint8_t>(juce::jlimit(p.low, p.high, (int)value));
    }
    return s;
}
} // namespace rocker
