#pragma once
#include "storage/Settings.h"
#include "Tokens.h"
#include "song/Edit.h"
#include <functional>
#include <deque>
#include <future>
#include "engine/Export.h"

namespace rocker {
enum class Page {
    home,
    device,
    tone,
    inputs,
    live,
    amp,
    tuner,
    diagnostics,
    customTuning,
    newProject,
    project,
    trackSettings,
    rescue
};
class Theme final : public juce::LookAndFeel_V4 {
  public:
    Theme();
    juce::Font font(float size, bool mono = false, bool bold = false) const;
    juce::Font getPopupMenuFont() override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

  private:
    juce::Typeface::Ptr regular, heavy, digits;
};
class Studio final : public juce::Component, private juce::Timer, private juce::AsyncUpdater {
  public:
    Studio(DeviceSystem&, juce::File, juce::File songLibrary = {});
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    ~Studio() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    Page currentPage() const { return page; }
    juce::StringArray layoutErrors() const;
    const Theme& theme() const { return skin; }

  private:
    using Rect = juce::Rectangle<int>;
    using Handler = std::function<void()>;
    struct Line {
        juce::String value;
        Rect bounds;
        juce::Font font;
        juce::Colour ink;
        bool right;
    };
    Theme skin;
    DeviceSystem& devices;
    juce::File settingsFile;
    Settings settings;
    Connection link;
    Page page;
    juce::OwnedArray<juce::Component> controls;
    std::vector<Line> textLines;
    std::vector<Rect> panels;
    juce::Viewport viewport;
    juce::Component rows;
    juce::String message;
    int answer = -1;
    bool played = false, laptop = false;
    std::vector<std::uint32_t> activityBeforePluck;
    int detectionTicks = 0;
    void identifyGuitar();
    void handleAsyncUpdate() override;
    void timerCallback() override;
    void rebuild();
    void show(Page);
    void persist();
    void finish();
    void refresh(bool);
    void status(juce::String);
    void makeHome(Rect);
    void makeWizard(Rect);
    void devicePage(Rect);
    void tonePage(Rect);
    void inputPage(Rect);
    void text(juce::String, Rect, int size = style::body, juce::Colour colour = style::chalk,
              bool mono = false, bool bold = false, bool right = false);
    juce::Button* button(const juce::String&, Rect, Handler, int look = 0, juce::String key = {});
    void combo(const juce::String&, Rect, const juce::StringArray&, int selected,
               std::function<void(int)> change, bool mono = false);
    void choose(DeviceChoice);
    juce::File libraryRoot, projectDir, diagnosticDir;
    Song song, diagnosticSong;
    AudioLibrary library;
    ViewState view;
    std::unique_ptr<Performance> performance;
    juce::String draftTitle, draftTempo = "112", draftTrack;
    int draftBeats = 4, draftUnit = 4, draftKind = 0, draftPort = 0, draftChannels = 1;
    bool editingTempo = false, diagnosing = false;
    int projectMonitor = 0;
    double lastTap = 0, lastArchive = 0;
    Rect timelineArea, ruler, sectionStrip;
    int selectedOccurrence = -1, cutBar = 0, targetBar = 0;
    bool askingRepair = false, copyingSection = false;
    juce::String sectionDraft;
    enum class TapeAction { split, repeat, detach, restore, resize, name, colour, move };
    void selectOccurrence(int);
    void tapeAction(TapeAction, int value = 0);
    void sectionDeck(Rect);
    void drawSections(juce::Graphics&);
    void sectionDown(const juce::MouseEvent&);
    void confirmRepair(bool independent);

