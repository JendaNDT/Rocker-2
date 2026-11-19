#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace rocker::style {
// Colours stay in semantic pairs so cached labels and previews resolve at paint time.
struct Ink {
    juce::uint32 graphite, porcelain;
    juce::Colour get(bool dark) const { return juce::Colour(dark ? graphite : porcelain); }
};
inline constexpr Ink background{0xff191a18, 0xfff4f6f8}, panel{0xff262723, 0xfffdfefe},
    rail{0xff232421, 0xffe5eaf0}, headerSurface{0xff2c2d28, 0xffffffff},
    raised{0xff373832, 0xfff9fbfd}, line{0xff575951, 0xffadb9c6},
    barGrid{0xff53544c, 0xffb8c4d1}, beatGrid{0xff353630, 0xffd4dce3},
    chalk{0xfff6f0dd, 0xff172a41}, muted{0xffbdb9aa, 0xff4b6173}, faint{0xffa3a294, 0xff536677},
    accent{0xfff2bd6c, 0xff255597}, onAccent{0xff332516, 0xffffffff},
    accentSurface{0xff453d2e, 0xffdce6f3}, record{0xfff48077, 0xffa72c43},
    recording{0xfff48077, 0xffa72c43}, playhead{0xfff6f0dd, 0xff172a41},
    selection{0xfff2bd6c, 0xff255597}, warning{0xffffc36a, 0xff805500},
    clipping{0xffff914f, 0xff9f380b}, ready{0xff92d0a8, 0xff16694f};
inline constexpr Ink drums{0xff89c9bd, 0xff15736f}, guitar{0xffe7b56c, 0xff2a599c},
    guitar2{0xffe8a1ba, 0xff954866}, bass{0xffb6acd4, 0xff735397}, voice{0xff92c2e4, 0xff286583},
    backing{0xffb6c0c4, 0xff536776}, other{0xffbcc58e, 0xff596423};
inline constexpr Ink intro{0xffaab3e5, 0xff525f8e}, verse{0xff89c9bd, 0xff15736f},
    chorus{0xffe7b56c, 0xff2a599c}, bridge{0xffe8a591, 0xff925645}, solo{0xffc5aadb, 0xff76518a},
    ending{0xffb6c0c4, 0xff536776}, navigationInk{0xffbdb9aa, 0xff4b6173}, armedInk{0xffffa69e, 0xff962039};
inline constexpr float selectionOpacity = 0.22f, disabledOpacity = 0.5f;
inline constexpr int graphiteRadius = 8, porcelainRadius = 3, lightTransportRadius = 6;
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
inline constexpr int trackHead = 200, statusStrip = 64, songDeck = 148, songLane = 76, soundDeck = 248,
                     faderReadout = 54, compactButton = 104;
inline constexpr int railWidth = 72, railItemWidth = 64, railItemHeight = 52, railModeHeight = 60,
                     railLabel = 10, railLogoHeight = 40, railIndicator = 3, panelRadius = 6,
                     workspaceHeader = 52, workspaceFooter = 76, inspectorWidth = 300,
                     workspaceRuler = 56, workspaceLane = 84, inspectorRow = 28, inspectorGap = 6,
                     armWidth = 112, trackToggle = 26, traceHeight = 3, outlineInset = 3,
                     transportSide = 300, transportTimeWidth = 120, transportPill = 36,
                     transportPillRadius = 18, transportLed = 6, safeGutter = 24,
                     closeColumn = 36, mixTrackHead = 466, mixNameWidth = 152, mixGainWidth = 140,
                     workspaceTitle = 18, meterClock = 140, homeActions = 380, resumeHeight = 188,
                     timelineBarMin = 24;
inline constexpr float armedOpacity = 0.18f, clipFillOpacity = 0.18f;
} // namespace rocker::style
