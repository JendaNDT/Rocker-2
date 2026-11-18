#include "Tunings.h"
#include "Pitch.h"
#include "ui/Strings.h"
#include <cmath>
namespace rocker {
std::vector<Tuning> Tuning::factory() {
    const std::array<std::array<int, 6>, 9> notes{{{40, 45, 50, 55, 59, 64},
                                                   {38, 45, 50, 55, 59, 64},
                                                   {36, 43, 48, 53, 57, 62},
                                                   {35, 42, 47, 52, 56, 61},
                                                   {39, 44, 49, 54, 58, 63},
                                                   {38, 43, 48, 53, 57, 62},
                                                   {38, 43, 50, 55, 59, 62},
                                                   {38, 45, 50, 54, 57, 62},
                                                   {38, 45, 50, 55, 57, 62}}};
    std::vector<Tuning> result;
    for (size_t i = 0; i < notes.size(); ++i)
        result.push_back({cs::tuningNames[(int)i], {notes[i].begin(), notes[i].end()}});
    return result;
}
juce::String Tuning::noteName(int note) {
    return cs::pitchNames[note % 12] + juce::String(note / 12 - 1);
}
std::vector<int> Tuning::parse(const juce::String& text) {
    auto words = juce::StringArray::fromTokens(text.toUpperCase(), " ,;", {});
    words.removeEmptyStrings();
    if (words.isEmpty() || words.size() > 12)
        return {};
    std::vector<int> result;
    for (auto word : words) {
        const int letter = juce::String("C D EF G A B").indexOfChar(word[0]);
        int semitone = word[0] == 'H' ? 11 : letter;
        if (semitone < 0 || word.length() < 2)
            return {};
        auto octave = word.substring(1);
        if (octave.startsWithChar('#')) {
            ++semitone;
            octave = octave.substring(1);
        } else if (octave.startsWithChar('B')) {
            --semitone;
            octave = octave.substring(1);
        }
        if (octave.length() != 1 || !octave.containsOnly("0123456789"))
            return {};
        int midi = 12 * (octave.getIntValue() + 1) + semitone;
        if (midi < 29 || midi > 86)
            return {};
        result.push_back(midi);
    }
    return result;
}
int Tuning::nearest(double hz) const {
    int best = 0;
    for (int i = 1; i < (int)notes.size(); ++i)
        if (std::abs(Pitch::cents(hz, notes[i])) < std::abs(Pitch::cents(hz, notes[best])))
            best = i;
    return best;
}
} // namespace rocker
