#pragma once
#include "song/Edit.h"
#include <set>
namespace rocker {
bool packSong(const juce::File& project,const juce::File& target);
std::optional<juce::File> unpackSong(const juce::File& package,const juce::File& library);
bool syncedFolder(const juce::File&);
std::set<juce::String> usedAudio(const Song&);
juce::Array<juce::File> unusedAudio(const Song&,const juce::File&,const EditHistory&);
int trashAudio(Song&,const juce::File&,const EditHistory&,const std::function<bool(const juce::File&)>&);
} // namespace rocker
