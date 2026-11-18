#include "GuitarSound.h"
#include <cmath>

namespace rocker {
namespace {
struct Parameter {
    const char* key;
    std::uint8_t AmpParameters::* field;
    bool bipolar = false;
};
constexpr Parameter parameters[]{
    {"zkresleni", &AmpParameters::drive},    {"basy", &AmpParameters::bass, true},
    {"stredy", &AmpParameters::mid, true},   {"vysky", &AmpParameters::treble, true},
    {"jas", &AmpParameters::presence, true}, {"hlasitost", &AmpParameters::volume},
    {"ozvena", &AmpParameters::echo},        {"dozvuk", &AmpParameters::room}};
constexpr std::array<const char*, 3> shapes{"overdrive", "distortion", "fuzz"};
} // namespace
juce::var GuitarSound::json() const {
    auto* obj = new juce::DynamicObject;
    juce::var result(obj);
    obj->setProperty("predvolba", name);
    obj->setProperty("zapnuto", enabled);
    obj->setProperty("upraveno", modified);
    obj->setProperty("reprobox", "rocker-ir-1");
    obj->setProperty("brana", -int(values.gate));
    obj->setProperty("typ", shapes[values.shape]);
    for (auto p : parameters) {
        const double percent = values.*p.field;
        obj->setProperty(p.key, p.bipolar ? (percent - 50.) / 50. : percent / 100.);
    }
    return result;
}
GuitarSound GuitarSound::read(const juce::var& data) {
    GuitarSound s;
    if (!data.isObject())
        return s;
    auto title = data.getProperty("predvolba", {});
    if (title.isString() && title.toString().trim().isNotEmpty())
        s.name = title.toString().trim().substring(0, 64);
    s.enabled = data.getProperty("zapnuto", true);
    s.modified = data.getProperty("upraveno", false);
    auto shape = data.getProperty("typ", {});
    bool percentFormat = shape.isInt();
    if (percentFormat)
        s.values.shape = static_cast<std::uint8_t>(juce::jlimit(0, 2, (int)shape));
    else
        for (size_t i = 0; i < shapes.size(); ++i)
            if (shape.toString() == shapes[i])
                s.values.shape = static_cast<std::uint8_t>(i);
    auto gate = data.getProperty("brana", {});
    if (gate.isInt() || gate.isDouble()) {
        double db = std::abs((double)gate);
        if (std::isfinite(db))
            s.values.gate = static_cast<std::uint8_t>(std::round(juce::jlimit(24., 84., db)));
    }
    for (auto p : parameters) {
        auto value = data.getProperty(p.key, {});
        if (!(value.isInt() || value.isDouble()) || !std::isfinite((double)value))
            continue;
        double n = value;
        if (!percentFormat)
            n = p.bipolar ? (n + 1) * 50 : n * 100;
        s.values.*p.field = static_cast<std::uint8_t>(std::round(juce::jlimit(0., 100., n)));
    }
    return s;
}
} // namespace rocker
