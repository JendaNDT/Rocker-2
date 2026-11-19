#include "Icons.h"
#include <RockerIconsData.h>
#include <map>

namespace rocker {
void paintIcon(juce::Graphics& g, Icon icon, juce::Colour ink, juce::Rectangle<float> bounds) {
    if (icon == Icon::none)
        return;
    using namespace RockerIconsData;
    static constexpr const char* names[]{
        "", "house_svg", "lightbulb_svg", "circledot_svg", "scissors_svg", "slidershorizontal_svg",
        "timer_svg", "gauge_svg", "cable_svg", "play_svg", "square_svg", "skipback_svg",
        "maximize_svg", "panelrightclose_svg", "panelrightopen_svg", "minus_svg", "plus_svg",
        "circlecheck_svg", "arrowup_svg", "arrowdown_svg", "send_svg"
    };
    static std::map<std::pair<Icon, juce::uint32>, std::unique_ptr<juce::Drawable>> cache;
    auto [entry, fresh] = cache.try_emplace({icon, ink.getARGB()});
    if (fresh) {
        int bytes = 0;
        const char* data = getNamedResource(names[static_cast<size_t>(icon)], bytes);
        if (data != nullptr) {
            auto svg = juce::String::fromUTF8(data, bytes).replace("currentColor", "#000000");
            entry->second = juce::Drawable::createFromSVGString(svg);
            if (entry->second)
                entry->second->replaceColour(juce::Colours::black, ink);
        }
    }
    if (entry->second)
        entry->second->drawWithin(g, bounds, juce::RectanglePlacement::centred, 1.f);
}
} // namespace rocker