    std::vector<Rect> lanes;
    int firstTrack = 0;
    std::map<juce::String, std::deque<std::pair<Frame, float>>> liveWaves;
    Rect inputMeter;
    bool soundDeckOpen = false, namingPreset = false;
    juce::String presetDraft;
    juce::Component::SafePointer<juce::Button> soundBadge;
    double pendingSoundSave = 0;
    void openTrackSound(int index);
    void trackSoundDeck(Rect);
    void applyTrackSound(bool recall);
    void newProject();
    void openProject(juce::File);
    void finishProject();
    void projectPage(Rect);
    void paintProject(juce::Graphics&);
    void startProject(bool, bool repair = false, TakeGroup* auditionGroup = nullptr);
    void startRepair();
    void audition(TakeGroup&);
    void chooseAttempt(bool keep);
    void discardAttempt();
    bool comparing = false;
    int preview = -1;
    juce::String previewGroup;
    void attemptDeck(TakeGroup&, Rect);
    void stopProject(bool automatic = false);
    void projectTick();
    static juce::String limit(const juce::Font&, juce::String, int width);
    void mixMode(bool exportPanel);
    void mixPage(Rect);
    void exportPage(Rect);
    void paintMix(juce::Graphics&);
    void toggleMix(int index, bool solo);
    void mixSlider(const juce::String&, Rect, float initial, bool pan, std::function<void(float)>);
    void seekMix(Frame);
    void beginExport();
    void pollExport();
    void findClick();
    Rect stereoMeter, mixReadout;
    std::vector<Rect> trackMeters;
    Playback::Meter lastMeter;
    MixFile delivered;
    juce::File deliveringProject;
    std::future<MixFile> delivery;
    std::unique_ptr<juce::FileChooser> clickChooser;
    std::future<std::optional<bool>> clickCheck;
    Delivery format = Delivery::wave24;
    bool exporting = false, selectedRange = false;
    juce::String soloNames() const;
    void saveSong(juce::String action = {});
    EditHistory history;
    Fragment clipboard;
    juce::Array<juce::File> versions;
    int versionIndex = 0;
    bool browsingVersions = false;
    Frame anchorBar = 0;
    enum class Drag { none, move, left, right, bars, takeLeft, takeRight, tapeEdge, tapeMove };
    Drag drag = Drag::none;
    Frame grabOffset = 0, dragTarget = 0;
    juce::Point<int> pointer;
    bool dragged = false;
    double lastPan = 0;
    enum class EditOp { split, copy, paste, erase, repeat, move, trimLeft, trimRight };
    Span selection() const { return {view.selectionStart, view.selectionEnd}; }
    void pick(Span, juce::String clip = {});
    void editMode(bool);
    void editOp(EditOp);
    void undo(bool forward);
    void editDeck(Rect);
    void versionsDeck(Rect);
    void showVersions();
    void restoreVersion();
    void normalizeSelection();
    void reveal(Frame);
    int visibleBars() const;
    Frame firstBar() const;
    Frame frameAt(int x) const;
    int pixelAt(Frame) const;
    void selectBar(Frame, bool extend);
    void editMouseDown(const juce::MouseEvent&);
    void paintEditOverlay(juce::Graphics&);
    void paintOverChildren(juce::Graphics&) override;
    bool editKey(const juce::KeyPress&);
    juce::String selectedTrack() const;

    void editTrack(bool);
    void configureSongMonitor();
    bool testingLoad = false;
    int trialAmps = 8, initialXruns = -1;
    void startLoadTrial();
    void finishLoadTrial(Performance&);
    void syncTest();
    void finishSync(Performance&);
    void field(const juce::String&, Rect, juce::String&);
    Frame playPosition() const;
    void toggleDistant();
    Page tunerReturn = Page::home;
    int measurementInput = 0, guitarSource = 0, primaryInput = 0, secondaryInput = 1, presetIndex = 1;
    bool hearSecond = false;
    AmpParameters sound = presets[1];
    std::vector<Tuning> tunings;
    int tuningIndex = 0, targetString = -1, nearestString = 0, stableString = -1;
    double stableTime = 0, readingHz = 0, cents = 0;
    double lastVoiced = 0, needleCents = 0;
    std::vector<bool> confirmed;
    juce::String customName, customNotes;
    Rect needleArea, inputBar, outputBar, routeSymbol;
    enum class Readout { inputLevel, level, cpu, pitch, hint, confirmed };
    std::map<Readout, std::size_t> dynamicLines;
    void livePages(Rect);
    void playPanel(Rect);
    void ampPanel(Rect);
    void tunerPanel(Rect);
    void diagnosticPanel(Rect);
    void customPanel(Rect);
    void pollLive();
    void resetRoute();
    void resetTuning();
    void changeMonitor(int);
    void toggleTuner();
    int heardInput() const;
    juce::StringArray inputNames() const;
    int listeningDelaySamples() const;
    juce::String latencyLabel() const;
    void dynamicText(Readout, juce::String, Rect, int, bool);
    void setReadout(Readout, const juce::String&);
    void fader(const juce::String&, Rect, int, int, int, const juce::String&, std::function<void(int)>);
};
} // namespace rocker
