#pragma once
#include <juce_core/juce_core.h>
namespace rocker::cs {
inline const juce::String verseName = u8"Sloka", chorusName = u8"Refrén", endingName = u8"Závěr";
inline const juce::StringArray sectionNames{
    juce::String(u8"Úvod"), verseName, chorusName, juce::String(u8"Most"),
    juce::String(u8"Sólo"), endingName};
} // namespace rocker::cs
