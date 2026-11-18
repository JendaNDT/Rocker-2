#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace rocker::style {
inline const juce::Colour background{0xff121417}, panel{0xff1b1e23}, raised{0xff242830},
    line{0xff333843}, barGrid{0xff3a404c}, beatGrid{0xff262a31}, chalk{0xffede9e3}, muted{0xffa9adb5},
    faint{0xff757b86}, record{0xffff3b3b}, recording{0xffb3141b}, playhead{0xfff7f4ee},
    selection{0xff4c8dff}, warning{0xffffc233}, clipping{0xffff6a1a}, ready{0xff3ccb7f};
inline constexpr float selectionOpacity = 0.22f;
inline const juce::Colour drums{0xff2ec4b6}, guitar{0xfff2a541}, guitar2{0xffe86a92}, bass{0xff8f7cf7},
    voice{0xff5bb8f5}, backing{0xff9aa3af}, other{0xffb7c46a};
inline const juce::Colour intro{0xff7c86ff}, verse{0xff3fb6a8}, chorus{0xfff0b429}, bridge{0xffe5737a},
    solo{0xffb884f5}, ending{0xff8d99a6};
inline constexpr int grid = 4, button = 32, transport = 44, recordButton = 52, radius = 4, tapeCut = 8,
                     tapeAngle = 45, target = 32, icon = 20, stroke = 2, hairline = 1, recordFrame = 6,
                     minWidth = 1280, minHeight = 720;
inline constexpr int body = 14, caption = 12, section = 11, counter = 28, distantBar = 180,
                     distantRecord = 96;
inline constexpr float tracking = 0.06f;
inline constexpr int inset = 16, gap = 12, gutter = 32, margin = 56, header = 76, footer = 80,
                     maxWidth = 1320, title = 36, heading = 20, keyWidth = 40, row = 64, nameWidth = 220,
                     levelWidth = 124, clipWidth = 152, labelWidth = 248, fieldWidth = 420,
                     maxWindow = 16384, menuHeight = 32, scaleWidth = 100;
inline constexpr int trackHead = 200, statusStrip = 64, songDeck = 148, songLane = 76, soundDeck = 248, faderReadout = 54,
                     compactButton = 104;
} // namespace rocker::style
