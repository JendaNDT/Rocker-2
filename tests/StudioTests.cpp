#include "storage/ProjectFiles.h"
#include "FakeDevice.h"
#include "ui/Studio.h"
#include "ui/Strings.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <numbers>
#include <cmath>
using namespace rocker;
using namespace juce;

namespace {
struct Temp {
    File dir =
        File::getSpecialLocation(File::tempDirectory).getChildFile("rocker-test-" + Uuid().toString());
    File file = dir.getChildFile("nastaveni.json");
    ~Temp() { dir.deleteRecursively(); }
};
template <class T> T* find(Component& root, const String& name) {
    for (auto* c : root.getChildren()) {
        if (c->getName() == name)
            if (auto* result = dynamic_cast<T*>(c))
                return result;
        if (auto* result = find<T>(*c, name))
            return result;
    }
    return nullptr;
}
void click(Studio& view, const String& name) {
    auto* b = find<Button>(view, name);
    INFO(name.toStdString());
    REQUIRE(b);
    REQUIRE(b->isEnabled());
    b->onClick();
    view.resized();
}
void chooseItem(Studio& view, const String& title, int index) {
    auto* box = find<ComboBox>(view, title);
    REQUIRE(box);
    REQUIRE(index < box->getNumItems());
    box->setSelectedItemIndex(index, sendNotificationSync);
    view.resized();
}
void tick() {
    Thread::sleep(60);
    Timer::callPendingTimersSynchronously();
}
void fits(const Studio& view) {
    auto errors = view.layoutErrors();
    INFO(errors.joinIntoString(" / ").toStdString());
    REQUIRE(errors.isEmpty());
}
void snapshot(Studio& view, const char* name, int pixels) {
    const auto scale = pixels == 1280 ? 1.f : 1.5f;
    auto img = view.createComponentSnapshot(view.getLocalBounds(), true, scale);
    REQUIRE(img.getWidth() == pixels);
    REQUIRE(img.getHeight() == pixels * 9 / 16);
    auto dest = SystemStats::getEnvironmentVariable("ROCKER_SNAPSHOTS", {});
    if (dest.isEmpty())
        return;
    auto dir = File(dest);
    REQUIRE(dir.createDirectory().wasOk());
    FileOutputStream stream(dir.getChildFile(String(name) + "-" + String(pixels) + ".png"));
    REQUIRE(stream.openedOk());
    stream.setPosition(0);
    stream.truncate();
    REQUIRE(PNGImageFormat().writeImageToStream(img, stream));
}
} // namespace
TEST_CASE("Settings round trip, partial fields and corrupt files") {
    Temp t;
    Settings settings;
    settings.device = {Driver::shared, u8"Příliš žluťoučký", u8"Sluchátka", 96000, 256};
    settings.visited = true;
    settings.aliases["1/2/in"] = u8"Řeřicha ůěť";
    REQUIRE(settings.save(t.file));
    auto loaded = Settings::load(t.file);
    CHECK(loaded.device == settings.device);
    CHECK(loaded.visited);
    CHECK(loaded.aliases == settings.aliases);
    REQUIRE(t.file.replaceWithText("{\"rate\":96000,\"input\":[],\"buffer\":-3,\"unknown\":42}"));
    loaded = Settings::load(t.file);
    CHECK(loaded.device.rate == 96000);
    CHECK(loaded.device.buffer == 128);
    CHECK(loaded.device.input.isEmpty());
    REQUIRE(t.file.replaceWithText("{!!!"));
    CHECK(Settings::load(t.file).device == DeviceChoice{});
}
TEST_CASE("A-17 real screen layouts and snapshots") {
    ScopedJuceInitialiser_GUI gui;
    Temp t;
    test::FakeDevice fake;
#if JUCE_MAC
    fake.core = true;
#endif
    Studio view(fake, t.file);
    for (int pixels : {1280, 1920}) {
        if (view.currentPage() == Page::home)
            click(view, cs::setup);
        fits(view);
        snapshot(view, "01-zarizeni", pixels);
        click(view, cs::next);
        click(view, cs::play);
        for (auto& answer : cs::answers) {
            click(view, answer);
            fits(view);
        }
        click(view, cs::answers[0]);
        snapshot(view, "02-sluchatka", pixels);
        click(view, cs::next);
        fake.process({{-0.35f}, {1.0f}}, 1);
        view.resized();
        fits(view);
        snapshot(view, "03-vstupy", pixels);
        click(view, cs::finish);
        fits(view);
        snapshot(view, "00-uvod", pixels);
        REQUIRE(find<Button>(view, cs::resume));
        CHECK_FALSE(find<Button>(view, cs::resume)->isEnabled());
        fits(view);
        click(view, cs::library);
        fits(view);
        click(view, cs::home);
        click(view, cs::song);
        fits(view);
        snapshot(view, "09-nova-skladba", pixels);
        click(view, cs::back);
        click(view, cs::livePage);
        chooseItem(view, cs::listen, 2);
        fake.process({std::vector<float>(512, 0.3f)}, 512);
        tick();
        fits(view);
        snapshot(view, "04-co-slysim", pixels);
        click(view, cs::ampPage);
        fits(view);
        snapshot(view, "05-aparat", pixels);
        click(view, cs::tuner);
        chooseItem(view, cs::tuning, 1);
        for (int frame = 0; frame < 6; ++frame) {
            std::vector<float> notes(4096);
            for (int i = 0; i < 4096; ++i)
                notes[i] = 0.3f * std::sin(2 * std::numbers::pi * Pitch::frequency(38) *
                                           (4096 * frame + i) / fake.rate);
            fake.process({notes}, 4096);
            tick();
        }
        fits(view);
        snapshot(view, "06-ladicka", pixels);
        click(view, cs::custom);
        fits(view);
        snapshot(view, "07-vlastni-ladeni", pixels);
        click(view, cs::diagnostics);
        fits(view);
        snapshot(view, "08-diagnostika", pixels);
        click(view, cs::home);
    }
    for (int key : {'t', 'T', 'n'})
        CHECK(view.keyPressed(KeyPress(key)));
    view.setSize(1920, 1080);
    fits(view);
    for (bool mono : {false, true})
        for (int size : {11, 12, 14, 28, 96, 180}) {
            auto font = view.theme().font((float)size, mono);
            REQUIRE(font.getTypefacePtr()->getName().containsIgnoreCase(mono ? "JetBrains" : "Inter"));
            GlyphArrangement glyphs;
            glyphs.addLineOfText(font, u8"řůěť", 0, 0);
            REQUIRE(glyphs.getNumGlyphs() == 4);
            for (int i = 0; i < 4; ++i) {
                CHECK(glyphs.getGlyph(i).getGlyphIndex() != 0);
                Path path;
                glyphs.getGlyph(i).createPath(path);
                CHECK_FALSE(path.isEmpty());
            }
        }
}
TEST_CASE("A-24 setup follows available inputs, drivers, rates and unplug events") {
    ScopedJuceInitialiser_GUI gui;
    for (int count : {1, 2, 8, 18})
        for (int rate : {44100, 48000, 96000}) {
            Temp t;
            test::FakeDevice fake;
            fake.channels = count;
            fake.rate = rate;
            fake.asio = count != 1 && count != 8;
            fake.laptop = count == 1;
            Studio view(fake, t.file);
            auto* drivers = find<ComboBox>(view, cs::driver);
            REQUIRE(drivers);
            CHECK(drivers->getText() ==
                  cs::drivers[(int)(fake.asio ? Driver::asio
                                              : (fake.laptop ? Driver::shared : Driver::exclusive))]);
            auto* rates = find<ComboBox>(view, cs::rate);
            REQUIRE(rates);
            CHECK(rates->getNumItems() == 1);
            CHECK(rates->getText() == cs::frequency(rate));
            fits(view);
            click(view, cs::skip);
            click(view, cs::next);
            for (int i = 0; i < count; ++i)
                REQUIRE(find<TextEditor>(view, cs::channel(i)));
            CHECK_FALSE(find<TextEditor>(view, cs::channel(count)));
            auto* editor = find<TextEditor>(view, cs::channel(0));
            editor->setText(u8"Kytara řůěť", false);
            editor->onTextChange();
            fits(view);
            fake.disconnect();
            view.resized();
            CHECK(view.currentPage() == Page::device);
            CHECK_FALSE(fake.active);
            CHECK_FALSE(find<ComboBox>(view, cs::rate));
            click(view, cs::skip);
            CHECK_FALSE(find<Button>(view, cs::play));
            click(view, cs::skip);
            CHECK_FALSE(find<TextEditor>(view, cs::channel(0)));
            click(view, cs::finish);
            fake.attached = true;
            fake.changed();
            view.resized();
            click(view, cs::setup);
            click(view, cs::next);
            click(view, cs::next);
            REQUIRE(find<TextEditor>(view, cs::channel(0))->getText() == String(u8"Kytara řůěť"));
            click(view, cs::finish);
            CHECK(Settings::load(t.file).visited);
        }
}

TEST_CASE("A pluck names the active input even if it ends between UI updates") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    Studio view(fake, temp.file);
    click(view, cs::next);
    click(view, cs::next);
    click(view, cs::findGuitar);
    fake.process({{0}, {-0.4f}}, 1);
    fake.process({}, 1024);
    for (int i = 0; i < 100 && !temp.file.existsAsFile(); ++i) {
        Thread::sleep(20);
        Timer::callPendingTimersSynchronously();
    }
    view.resized();
    REQUIRE(find<TextEditor>(view, cs::channel(1))->getText() == cs::guitar);
    CHECK(find<TextEditor>(view, cs::channel(0))->getText().isEmpty());
}

TEST_CASE("Guitar routing, custom tuning and cable calibration survive normal UI workflows") {
    CHECK(cs::latencyRating(10) == cs::good);
    CHECK(cs::latencyRating(10.01) == cs::usable);
    CHECK(cs::latencyRating(15) == cs::usable);
    CHECK(cs::latencyRating(15.01) == cs::slow);
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    {
        Studio view(fake, temp.file);
        click(view, cs::home);
        click(view, cs::livePage);
        chooseItem(view, cs::source, 3);
        REQUIRE(find<ComboBox>(view, cs::ampIn)->getNumItems() == 1);
        chooseItem(view, cs::listeningTo, 1);
        auto* modes = find<ComboBox>(view, cs::listen);
        REQUIRE(modes);
        CHECK(modes->getNumItems() == 2);
        chooseItem(view, cs::listen, 1);
        CHECK(fake.process({{0.5f}, {-0.25f}}, 1)[0][0] == -0.25f);
        click(view, cs::diagnostics);
        REQUIRE(fake.active->live.monitor == Monitor::off);
        chooseItem(view, cs::input, 1);
        click(view, cs::measure);
        const int delay = 411;
        std::vector<float> wire(delay, 0);
        for (int pos = 0; pos < fake.rate; pos += 64) {
            test::FakeDevice::Block input(2, std::vector<float>(64));
            for (int i = 0; i < 64; ++i)
                input[1][i] = wire[(pos + i) % delay];
            auto output = fake.process(input, 64);
            for (int i = 0; i < 64; ++i)
                wire[(pos + i) % delay] = output[0][i];
        }
        for (int i = 0; i < 10 && Settings::load(temp.file).latency.empty(); ++i)
            tick();
        auto saved = Settings::load(temp.file);
        auto key = Settings::calibrationKey(saved.device);
        REQUIRE(saved.latency.contains(key));
        CHECK(saved.latency.at(key).samples == delay);
        CHECK(saved.latency.at(key).correction() == 38);
        auto other = saved.device;
        other.buffer *= 2;
        CHECK_FALSE(saved.latency.contains(Settings::calibrationKey(other)));
        view.resized();
        fits(view);
        click(view, cs::measure);
        fake.process({}, fake.rate);
        tick();
        view.resized();
        CHECK(Settings::load(temp.file).latency.at(key).samples == delay);
        click(view, cs::tuner);
        click(view, cs::custom);
        auto enter = [&](const String& label, const String& value) {
            auto* field = find<TextEditor>(view, label);
            REQUIRE(field);
            field->setText(value, false);
            field->onTextChange();
        };
        enter(cs::tuningName, String(u8"Jendův baryton"));
        enter(cs::userNotes, "B1 F#2 B2 E3 G#3 C#4");
        click(view, cs::saveTuning);
        REQUIRE(view.currentPage() == Page::tuner);
        REQUIRE(find<ComboBox>(view, cs::tuning)->getText() == String(u8"Jendův baryton"));
    }
    Studio reopened(fake, temp.file);
    click(reopened, cs::tuner);
    REQUIRE(find<ComboBox>(reopened, cs::tuning)->getText() == String(u8"Jendův baryton"));
    REQUIRE(Settings::load(temp.file).tunings.at(0).notes == Tuning::parse("B1 F#2 B2 E3 G#3 C#4"));
}

TEST_CASE("E2 player records eight bars of drums, overdubs dry guitar and resumes the saved view") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    Settings initial;
    initial.visited = true;
    REQUIRE(initial.save(temp.file));
    {
        Studio view(fake, temp.file);
        click(view, cs::song);
        auto* title = find<TextEditor>(view, cs::titleSong);
        REQUIRE(title);
        title->setText(String(u8"Noční riff"), false);
        title->onTextChange();
        click(view, cs::createSong);
        REQUIRE(view.currentPage() == Page::project);
        fits(view);
        click(view, cs::trackEdit);
        chooseItem(view, cs::input, 0);
        click(view, cs::apply);
        click(view, cs::arm);
        click(view, cs::recordSong);
        BeatGrid grid;
        const int total = (int)grid.beat(36) + 384;
        auto feed = [&](bool guitar) {
            for (int pos = 0; pos < total; pos += 256) {
                int n = std::min(256, total - pos);
                test::FakeDevice::Block in(2, std::vector<float>(n));
                for (int i = 0; i < n; ++i)
                    in[guitar ? 1 : 0][i] = guitar ? 0.2f * std::sin((pos + i) * 0.017f)
                                                   : 0.68f * std::exp(-((pos + i) % 6000) / 800.f) *
                                                         std::cos((pos + i) * 0.035f);
                fake.process(in, n);
                if (pos % 16384 == 0)
                    Thread::sleep(1);
            }
        };
        feed(false);
        click(view, cs::stopSong);
        click(view, cs::atStart);
        click(view, cs::disarm);
        chooseItem(view, cs::audioTracks, 1);
        click(view, cs::trackEdit);
        chooseItem(view, cs::input, 1);
        click(view, cs::apply);
        click(view, cs::arm);
        chooseItem(view, cs::listen, 2);
        click(view, cs::ampPage);
        REQUIRE(fake.active->live.input.load() == 1);
        click(view, cs::soundOff);
        click(view, cs::soundOn);
        REQUIRE(fake.active->live.input.load() == 1);
        click(view, cs::useProject);
        click(view, cs::recordSong);
        feed(true);
        click(view, cs::distantView);
        fits(view);
        for (int pixels : {1280, 1920})
            snapshot(view, "11-zdalky", pixels);
        click(view, cs::normal);
        click(view, cs::stopSong);
        click(view, cs::atStart);
        auto dir = File(Settings::load(temp.file).lastSong);
        bool bad = false;
        auto loaded = Song::load(dir, bad);
        REQUIRE(loaded);
        REQUIRE(loaded->clips.size() == 2);
        REQUIRE(loaded->assets.size() == 2);
        auto wav = waveReader(dir.getChildFile(loaded->assets[1].path));
        REQUIRE(wav);
        AudioBuffer<float> sample(1, 1);
        REQUIRE(wav->read(&sample, 0, 1, 233, true, false));
        CHECK(sample.getSample(0, 0) == 0.2f * std::sin((grid.beat(4) + 233) * 0.017f));
        click(view, cs::playSong);
        click(view, "Crunch");
        chooseItem(view, cs::preset, 3);
        auto* drive = find<Slider>(view, cs::drive);
        REQUIRE(drive);
        drive->setValue(64, sendNotificationSync);
        REQUIRE(find<Button>(view, "Metal*"));
        fake.process({}, 4096);
        click(view, cs::savePreset);
        auto* presetEditor = find<TextEditor>(view, cs::presetTitle);
        REQUIRE(presetEditor);
        presetEditor->setText(String(u8"Můj aparát"), false);
        presetEditor->onTextChange();
        click(view, cs::storePreset);
        REQUIRE(find<Button>(view, String(u8"Můj aparát")));
        REQUIRE(Settings::load(temp.file).guitarPresets.size() == 1);
        fits(view);
        for (int pixels : {1280, 1920})
            snapshot(view, "12-zvuk-stopy", pixels);
        click(view, cs::useProject);
        click(view, cs::stopSong);
        fits(view);
        for (int pixels : {1280, 1920})
            snapshot(view, "10-skladba", pixels);
        click(view, cs::home);
    }
    auto machine = Settings::load(temp.file);
    machine.guitarPresets.clear();
    REQUIRE(machine.save(temp.file));
    Studio reopened(fake, temp.file);
    click(reopened, cs::resume);
    click(reopened, String(u8"Můj aparát"));
    auto* reopenedDrive = find<Slider>(reopened, cs::drive);
    REQUIRE(reopenedDrive);
    CHECK(reopenedDrive->getValue() == 64);
    CHECK(reopened.currentPage() == Page::project);
    auto v = ViewState::load(File(Settings::load(temp.file).lastSong));
    CHECK(v.cursor == 4096);
    CHECK(v.selected == 1);
}
TEST_CASE("A-17 edit bars five and six, move to nine, repeat, undo and restore a dated version") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    auto folder = temp.dir.getChildFile("Song");
    Song initial;
    initial.title = String(u8"Zkušebna");
    initial.grid.bpm = 240;
    initial.tracks = {{"d", cs::trackKinds[0], "bici"}, {"g", cs::guitar, "kytara"}};
    Frame length = initial.grid.beat(32);
    initial.assets = {{"a", "Zvuk/riff.wav", length, 1}};
    initial.clips = {{"one", "d", "a", 0, 0, length}, {"two", "g", "a", 0, 0, length}};
    auto file = folder.getChildFile("Zvuk/riff.wav");
    REQUIRE(file.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(file, 48000, 1);
    REQUIRE(writer);
    AudioBuffer<float> block(1, 12000);
    for (int i = 0; i < 12000; ++i)
        block.setSample(0, i, 0.7f * std::sin(i * 0.07f) * std::exp(-i / 2900.f));
    for (int i = 0; i < 32; ++i)
        REQUIRE(writer->writeFromAudioSampleBuffer(block, 0, 12000));
    writer.reset();
    MemoryBlock before;
    REQUIRE(file.loadFileAsData(before));
    auto archived = initial;
    archived.title = "Archive";
    REQUIRE(archived.archive(folder));
    REQUIRE(initial.save(folder));
    Settings settings;
    settings.visited = true;
    settings.lastSong = folder.getFullPathName();
    REQUIRE(settings.save(temp.file));
    Studio view(fake, temp.file);
    click(view, cs::resume);
    click(view, cs::editMode);
    click(view, cs::zoomOut);
    click(view, cs::guitar);
    auto* head = find<Button>(view, cs::trackKinds[0]);
    REQUIRE(head);
    const int x0 = style::railWidth + style::trackHead;
    const int width = view.getWidth() - style::inspectorWidth - x0;
    auto x = [&](double bar) { return float(x0 + bar * width / 16); };
    const int axisY = head->getY() - style::grid - style::workspaceRuler / 4;
    auto* guitar = find<Button>(view, cs::guitar);
    REQUIRE(guitar);
    const int laneY = guitar->getBounds().getCentreY();
    auto mouse = [&](Point<float> here, Point<float> down, int mods, bool dragging = false) {
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), here, ModifierKeys(mods), 0, 0, 0,
                          0, 0, &view, &view, Time::getCurrentTime(), down, Time::getCurrentTime(), 1,
                          dragging);
    };
    auto tap = [&](float at, int y, int mods) {
        Point<float> pt(at, (float)y);
        auto e = mouse(pt, pt, mods);
        view.mouseDown(e);
        view.mouseUp(e);
        view.resized();
    };
    tap(x(4.12), axisY, 0);
    tap(x(5.12), axisY, ModifierKeys::shiftModifier);
    auto v = ViewState::load(folder);
    REQUIRE(v.selectionStart == initial.grid.beat(16));
    REQUIRE(v.selectionEnd == initial.grid.beat(24));
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "13-strihej", pixels);
    Point<float> down(x(4.45), (float)laneY), up(x(8.45), (float)laneY);
    view.mouseDown(mouse(down, down, 0));
    view.mouseDrag(mouse(up, down, 0, true));
    view.mouseUp(mouse(up, down, 0, true));
    view.resized();
    auto key = [&](int code, int mods = 0) {
        REQUIRE(view.keyPressed(KeyPress(code, ModifierKeys(mods), 0)));
        view.resized();
    };
    key('D');
    key('S', ModifierKeys::ctrlModifier);
    bool bad = false;
    auto result = Song::load(folder, bad);
    REQUIRE(result);
    REQUIRE_FALSE(bad);
    REQUIRE(result->clips.size() == 5);
    CHECK(result->clips.back().position == initial.grid.beat(40));
    CHECK(result->clips.back().offset == initial.grid.beat(16));
    key('Z', ModifierKeys::ctrlModifier);
    key('Z', ModifierKeys::ctrlModifier);
    REQUIRE(Song::load(folder, bad) == initial);
    key('Y', ModifierKeys::ctrlModifier);
    key('Y', ModifierKeys::ctrlModifier);
    REQUIRE(Song::load(folder, bad) == result);
    Point<float> edge(x(8.02), (float)laneY), trim(x(8.32), (float)laneY);
    view.mouseDown(mouse(edge, edge, 0));
    view.mouseDrag(mouse(trim, edge, ModifierKeys::altModifier, true));
    view.resized();
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "14-lupa", pixels);
    view.mouseUp(mouse(trim, edge, ModifierKeys::altModifier, true));
    key('S', ModifierKeys::ctrlModifier);
    auto trimmed = Song::load(folder, bad);
    REQUIRE(trimmed);
    REQUIRE(trimmed != result);
    auto freeEdge = ViewState::load(folder).selectionStart;
    CHECK(freeEdge != snapBeat(initial.grid, freeEdge));
    click(view, cs::fileMenu);
    fits(view);
    click(view, cs::applyVersion);
    REQUIRE(Song::load(folder, bad) == archived);
    key('Z', ModifierKeys::ctrlModifier);
    REQUIRE(Song::load(folder, bad) == trimmed);
    MemoryBlock after;
    REQUIRE(file.loadFileAsData(after));
    REQUIRE(after == before);
}

TEST_CASE("Repair bar six twice from the keyboard and keep the original after reopening") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice device;
    auto folder = temp.dir.getChildFile("Repair");
    Song initial;
    initial.title = "Riff";
    initial.grid.bpm = 240;
    initial.tracks = {{"g", cs::guitar, "kytara", {"In 1"}}};
    auto originalFile = folder.getChildFile("Zvuk/original.wav");
    REQUIRE(originalFile.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(originalFile, initial.grid.rate, 1);
    REQUIRE(writer);
    const int beat = (int)initial.grid.beat(1);
    AudioBuffer<float> phrase(1, beat);
    for (int n = 0; n < 32; ++n) {
        for (int i = 0; i < beat; ++i)
            phrase.setSample(0, i, .5f * std::sin(i * (n == 20 ? .085f : .06f)) * std::exp(-i / 3300.f));
        REQUIRE(writer->writeFromAudioSampleBuffer(phrase, 0, beat));
    }
    writer.reset();
    initial.assets = {{"original", "Zvuk/original.wav", 32 * beat, 1}};
    initial.clips = {{"original-clip", "g", "original", 0, 0, 32 * beat}};
    MemoryBlock original;
    REQUIRE(originalFile.loadFileAsData(original));
    REQUIRE(initial.save(folder));
    Settings settings;
    settings.visited = true;
    settings.lastSong = folder.getFullPathName();
    REQUIRE(settings.save(temp.file));
    ViewState state;
    state.click = false;
    state.editing = true;
    state.selectionStart = initial.grid.beat(20);
    state.selectionEnd = initial.grid.beat(24);
    REQUIRE(state.save(folder));
    bool bad = false;
    {
        Studio view(device, temp.file);
        click(view, cs::resume);
        for (int i = 1; i <= 2; ++i) {
            REQUIRE(view.keyPressed(KeyPress('P')));
            auto rescue = folder.getChildFile("Zachrana");
            REQUIRE(rescue.findChildFiles(File::findFiles, false, "*.json").size() == 1);
            const test::FakeDevice::Block input{std::vector<float>(256, 0.15f * i)};
            for (int block = 0; block < 950; ++block)
                device.process(input, 256);
            // The message-thread timer closes the completed take and starts the audition.
            for (int poll = 0;
                 poll < 60 && !rescue.findChildFiles(File::findFiles, false, "*.json").isEmpty(); ++poll)
                tick();
            auto journals = rescue.findChildFiles(File::findFiles, false, "*.json");
            INFO((journals.isEmpty() ? std::string{} : journals[0].loadFileAsString().toStdString()));
            REQUIRE(journals.isEmpty());
            view.resized();
            auto saved = Song::load(folder, bad);
            REQUIRE(saved);
            REQUIRE_FALSE(bad);
            REQUIRE(saved->groups.size() == 1);
            auto& g = saved->groups.front();
            REQUIRE(g.attempts.size() == i);
            CHECK(g.from == state.selectionStart);
            CHECK(g.to == state.selectionEnd);
            auto& pass = g.attempts.back().clips.front();
            CHECK(pass.position == initial.grid.beat(12));
            CHECK(pass.length == initial.grid.beat(16));
            CHECK(pass.offset == 373);
            auto wave = waveReader(folder.getChildFile(saved->assets.back().path));
            REQUIRE(wave);
            CHECK(wave->lengthInSamples == pass.length + 373);
        }
        fits(view);
        for (int pixels : {1280, 1920})
            snapshot(view, "15-pokusy", pixels);
        REQUIRE(view.keyPressed(KeyPress(KeyPress::tabKey)));
        REQUIRE(view.keyPressed(KeyPress(KeyPress::returnKey)));
        REQUIRE(Song::load(folder, bad)->groups.front().active == 0);
        click(view, cs::stopSong);
        REQUIRE(view.keyPressed(KeyPress(KeyPress::tabKey)));
        view.resized();
        click(view, cs::discardAttempt);
        REQUIRE(Song::load(folder, bad)->groups.front().attempts.front().discarded);
        click(view, cs::stopSong);
        click(view, cs::restoredTake);
        REQUIRE_FALSE(Song::load(folder, bad)->groups.front().attempts.front().discarded);
        REQUIRE(view.keyPressed(KeyPress('Z', ModifierKeys::ctrlModifier, 0)));
        REQUIRE(Song::load(folder, bad)->groups.front().attempts.front().discarded);
    }
    MemoryBlock after;
    REQUIRE(originalFile.loadFileAsData(after));
    REQUIRE(after == original);
    Studio reopened(device, temp.file);
    click(reopened, cs::resume);
    REQUIRE(find<Button>(reopened, cs::restoredTake));
    fits(reopened);
}

TEST_CASE("A-02 A-17 section strip splits, repeats, trims and asks before a shared repair") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    auto dir = temp.dir.getChildFile("Arrangement");
    Song song;
    song.title = String(u8"Sloka a refrén");
    song.bars = 16;
    song.grid.bpm = 240;
    song.tracks = {{"g", cs::guitar, "kytara", {"In 1"}}};
    auto file = dir.getChildFile("Zvuk/riff.wav");
    REQUIRE(file.getParentDirectory().createDirectory().wasOk());
    auto writer = waveWriter(file, 48000, 1);
    REQUIRE(writer);
    AudioBuffer<float> beat(1, 12000);
    for (int i = 0; i < beat.getNumSamples(); ++i)
        beat.setSample(0, i, .6f * std::sin(i * .06f) * std::exp(-i / 2700.f));
    for (int n = 0; n < 64; ++n)
        REQUIRE(writer->writeFromAudioSampleBuffer(beat, 0, beat.getNumSamples()));
    writer.reset();
    song.assets = {{"a", "Zvuk/riff.wav", song.barFrame(16), 1}};
    song.clips = {{"g1", "g", "a", 0, 0, song.barFrame(16)}};
    REQUIRE(song.save(dir));
    Settings settings;
    settings.visited = true;
    settings.lastSong = dir.getFullPathName();
    REQUIRE(settings.save(temp.file));
    ViewState state;
    state.editing = true;
    state.zoom = 28;
    REQUIRE(state.save(dir));
    Studio view(fake, temp.file);
    click(view, cs::resume);
    auto* head = find<Button>(view, cs::guitar);
    REQUIRE(head);
    const int start = style::railWidth + style::trackHead;
    const int width = view.getWidth() - style::inspectorWidth - start;
    const int axisY = head->getY() - style::grid - style::workspaceRuler / 4;
    const int tapeY = axisY - style::workspaceRuler / 2;
    auto x = [&](double bar) { return float(start + bar * width / 28); };
    auto gesture = [&](double a, double b, int y, int modifiers = 0) {
        Point<float> down{x(a), float(y)}, up{x(b), float(y)};
        auto event = [&](Point<float> here, bool drag) {
            return MouseEvent(Desktop::getInstance().getMainMouseSource(), here, ModifierKeys(modifiers),
                              0, 0, 0, 0, 0, &view, &view, Time::getCurrentTime(), down,
                              Time::getCurrentTime(), 1, drag);
        };
        view.mouseDown(event(down, false));
        if (a != b)
            view.mouseDrag(event(up, true));
        view.mouseUp(event(up, a != b));
        view.resized();
    };
    auto key = [&](int code, int mod = 0) {
        REQUIRE(view.keyPressed(KeyPress(code, ModifierKeys(mod), 0)));
        view.resized();
    };
    bool bad = false;
    auto read = [&] {
        auto result = Song::load(dir, bad);
        REQUIRE(result.has_value());
        REQUIRE_FALSE(bad);
        return *result;
    };
    gesture(8, 8, tapeY);
    click(view, cs::sectionSplit);
    key('D');
    REQUIRE(read().linked(2));
    auto full = read();
    gesture(24, 22, tapeY);
    REQUIRE(read().order[2].length == 6);
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "16-dily", pixels);
    key('Z', ModifierKeys::ctrlModifier);
    CHECK(read() == full);
    key('Y', ModifierKeys::ctrlModifier);
    gesture(22, 24, tapeY);
    CHECK(read() == full);
    gesture(24, 22, tapeY);
    click(view, cs::sectionRestore);
    CHECK(read() == full);
    auto repair = [&](String answer) {
        gesture(18.4, 18.4, axisY);
        key('P');
        fits(view);
        REQUIRE_FALSE(
            dir.getChildFile("Zachrana").findChildFiles(File::findFiles, false, "*.json").size());
        for (int pixels : {1280, 1920})
            snapshot(view, "17-oprava-dilu", pixels);
        click(view, answer);
        auto rescue = dir.getChildFile("Zachrana");
        REQUIRE(rescue.findChildFiles(File::findFiles, false, "*.json").size() == 1);
        test::FakeDevice::Block input{std::vector<float>(256, .125f)};
        for (int i = 0; i < 950; ++i)
            fake.process(input, 256);
        for (int i = 0; i < 60 && !rescue.findChildFiles(File::findFiles, false, "*.json").isEmpty(); ++i)
            tick();
        REQUIRE(rescue.findChildFiles(File::findFiles, false, "*.json").isEmpty());
        view.resized();
        click(view, cs::stopSong);
    };
    repair(cs::sharedRepair);
    REQUIRE(read().groups.size() == 2);
    REQUIRE(read().groups.front().attempts.size() == 1);
    repair(cs::ownRepair);
    auto last = read();
    REQUIRE_FALSE(last.linked(2));
    REQUIRE(last.groups.front().attempts.size() == 1);
    REQUIRE(last.groups.back().attempts.size() == 2);
    CHECK(last.sections.at(last.order[2].section).name.endsWith("(2)"));
    key('3');
    auto slider = find<Slider>(view, cs::gainLabel + " " + cs::guitar);
    REQUIRE(slider);
    auto* pan = find<Slider>(view, cs::panLabel + " " + cs::guitar);
    REQUIRE(pan);
    pan->setValue(-1., sendNotificationSync);
    CHECK(pan->getTextFromValue(-1.).contains("100"));
    for (auto db : {-3., -12.5, -95.9}) {
        slider->setValue(db, sendNotificationSync);
        CHECK(slider->getTextFromValue(db).endsWith(" dB"));
        fits(view);
    }
    pan->setValue(0., sendNotificationSync);
    slider->setValue(-3., sendNotificationSync);
    key('S', ModifierKeys::ctrlModifier);
    CHECK(read().tracks.at(0).gainDb == -3.f);
    key(' '); // Live meters and the playhead use the same transport as Hraj.
    test::FakeDevice::Block monitor{std::vector<float>(256)};
    for (int i = 0; i < 9; ++i)
        fake.process(monitor, 256);
    view.resized();
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "18-mix", pixels);
    key(' ');

    key('S');
    REQUIRE(read().tracks.at(ViewState::load(dir).selected).solo);
    fits(view);
    key('E', ModifierKeys::ctrlModifier);
    REQUIRE(find<Button>(view, cs::exportSolo));
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "19-export", pixels);
    click(view, cs::rangeOnly);
    click(view, cs::exportSolo);
    for (int i = 0; i < 150 && !find<Button>(view, cs::playMix); ++i) {
        tick();
        view.resized();
    }
    REQUIRE(find<Button>(view, cs::playMix));
    auto files = dir.getChildFile("Exporty").findChildFiles(File::findFiles, false, "*.wav");
    REQUIRE(files.size() == 1);
    REQUIRE(waveReader(files[0])->lengthInSamples ==
            ViewState::load(dir).selectionEnd - ViewState::load(dir).selectionStart);
    click(view, cs::wholeSong);
    click(view, cs::louderExport);
    fits(view);
    click(view, cs::exportSolo);
    for (int i = 0; i < 150 && !find<Button>(view, cs::playMix); ++i) {
        tick();
        view.resized();
    }
    REQUIRE(find<Button>(view, cs::playMix));
    REQUIRE(dir.getChildFile("Exporty").findChildFiles(File::findFiles, false, "*hlasitejsi.wav").size() == 1);
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "35-hlasitejsi-export", pixels);
    click(view, cs::stems);
    CHECK_FALSE(find<Button>(view, cs::playMix));
    fits(view);
    click(view, cs::makeStems);
    juce::Array<juce::File> stems;
    for (int i = 0; i < 150; ++i) {
        tick();
        view.resized();
        stems = dir.getChildFile("Exporty").findChildFiles(File::findDirectories, false, "*stopy*");
        if (!stems.isEmpty() && find<Button>(view, cs::mixFolder))
            break;
    }
    REQUIRE(stems.size() == 1);
    CHECK(stems[0].findChildFiles(File::findFiles, false, "*.wav").size() == 1);
    CHECK(find<Button>(view, cs::mixFolder));
    CHECK_FALSE(find<Button>(view, cs::playMix));
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "32-stopy-zvlast", pixels);
    click(view, cs::back);
    key('M');
    REQUIRE(read().tracks.at(ViewState::load(dir).selected).mute);
    key('1');
    REQUIRE_FALSE(ViewState::load(dir).mixing);
}

TEST_CASE("A-17 E8 an idea becomes a starred riff and a song with an imported backing") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice device;
    Settings config;
    config.visited = true;
    REQUIRE(config.save(temp.file));
    Studio view(device, temp.file);
    int step = 0;
    auto play = [&](int samples) {
        while (samples > 0) {
            int n = std::min(512, samples);
            std::vector<float> signal(n);
            for (auto& x : signal) {
                x = float(.8 * std::exp(-(step % 12000) / 4800.) * std::sin(step * .05));
                ++step;
            }
            device.process({signal}, n);
            samples -= n;
        }
    };
    auto key = [&](int code, int mods = 0) {
        REQUIRE(view.keyPressed(KeyPress(code, ModifierKeys(mods), 0)));
        view.resized();
    };
    play(3 * 48000);
    key('R');
    REQUIRE(view.currentPage() == Page::ideas);
    for (int i = 0; i < 40; ++i) {
        play(4800);
        tick();
    }
    view.resized();
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "20-nahravam-napad", pixels);
    key(KeyPress::spaceKey);
    for (int i = 0; i < 150 && !find<Button>(view, cs::ideaStar); ++i) {
        tick();
        view.resized();
    }
    click(view, cs::deleteIdea);
    REQUIRE(find<Button>(view, cs::answers[0]));
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "31-kos-napadu", pixels);
    click(view, cs::answers[2]);
    REQUIRE(find<Button>(view, cs::deleteIdea));
    click(view, cs::ideaStar);
    auto* note = find<TextEditor>(view, cs::note);
    REQUIRE(note);
    note->setText(String::fromUTF8("Riff na zítřek"), true);
    click(view, cs::saveNote);
    click(view, cs::starredOnly);
    click(view, cs::tapIdea);
    for (int i = 0; i < 100 && !find<Button>(view, cs::tap + " 0 / 8"); ++i) {
        tick();
        view.resized();
    }
    REQUIRE(find<Button>(view, cs::tap + " 0 / 8"));
    for (int i = 0; i < 8; ++i) {
        play(i == 0 ? 2400 : 24000);
        key(KeyPress::spaceKey);
    }
    auto catalog = Ideas(temp.dir.getChildFile("Napady"));
    REQUIRE(catalog.all().size() == 1);
    const auto& idea = catalog.all().front();
    CHECK(idea.star);
    CHECK(idea.length == 6 * 48000);
    CHECK(idea.bpm == 120);
    CHECK(idea.note.isNotEmpty());
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "21-napady", pixels);
    click(view, cs::home);
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "22-uvod-s-napadem", pixels);
    click(view, cs::library);
    click(view, cs::useIdea);
    REQUIRE(view.currentPage() == Page::project);
    auto dir = File(Settings::load(temp.file).lastSong);
    StringArray drop{catalog.wave(idea.id).getFullPathName()};
    REQUIRE(view.isInterestedInFileDrag(drop));
    view.filesDropped(drop, 0, 0);
    bool damaged = false;
    for (int i = 0; i < 100; ++i) {
        tick();
        view.resized();
        auto song = Song::load(dir, damaged);
        if (song && song->tracks.size() == 2)
            break;
    }
    auto song = Song::load(dir, damaged);
    REQUIRE(song);
    REQUIRE(song->tracks.size() == 2);
    CHECK(song->tracks.back().kind == "podklad");
    auto at = [&] {
        auto current = Song::load(dir, damaged);
        REQUIRE(current);
        for (auto& c : current->clips)
            if (c.track == song->tracks.back().id)
                return c.position;
        return Frame(-1);
    };
    CHECK(at() == 0);
    key(KeyPress::rightKey);
    CHECK(at() == 48);
    key(KeyPress::rightKey, ModifierKeys::shiftModifier);
    CHECK(at() == 528);
}

TEST_CASE("A-24 acoustic and electronic drums use only the microphones that responded") {
    ScopedJuceInitialiser_GUI gui;
    for (int count : {1, 2, 8, 18}) {
        CAPTURE(count);
        Temp temp;
        test::FakeDevice fake;
        fake.channels = count;
        fake.asio = count == 2;
        fake.laptop = count == 1;
        fake.rate = count == 8 ? 96000 : 44100;
        Studio view(fake, temp.file, temp.dir.getChildFile("songs"));
        click(view, cs::pedal);
        REQUIRE(view.keyPressed(KeyPress('J')));
        CHECK(Settings::load(temp.file).pedalKey == 'J');
        click(view, cs::next);
        click(view, cs::next);
        click(view, cs::drumTest);
        click(view, cs::finishHits);
        CHECK_FALSE(find<Button>(view, cs::kitApply));
        click(view, cs::drumAgain);
        test::FakeDevice::Block signal(count, std::vector<float>(8, 0.f));
        int active = std::min(count, 8);
        for (int i = 0; i < active; ++i) {
            signal[i][2] = i == 0 ? -.75f : .125f;
            fake.process(signal, 8);
            signal[i][2] = 0;
        }
        fake.process({}, 512);
        CHECK(fake.active->hitPeak(0) == .75f);
        click(view, cs::finishHits);
        fits(view);
        if (count == 8)
            for (int px : {1280, 1920})
                snapshot(view, "23-sada-bicich", px);
        click(view, cs::kitApply);
        click(view, cs::finish);
        click(view, cs::song);
        click(view, cs::createSong);
        bool bad = false;
        auto dir = File(Settings::load(temp.file).lastSong);
        auto song = Song::load(dir, bad);
        REQUIRE(song);
        REQUIRE_FALSE(bad);
        if (count == 2) {
            CHECK(song->tracks[0].inputs.size() == 2);
            auto stereo = fake.process({{.25f}, {-.75f}}, 1);
            CHECK(stereo[0][0] == .25f);
            CHECK(stereo[1][0] == -.75f);
        } else {
            REQUIRE(song->tracks.size() == (size_t)active + 1);
            for (int i = 0; i < active; ++i) {
                CHECK(song->tracks[i].inputs[0] == "In " + String(i + 1));
                CHECK(song->linkedTrack(song->tracks[0].id, song->tracks[i].id));
            }
            if (active >= 4) {
                CHECK(song->tracks[active - 2].pan == -1);
                CHECK(song->tracks[active - 1].pan == 1);
            }
        }
        click(view, cs::arm);
        song = Song::load(dir, bad);
        REQUIRE(song);
        for (auto& t : song->tracks)
            CHECK(t.armed == (t.kind == "bici"));
        if (count == 8) {
            click(view, cs::distantView);
            chooseItem(view, cs::farTarget, 2);
            click(view, cs::normal);
        }
        REQUIRE(view.keyPressed(KeyPress('J')));
        view.resized();
        click(view, cs::farOffer);
        for (int i = 0; i < 480; ++i)
            fake.process(signal, 8);
        for (int px : {1280, 1920})
            if (count == 8)
                snapshot(view, "24-odpocet", px);
        test::FakeDevice::Block audio(count, std::vector<float>(512));
        for (int channel = 0; channel < count; ++channel)
            std::fill(audio[channel].begin(), audio[channel].end(), float(channel + 1) / 32);
        for (Frame pos = 0; pos < song->barFrame(count == 8 ? 5 : 2); pos += 512)
            fake.process(audio, 512);
        if (count == 8)
            for (int px : {1280, 1920})
                snapshot(view, "25-zaznam-bicich", px);
        REQUIRE(view.keyPressed(KeyPress(KeyPress::escapeKey)));
        view.resized();
        REQUIRE(find<Button>(view, cs::stopSong));
        REQUIRE(view.keyPressed(KeyPress(KeyPress::returnKey)));
        view.resized();
        song = Song::load(dir, bad);
        REQUIRE(song);
        REQUIRE(song->assets.size() == (size_t)(count == 2 ? 1 : active));
        for (size_t i = 0; i < song->assets.size(); ++i) {
            auto wav = waveReader(dir.getChildFile(song->assets[i].path));
            REQUIRE(wav);
            AudioBuffer<float> block((int)wav->numChannels, 1);
            REQUIRE(wav->read(&block, 0, 1, 512, true, true));
            CHECK(block.getSample(0, 0) == float(i + 1) / 32);
            if (count == 2)
                CHECK(block.getSample(1, 0) == 2.f / 32);
        }
        if (count == 8) {
            click(view, cs::distantView);
            for (int px : {1280, 1920})
                snapshot(view, "26-ulozene-bici", px);
            click(view, cs::normal);
            click(view, cs::mixMode);
            auto* gain = find<Slider>(view, cs::kitGain);
            REQUIRE(gain);
            gain->setValue(-6, sendNotificationSync);
            for (int poll = 0; poll < 10; ++poll)
                tick();
            click(view, cs::playMode);
            song = Song::load(dir, bad);
            REQUIRE(song);
            for (auto& t : song->tracks)
                if (t.kind == "bici")
                    CHECK(t.groupDb == -6);
        }
        REQUIRE(view.keyPressed(KeyPress(KeyPress::numberPad0)));
        REQUIRE(view.keyPressed(KeyPress(KeyPress::numberPad0)));
        if (count == 18) {
            REQUIRE(view.keyPressed(KeyPress(KeyPress::escapeKey)));
            view.resized();
            click(view, cs::setup);
            click(view, cs::next);
            click(view, cs::next);
            click(view, cs::drumTest);
            fake.disconnect();
            view.resized();
            CHECK(view.currentPage() == Page::device);
        }
    }
}

TEST_CASE("A-24 MIDI module alone and paired with its stereo audio record through the studio") {
    ScopedJuceInitialiser_GUI gui;
    for (bool paired : {false, true}) {
        CAPTURE(paired);
        Temp temp;
        test::FakeDevice fake;
        fake.channels = paired ? 2 : 0;
        fake.rate = paired ? 96000 : 44100;
        fake.midi.add({"Module USB", "module-id"});
        Studio view(fake, temp.file, temp.dir.getChildFile("songs"));
        click(view, cs::next);
        click(view, cs::next);
        if (paired) {
            click(view, cs::drumTest);
            fake.process({{.3f}, {.4f}}, 1);
            click(view, cs::finishHits);
            click(view, cs::kitApply);
        }
        click(view, cs::midiDrums);
        REQUIRE(fake.selectedMidi == "module-id");
        auto* modes = find<ComboBox>(view, cs::midiMode);
        REQUIRE(modes);
        CHECK(modes->getNumItems() == (paired ? 3 : 2));
        if (paired)
            chooseItem(view, cs::midiMode, 2);
        click(view, cs::padLearn);
        for (int k = 0; k < 8; ++k) {
            fake.active->drums.push({fake.active->drums.position, 70 + k, 90});
            fake.process({}, 16);
            for (int attempt = 0;
                 attempt < 20 && fake.active->drums.mapping[70 + k].load() != DrumPlayer::pads[k];
                 ++attempt)
                tick();
            view.resized();
            CHECK(fake.active->drums.mapping[70 + k].load() == DrumPlayer::pads[k]);
            if (k == 1 && paired)
                for (int px : {1280, 1920})
                    snapshot(view, "27-naucit-pady", px);
        }
        fits(view);
        click(view, cs::back);
        click(view, cs::finish);
        click(view, cs::song);
        click(view, cs::createSong);
        auto dir = File(Settings::load(temp.file).lastSong);
        bool bad = false;
        auto song = Song::load(dir, bad);
        REQUIRE(song);
        const int m = paired ? 1 : 0;
        REQUIRE(song->tracks[m].kind == "midi");
        if (paired) {
            REQUIRE(song->tracks[0].inputs.size() == 2);
            CHECK(song->linkedTrack(song->tracks[0].id, song->tracks[m].id));
            click(view, cs::midiListen);
        }
        click(view, cs::countIn);
        click(view, cs::arm);
        click(view, cs::recordSong);
        test::FakeDevice::Block signal(fake.channels, std::vector<float>(512, .1f));
        for (int i = 0; i < 50; ++i) {
            if (i == 3)
                fake.active->drums.push({fake.active->drums.position + 50, 71, 112});
            fake.process(signal, 512);
        }
        click(view, cs::stopSong);
        song = Song::load(dir, bad);
        REQUIRE(song);
        CHECK_FALSE(bad);
        REQUIRE(song->assets.size() == (paired ? 2 : 1));
        auto asset = std::find_if(song->assets.begin(), song->assets.end(),
                                  [](auto& a) { return a.path.endsWith(".mid"); });
        REQUIRE(asset != song->assets.end());
        auto notes = readMidi(dir.getChildFile(asset->path), fake.rate);
        REQUIRE(notes);
        REQUIRE(notes->size() == 1);
        CHECK(notes->front().note == 38);
        CHECK(notes->front().frame == 3 * 512 + 50);
        CHECK(notes->front().velocity == 112);
        click(view, cs::trackEdit);
        if (paired)
            for (int px : {1280, 1920})
                snapshot(view, "28-sada-midi", px);
        chooseItem(view, cs::kitPreset, 1);
        click(view, cs::apply);
        song = Song::load(dir, bad);
        REQUIRE(song);
        CHECK(song->tracks[m].kit == 1);
        fake.midi.clear();
        fake.midiChanged();
        view.resized();
        click(view, cs::recordSong);
        CHECK_FALSE(find<Button>(view, cs::stopSong));
    }
}

TEST_CASE("A-24 separate clocks, unplugged recording and package remapping through the UI") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    fake.asio = false;
    fake.separate = true;
    fake.channels = 1;
    Studio studio(fake, temp.file, temp.dir.getChildFile("songs"));
    click(studio, cs::home);
    click(studio, cs::song);
    click(studio, cs::createSong);
    click(studio, cs::countIn);
    click(studio, cs::arm);
    click(studio, cs::recordSong);
    CHECK_FALSE(find<Button>(studio, cs::stopSong));
    click(studio, cs::home);
    click(studio, cs::setup);
    click(studio, cs::clockOverride);
    click(studio, cs::home);
    REQUIRE(studio.keyPressed(KeyPress(KeyPress::returnKey)));
    studio.resized();
    click(studio, cs::recordSong);
    test::FakeDevice::Block block(1, std::vector<float>(1024, .25f));
    for (int i = 0; i < 20; ++i)
        fake.process(block, 1024);
    fake.disconnect();
    studio.resized();
    CHECK(studio.currentPage() == Page::project);
    auto dir = File(Settings::load(temp.file).lastSong);
    bool bad = false;
    auto s = Song::load(dir, bad);
    REQUIRE(s);
    REQUIRE(s->assets.size() == 1);
    CHECK(s->assets[0].length == 20480);
    fake.attached = true;
    fake.changed();
    studio.resized();
    REQUIRE(fake.active);
    click(studio, cs::home);
    s->tracks[0].inputs = StringArray{"Old input"};
    REQUIRE(s->save(dir));
    auto file = temp.dir.getChildFile("transfer.rocker2");
    REQUIRE(packSong(dir, file));
    REQUIRE(studio.isInterestedInFileDrag({file.getFullPathName()}));
    studio.filesDropped({file.getFullPathName()}, 0, 0);
    for (int n = 0; n < 80 && studio.currentPage() != Page::remap; ++n) {
        tick();
        studio.resized();
    }
    REQUIRE(studio.currentPage() == Page::remap);
    fits(studio);
    for (int px : {1280, 1920})
        snapshot(studio, "29-premapovani", px);
    chooseItem(studio, cs::newInput + " 1", 1);
    click(studio, cs::apply);
    auto copy = File(Settings::load(temp.file).lastSong);
    CHECK(copy != dir);
    s = Song::load(copy, bad);
    REQUIRE(s);
    CHECK(s->tracks[0].inputs[0] == "In 1");
    click(studio, cs::fileMenu);
    fits(studio);
    for (int px : {1280, 1920})
        snapshot(studio, "30-baleni-skladby", px);
}

TEST_CASE("A-25 ideas record, preview and become songs with a custom song folder") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    Settings config;
    config.visited = true;
    const auto songs = temp.dir.getChildFile("Hudba/Moje skladby");
    config.libraryPath = songs.getFullPathName();
    REQUIRE(config.save(temp.file));
    Studio view(fake, temp.file);
    auto key = [&](int code) {
        REQUIRE(view.keyPressed(KeyPress(code)));
        view.resized();
    };
    test::FakeDevice::Block signal(1, std::vector<float>(512));
    for (size_t i = 0; i < signal.front().size(); ++i)
        signal.front()[i] = float(.25 * std::sin(i * .1));
    auto play = [&](int blocks) {
        for (int i = 0; i < blocks; ++i)
            fake.process(signal, 512);
    };
    play(94);
    key('R');
    fake.changed();
    REQUIRE(view.currentPage() == Page::ideas);
    for (int i = 0; i < 15; ++i) {
        play(12);
        tick();
    }
    key(KeyPress::spaceKey);
    for (int i = 0; i < 65 && !find<Button>(view, cs::ideaStar); ++i) {
        tick();
        view.resized();
    }
    REQUIRE(find<Button>(view, cs::ideaStar));
    Ideas catalog(temp.dir.getChildFile("Napady"));
    REQUIRE(catalog.all().size() == 1);
    REQUIRE(catalog.all().front().tuning.isNotEmpty());
    key(KeyPress::spaceKey);
    for (int i = 0; i < 65 && !find<Button>(view, cs::stopSong); ++i) {
        tick();
        view.resized();
    }
    REQUIRE(find<Button>(view, cs::stopSong));
    key(KeyPress::spaceKey);
    click(view, cs::useIdea);
    REQUIRE(view.currentPage() == Page::project);
    CHECK(File(Settings::load(temp.file).lastSong).isAChildOf(songs));
    temp.dir.getChildFile("Napady").deleteRecursively();
    REQUIRE(temp.dir.getChildFile("Napady").replaceWithText("blocked"));
    click(view, cs::home);
    key('R');
    CHECK(view.statusText() == String(u8"Nápad se nepodařilo uložit."));
    fits(view);
}

TEST_CASE("A-24 changing to a song rate retains unrelated device calibrations") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    fake.rate = 44100;
    fake.rates = {44100., 48000.};
    Settings config;
    config.visited = true;
    config.device = {Driver::asio, "Studio USB", "Studio USB", 44100, 128};
    const auto oldKey = Settings::calibrationKey(config.device);
    auto newChoice = config.device;
    newChoice.rate = 48000;
    const auto newKey = Settings::calibrationKey(newChoice);
    config.latency[oldKey] = {500, 373};
    config.latency[newKey] = {501, 373};
    const auto dir = temp.dir.getChildFile("Skladby/Přenos");
    config.lastSong = dir.getFullPathName();
    REQUIRE(config.save(temp.file));
    Song song;
    song.title = "Test";
    song.bars = 1;
    REQUIRE(song.save(dir));
    Studio view(fake, temp.file);
    REQUIRE(view.keyPressed(KeyPress(KeyPress::returnKey)));
    view.resized();
    REQUIRE(view.currentPage() == Page::project);
    CHECK(view.statusText().contains("48 kHz"));
    CHECK(view.statusText().contains("44,1 kHz"));
    click(view, String(u8"Přepnout zvukovku na 48 kHz"));
    CHECK(fake.rate == 48000);
    const auto saved = Settings::load(temp.file);
    CHECK(saved.device.rate == 48000);
    CHECK(saved.latency.contains(oldKey));
    CHECK_FALSE(saved.latency.contains(newKey));
    REQUIRE(view.keyPressed(KeyPress(KeyPress::spaceKey)));
    view.resized();
    CHECK(find<Button>(view, cs::stopSong));
    fits(view);
}

TEST_CASE("A-17 E15 voice presets and controls stay available from a song track") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    const auto dir = temp.dir.getChildFile("Skladby/Voice");
    Song song;
    song.title = "Voice";
    song.bars = 1;
    song.tracks = {{"v", "Vocal", "jine", {"In 1"}}};
    REQUIRE(song.save(dir));
    Settings config;
    config.visited = true;
    config.lastSong = dir.getFullPathName();
    REQUIRE(config.save(temp.file));
    Studio view(fake, temp.file);
    click(view, cs::resume);
    click(view, cs::voicePanel);
    chooseItem(view, cs::presetTitle, 1);
    auto* compression = find<Slider>(view, cs::voiceCompression);
    REQUIRE(compression);
    compression->setValue(62., sendNotificationSync);
    REQUIRE(view.keyPressed(KeyPress('S', ModifierKeys(ModifierKeys::ctrlModifier), 0)));
    view.resized();
    fits(view);
    bool damaged = false;
    auto saved = Song::load(dir, damaged);
    REQUIRE(saved);
    CHECK_FALSE(damaged);
    CHECK(saved->tracks[0].voice.values.compression == 62);
    for (int pixels : {1280, 1920})
        snapshot(view, "33-zvuk-stopy", pixels);
    chooseItem(view, cs::presetTitle, 0);
    CHECK_FALSE(find<Slider>(view, cs::voiceCompression));
}

TEST_CASE("A-17 the transport toggles the selected guide and the standalone metronome plays it") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    const auto dir = temp.dir.getChildFile("Skladby/Guide");
    Song song;
    song.title = "Guide";
    song.bars = 2;
    song.tracks = {{"g", "Guitar", "kytara", {"In 1"}}};
    REQUIRE(song.save(dir));
    Settings config;
    config.visited = true;
    config.lastSong = dir.getFullPathName();
    REQUIRE(config.save(temp.file));
    Studio view(fake, temp.file);
    click(view, cs::resume);
    click(view, cs::navMetro);
    REQUIRE(view.currentPage() == Page::metronome);
    click(view, "7/8");
    auto* volume = find<Slider>(view, cs::clickLevel);
    REQUIRE(volume);
    volume->setValue(40, sendNotificationSync);
    chooseItem(view, cs::clickVoice, 1);
    click(view, cs::playSong);
    auto sound = fake.process({}, 512);
    REQUIRE_FALSE(sound.empty());
    CHECK(std::abs(sound[0][0] - .064f) < 1.e-6f);
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "38-metronom", pixels);
    chooseItem(view, cs::clickMode, 1);
    click(view, cs::applySongTempo);
    REQUIRE(view.currentPage() == Page::project);
    CHECK(Settings::load(temp.file).clickVolume == 40);
    CHECK(Settings::load(temp.file).clickSound == 1);
    bool damaged = false;
    auto stored = Song::load(dir, damaged);
    REQUIRE(stored);
    CHECK(stored->grid.beats == 7);
    CHECK(stored->grid.unit == 8);
    CHECK(ViewState::load(dir).rhythm);
    click(view, cs::home);
    click(view, cs::resume);
    CHECK(find<Button>(view, cs::guideRhythm));
    fits(view);
    for (int pixels : {1280, 1920})
        snapshot(view, "34-vodici-rytmus", pixels);
    click(view, cs::guideRhythm);
    CHECK_FALSE(ViewState::load(dir).click);
    click(view, cs::guideOff);
    CHECK(ViewState::load(dir).click);
    CHECK(ViewState::load(dir).rhythm);
    click(view, cs::ideas);
    click(view, cs::navMetro);
    chooseItem(view, cs::clickMode, 0);
    click(view, "5/4");
    click(view, cs::tempoMore);
    click(view, cs::applyIdeaTempo);
    REQUIRE(view.currentPage() == Page::ideas);
    auto* tempo = find<Button>(view, cs::tempo);
    REQUIRE(tempo);
    CHECK(tempo->getTooltip() == String(u8"113,0 BPM · 5/4"));
    fits(view);
}

TEST_CASE("A-17 A-33 loop recording keeps passes and Tab compares them from the editing view") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    const auto dir = temp.dir.getChildFile("Skladby/Loop");
    Song song;
    song.title = "Loop";
    song.grid.bpm = 300;
    song.bars = 1;
    song.tracks = {{"t", "Loop guitar", "kytara", {"In 1"}}};
    REQUIRE(song.save(dir));
    ViewState view;
    view.editing = true;
    view.click = view.countIn = false;
    view.leadBars = 0;
    view.tailBars = 0;
    view.selectionStart = 0;
    view.selectionEnd = song.barFrame(1);
    REQUIRE(view.save(dir));
    Settings config;
    config.visited = true;
    config.lastSong = dir.getFullPathName();
    REQUIRE(config.save(temp.file));
    Studio studio(fake, temp.file);
    click(studio, cs::resume);
    click(studio, cs::recordLoop);
    REQUIRE(find<Button>(studio, cs::stopSong));
    const Frame expected = 3 * song.barFrame(1);
    test::FakeDevice::Block input{std::vector<float>(512, .27f)};
    for (Frame at = 0; at < expected; at += 512)
        fake.process(input, int(std::min<Frame>(512, expected - at)));
    studio.resized();
    fits(studio);
    for (int pixels : {1280, 1920})
        snapshot(studio, "36-nahravam-dokola", pixels);
    click(studio, cs::stopSong);
    fake.process(input, 121 + 252);
    tick();
    studio.resized();
    bool damaged = false;
    auto saved = Song::load(dir, damaged);
    REQUIRE(saved);
    CHECK_FALSE(damaged);
    REQUIRE(saved->groups.size() == 1);
    REQUIRE(saved->groups[0].attempts.size() == 3);
    CHECK(saved->groups[0].active == 3);
    click(studio, cs::listenTake);
    auto replay = fake.process({}, 512);
    CHECK(replay[0][500] > .05f);
    click(studio, cs::stopSong);
    REQUIRE(studio.keyPressed(KeyPress(KeyPress::tabKey)));
    studio.resized();
    CHECK(find<Button>(studio, cs::stopSong));
}

TEST_CASE("A-17 redesign keeps the transport stable and gives the mixer a collapsible inspector") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    const auto dir = temp.dir.getChildFile("Skladby/Redesign");
    Song song;
    song.title = u8"Příliš dlouhý název rozpracované písně pro kytaru, basu a bicí";
    song.bars = 8;
    for (int i = 0; i < 8; ++i)
        song.tracks.push_back({String(i), String(u8"Dlouhá rytmická kytara ") + String(i + 1),
                               "kytara", {i == 1 ? "In 2" : "In 1"}, i == 0});
    REQUIRE(song.save(dir));
    ViewState options;
    options.countIn = false;
    REQUIRE(options.save(dir));
    Settings settings;
    settings.visited = true;
    settings.lastSong = dir.getFullPathName();
    REQUIRE(settings.save(temp.file));
    Studio studio(fake, temp.file);
    fits(studio);
    for (int pixels : {1280, 1920})
        snapshot(studio, "40-uvod-skladba", pixels);
    click(studio, cs::resume);
    fits(studio);
    const auto bounds = [&](const String& title) {
        auto* action = find<Button>(studio, title);
        REQUIRE(action);
        return action->getBounds();
    };
    const auto play = bounds(cs::playSong);
    const auto record = bounds(cs::recordSong);
    const auto start = bounds(cs::atStart);
    click(studio, cs::recordSong);
    CHECK(bounds(cs::stopSong) == record);
    CHECK(bounds(cs::playSong) == play);
    CHECK(bounds(cs::atStart) == start);
    fits(studio);
    for (int pixels : {1280, 1920})
        snapshot(studio, "37-hraj-nahravani", pixels);
    test::FakeDevice::Block input{std::vector<float>(512, .2f)};
    for (int i = 0; i < 16; ++i)
        fake.process(input, 512);
    click(studio, cs::stopSong);
    click(studio, cs::atStart);
    click(studio, cs::playSong);
    CHECK(bounds(cs::stopSong) == play);
    CHECK(bounds(cs::recordSong) == record);
    click(studio, cs::editMode);
    CHECK(bounds(cs::stopSong) == play);
    click(studio, cs::stopSong);
    click(studio, cs::mixMode);
    CHECK(bounds(cs::playSong) == play);
    REQUIRE(find<Slider>(studio, cs::master));
    click(studio, cs::closeInspector);
    CHECK_FALSE(find<Slider>(studio, cs::master));
    CHECK(bounds(cs::recordSong) == record);
    fits(studio);
    for (int pixels : {1280, 1920})
        snapshot(studio, "39-mix-sbaleny", pixels);
    click(studio, cs::openInspector);
    REQUIRE(find<Slider>(studio, cs::master));
    fits(studio);
}

TEST_CASE("Recording workflow offers the saved take, monitor help and explicit input repair", "[workflow]") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    const auto dir = temp.dir.getChildFile("song");
    Song song;
    song.title = "Riff";
    song.tracks = {{"g", "Kytara", "kytara", {"In 1"}, true}};
    REQUIRE(song.save(dir));
    ViewState view;
    view.click = view.countIn = false;
    view.leadBars = view.tailBars = 0;
    view.selectionEnd = song.barFrame(1);
    REQUIRE(view.save(dir));
    Settings config;
    config.visited = true;
    config.lastSong = dir.getFullPathName();
    REQUIRE(config.save(temp.file));
    Studio studio(fake, temp.file);
    click(studio, cs::resume);
    const auto process = [&](float level, int frames) {
        return fake.process({std::vector<float>(frames, level)}, frames);
    };

    const auto capture = [&](const char* scene) {
        fits(studio);
        for (int pixels : {1280, 1920})
            snapshot(studio, scene, pixels);
    };
    SECTION("Switching to edit follows the running transport") {
        song.bars = 24;
        song.grid.bpm = 300;
        song.grid.beats = 1;
        const auto backing = dir.getChildFile("Zvuk/pulse.wav");
        REQUIRE(backing.getParentDirectory().createDirectory().wasOk());
        auto writer = waveWriter(backing, song.grid.rate, 1);
        REQUIRE(writer);
        auto signal = [](int at) { return float((at % 1009) + 1) / 2048.f; };
        const int duration = int(song.end());
        AudioBuffer<float> sample(1, duration);
        for (int at = 0; at < duration; ++at)
            sample.setSample(0, at, signal(at));
        REQUIRE(writer->writeFromAudioSampleBuffer(sample, 0, duration));
        writer.reset();
        song.tracks.push_back({"pulse", "Podklad", "podklad"});
        song.tracks.back().pan = -1.f;
        song.assets.push_back({"a", "Zvuk/pulse.wav", duration, 1});
        song.clips.push_back({"c", "pulse", "a", 0, 0, duration});
        REQUIRE(song.save(dir));
        click(studio, cs::home);
        click(studio, cs::resume);
        click(studio, cs::playSong);
        for (int block = 0; block < 150; ++block)
            process(0, 1024);
        click(studio, cs::editMode);
        const auto state = ViewState::load(dir);
        CHECK(state.firstBar > 0);
        CHECK(state.firstBar <= 16);
        CHECK(state.firstBar + state.zoom > 16);
        capture("43-prechod-v-taktu");
        int expected = 150 * 1024;
        for (const char key : std::string("312132")) {
            REQUIRE(studio.keyPressed(KeyPress(key)));
            studio.resized();
            auto continuedAudio = process(0, 128);
            CHECK(std::abs(continuedAudio[0][0] - signal(expected)) < .00001f);
            CHECK(std::abs(continuedAudio[0][127] - signal(expected + 127)) < .00001f);
            expected += 128;
            const auto continued = ViewState::load(dir);
            CHECK(continued.trackId == "g");
            CHECK(continued.zoom == state.zoom);
            CHECK(continued.selectionStart == state.selectionStart);
            CHECK(continued.selectionEnd == state.selectionEnd);
        }
        REQUIRE(find<Button>(studio, cs::stopSong));
        fits(studio);
    }
    SECTION("Opening a reordered song retains the track by identity") {
        click(studio, cs::home);
        song.tracks.insert(song.tracks.begin(), {"b", "Basa", "jine", {"In 2"}});
        REQUIRE(song.save(dir));
        click(studio, cs::resume);
        auto* tracks = find<ComboBox>(studio, cs::audioTracks);
        REQUIRE(tracks);
        CHECK(tracks->getText() == "Kytara");
    }
    SECTION("One press plays the saved range and repeat keeps the original") {
        click(studio, cs::recordSong);
        process(.25f, 4096);
        click(studio, cs::stopSong);
        capture("42-ulozeny-pokus");
        REQUIRE(studio.keyPressed(KeyPress(KeyPress::F5Key)));
        studio.resized();
        auto heard = process(0, 4096);
        CHECK(std::abs(heard[0][1500]) > .05f);
        CHECK(heard[0][4095] == 0.f);
        tick();
        studio.resized();
        click(studio, cs::repeatTake);
        process(-.5f, 3072);
        click(studio, cs::stopSong);
        bool bad = false;
        auto saved = Song::load(dir, bad);
        REQUIRE(saved);
        REQUIRE_FALSE(bad);
        REQUIRE(saved->assets.size() == 2);
        REQUIRE(saved->groups.size() == 1);
        CHECK(saved->groups.front().active == 1);
        click(studio, cs::listenTake);
        heard = process(0, 4096);
        CHECK(heard[0][1500] < -.05f);
        CHECK(heard[0][3072] == 0.f);
        tick();
        studio.resized();
        REQUIRE(studio.keyPressed(KeyPress(KeyPress::F6Key)));
        studio.resized();
        process(.4f, 8192);
        click(studio, cs::stopSong);
        click(studio, cs::editMode);
        click(studio, cs::home);
        click(studio, cs::resume);
        click(studio, cs::attemptLabel + " 1");
        click(studio, cs::keepAttempt);
        heard = process(0, 4096);
        CHECK(heard[0][1500] > .05f);
        saved = Song::load(dir, bad);
        REQUIRE(saved);
        CHECK_FALSE(bad);
        CHECK(saved->groups.front().active == 0);
        CHECK(saved->assets.size() == 3);
    }
    SECTION("Repeating both armed tracks keeps their independent inputs in one session") {
        song.tracks.push_back({"bass", "Basa", "jine", {"In 2"}, true});
        REQUIRE(song.save(dir));
        click(studio, cs::home);
        click(studio, cs::resume);
        click(studio, cs::recordSong);
        fake.process({std::vector<float>(4096, .1f), std::vector<float>(4096, .2f)}, 4096);
        click(studio, cs::stopSong);
        click(studio, cs::disarm);
        click(studio, cs::repeatTake);
        fake.process({std::vector<float>(4096, .2f), std::vector<float>(4096, .1f)}, 4096);
        click(studio, cs::stopSong);
        CHECK(studio.statusText().contains("Kytara, Basa"));
        CHECK(studio.statusText().contains(cs::savedTakeLabel));
        bool bad = false;
        auto recorded = Song::load(dir, bad);
        REQUIRE(recorded);
        REQUIRE_FALSE(bad);
        REQUIRE(recorded->groups.size() == 2);
        CHECK(recorded->groups[0].sameSession(recorded->groups[1]));
        click(studio, cs::editMode);
        click(studio, cs::attemptLabel + " 1");
        click(studio, cs::keepAttempt);
        recorded = Song::load(dir, bad);
        REQUIRE(recorded);
        CHECK(recorded->groups[0].active == 0);
        CHECK(recorded->groups[1].active == 0);
    }
    SECTION("Monitoring can be silenced during capture") {
        const auto files = dir.getChildFile("Zvuk");
        REQUIRE(files.createDirectory().wasOk());
        auto writer = waveWriter(files.getChildFile("back.wav"), 48000, 1);
        REQUIRE(writer);
        AudioBuffer<float> backing(1, 8192);
        std::fill_n(backing.getWritePointer(0), backing.getNumSamples(), -.125f);
        REQUIRE(writer->writeFromAudioSampleBuffer(backing, 0, backing.getNumSamples()));
        writer.reset();
        song.tracks.push_back({"back", "Backing", "podklad"});
        song.tracks.back().pan = 1.f;
        song.assets.push_back({"a", "Zvuk/back.wav", 8192, 1});
        song.clips.push_back({"c", "back", "a", 0, 0, 8192});
        REQUIRE(song.save(dir));
        click(studio, cs::home);
        click(studio, cs::resume);
        chooseItem(studio, cs::listen, 1);
        click(studio, cs::recordSong);
        CHECK(process(.25f, 1024)[0][500] > .1f);
        click(studio, cs::monitorHelp);
        capture("44-dvoji-odposlech");
        click(studio, cs::monitorDisable);
        const auto mixed = process(.25f, 2048);
        CHECK(std::abs(mixed[0][1024]) < .00001f);
        CHECK(std::abs(mixed[1][1024] + .125f) < .00001f);
        CHECK(fake.active->read(0).peak > .2f);
        click(studio, cs::monitorRestore);
        CHECK(process(.25f, 1024)[0][500] > .1f);
        click(studio, cs::stopSong);
        bool bad = false;
        auto saved = Song::load(dir, bad);
        REQUIRE(saved);
        REQUIRE(saved->assets.size() == 2);
        auto wave = waveReader(dir.getChildFile(saved->assets.back().path));
        REQUIRE(wave);
        AudioBuffer<float> check(1, 1);
        REQUIRE(wave->read(&check, 0, 1, 2000, true, false));
        CHECK(std::abs(check.getSample(0, 0) - .25f) < .001f);
    }
    SECTION("Punch monitoring switches live for both clean and amplified guitar") {
        click(studio, cs::home);
        song.tracks.front().amp.enabled = true;
        REQUIRE(song.save(dir));
        click(studio, cs::resume);
        const int mode = GENERATE(1, 2);
        chooseItem(studio, cs::listen, mode);
        click(studio, cs::editMode);
        REQUIRE(studio.keyPressed(KeyPress('P')));
        studio.resized();
        REQUIRE(find<Button>(studio, cs::stopSong));
        auto sounding = process(.2f, 4096);
        CHECK(FloatVectorOperations::findMaximum(sounding[0].data(), 4096) > .001f);
        click(studio, cs::monitorHelp);
        click(studio, cs::monitorDisable);
        auto muted = process(.2f, 8192);
        CHECK(std::abs(muted[0].back()) < .0001f);
        CHECK(fake.active->live.monitor == Monitor::off);
        click(studio, cs::monitorRestore);
        auto restored = process(.2f, 4096);
        CHECK(FloatVectorOperations::findMaximum(restored[0].data(), 4096) > .001f);
    }
    SECTION("No receipt for an empty count-in or failed atomic commit") {
        click(studio, cs::countIn);
        click(studio, cs::recordSong);
        process(.25f, 128);
        click(studio, cs::stopSong);
        CHECK_FALSE(find<Button>(studio, cs::listenTake));
        click(studio, cs::countIn);
        click(studio, cs::recordSong);
        process(.25f, 2048);
        auto manifest = dir.getChildFile("skladba.json");
        REQUIRE(manifest.moveFileTo(dir.getChildFile("backup")));
        REQUIRE(manifest.createDirectory().wasOk());
        REQUIRE(manifest.getChildFile("block").replaceWithText("test"));
        click(studio, cs::stopSong);
        CHECK_FALSE(find<Button>(studio, cs::listenTake));
        CHECK(studio.statusText() == cs::takeNeedsRecovery);
    }
    SECTION("Only the missing side of a stereo track needs repair") {
        song.tracks[0].inputs = StringArray{"In 1", "In 2"};
        song.tracks.push_back({"other", "Basa", "jine", {"In 1"}});
        REQUIRE(song.save(dir));
        click(studio, cs::home);
        click(studio, cs::resume);
        fake.channels = 1;
        fake.changed();
        studio.resized();
        auto* missing = find<Button>(studio, cs::chooseInput);
        REQUIRE(missing);
        CHECK(missing->getTooltip().contains(cs::channel(1)));
        CHECK_FALSE(missing->getTooltip().contains(cs::channel(0)));
        click(studio, cs::chooseInput);
        click(studio, cs::useMono);
        click(studio, cs::apply);
        bool bad = false;
        auto mono = Song::load(dir, bad);
        REQUIRE(mono);
        CHECK(mono->tracks.front().inputs.size() == 1);
        CHECK(mono->tracks.back().inputs[0] == "In 1");
        fake.disconnect();
        fake.attached = true;
        fake.channels = 2;
        fake.changed();
        studio.resized();
        click(studio, cs::chooseInput);
        chooseItem(studio, cs::newInput + " 1", 1);
        click(studio, cs::apply);
        CHECK_FALSE(find<Button>(studio, cs::chooseInput));
        CHECK(find<Button>(studio, cs::chooseInput + cs::divider + "Basa"));
    }
    SECTION("An unplugged track stays in its song") {
        process(0, 256);
        CHECK_FALSE(find<Button>(studio, cs::chooseInput));
        fake.disconnect();
        studio.resized();
        REQUIRE(studio.currentPage() == Page::project);
        REQUIRE(find<Button>(studio, cs::chooseInput));
        capture("45-odpojeny-vstup");
        fake.attached = fake.laptop = true;
        fake.asio = false;
        fake.changed();
        studio.resized();
        click(studio, cs::recordSong);
        CHECK_FALSE(find<Button>(studio, cs::stopSong));
        click(studio, cs::chooseInput);
        chooseItem(studio, cs::newInput + " 1", 1);
        click(studio, cs::apply);
        click(studio, cs::recordSong);
        REQUIRE(find<Button>(studio, cs::stopSong));
        fake.changed();
        studio.resized();
        REQUIRE(find<Button>(studio, cs::stopSong));
    }
}

TEST_CASE("Metronome practice delivers stereo beats from the normal entry points", "[metronome]") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    Settings config;
    config.visited = true;
    REQUIRE(config.save(temp.file));
    Studio view(fake, temp.file);
    SECTION("Home") {}
    SECTION("Ideas") { click(view, cs::ideas); }
    SECTION("Play without a song") { click(view, cs::livePage); }
    SECTION("Setup") { click(view, cs::setup); }
    click(view, cs::navMetro);
    click(view, cs::playSong);
    BeatGrid grid;
    for (int beat = 0; beat < 16; ++beat) {
        const int length = int(grid.beat(beat + 1) - grid.beat(beat));
        auto output = fake.process({}, length);
        REQUIRE(output.size() == 2);
        REQUIRE(output[0] == output[1]);
        INFO(beat);
        CHECK(std::abs(output[0].front() - .16f) < 1.e-6f);
        CHECK(output[0].back() == 0.f);
    }
    chooseItem(view, cs::clickMode, 2);
    auto silent = fake.process({}, int(grid.beat(2)));
    CHECK(std::all_of(silent[0].begin(), silent[0].end(), [](float v) { return v == 0.f; }));
    chooseItem(view, cs::clickMode, 0);
    auto* volume = find<Slider>(view, cs::clickLevel);
    REQUIRE(volume);
    volume->setValue(0., sendNotificationSync);
    silent = fake.process({}, int(grid.beat(1)));
    CHECK(std::all_of(silent[0].begin(), silent[0].end(), [](float v) { return v == 0.f; }));
    volume->setValue(40., sendNotificationSync);
    CHECK(std::abs(fake.process({}, 128)[0][0] - .064f) < 1.e-6f);
    fits(view);
}

TEST_CASE("Metronome survives device list notifications and handles unplugging", "[metronome]") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    Settings config;
    config.visited = true;
    REQUIRE(config.save(temp.file));
    Studio view(fake, temp.file);
    click(view, cs::navMetro);
    click(view, cs::playSong);
    CHECK(fake.process({}, int(BeatGrid{}.beat(1)))[0][0] > .1f);
    fake.changed();
    REQUIRE(view.currentPage() == Page::metronome);
    CHECK(fake.process({}, 128)[1][0] > .1f);
    fake.disconnect();
    view.resized();
    CHECK(view.currentPage() == Page::device);
    CHECK_FALSE(fake.active);
}

TEST_CASE("Metronome preview and live toggles respect the song and keep its transport running", "[metronome]") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    const auto dir = temp.dir.getChildFile("Skladby/Klik");
    Song song;
    song.title = "Klik";
    song.bars = 4;
    song.tracks = {{"g", "Guitar", "kytara", {"In 1"}, true}};
    REQUIRE(song.save(dir));
    ViewState stored;
    stored.click = false;
    stored.countIn = false;
    stored.rhythm = GENERATE(false, true);
    REQUIRE(stored.save(dir));
    Settings config;
    config.visited = true;
    config.lastSong = dir.getFullPathName();
    REQUIRE(config.save(temp.file));
    Studio view(fake, temp.file);
    click(view, cs::resume);
    auto level = [&](int beat) {
        auto block = fake.process({}, int(song.grid.beat(beat + 1) - song.grid.beat(beat)));
        REQUIRE(block[0] == block[1]);
        return *std::max_element(block[0].begin(), block[0].end());
    };
    SECTION("Standalone preview is audible and Back preserves song settings") {
        click(view, cs::navMetro);
        click(view, cs::playSong);
        CHECK(level(0) > .01f);
        click(view, cs::back);
        REQUIRE(view.currentPage() == Page::project);
        CHECK_FALSE(ViewState::load(dir).click);
    }
    SECTION("Mouse and K reach the running playback or recording") {
        const bool recording = GENERATE(false, true);
        click(view, recording ? cs::recordSong : cs::playSong);
        CHECK(level(0) == 0.f);
        click(view, cs::guideOff);
        CHECK(level(1) > .01f);
        REQUIRE(view.keyPressed(KeyPress('K')));
        view.resized();
        CHECK(level(2) == 0.f);
        REQUIRE(view.keyPressed(KeyPress('k')));
        view.resized();
        CHECK(level(3) > .01f);
        click(view, cs::stopSong);
        if (recording) {
            bool damaged = false;
            auto saved = Song::load(dir, damaged);
            REQUIRE(saved);
            REQUIRE(saved->assets.size() == 1);
            auto dry = waveReader(dir.getChildFile(saved->assets[0].path));
            REQUIRE(dry);
            REQUIRE(dry->lengthInSamples == song.grid.beat(4));
            AudioBuffer<float> data(1, int(dry->lengthInSamples));
            REQUIRE(dry->read(&data, 0, data.getNumSamples(), 0, true, false));
            CHECK(data.getMagnitude(0, data.getNumSamples()) == 0.f);
        }
    }
}

TEST_CASE("Metronome reaches mono and stereo outputs at the device rate", "[metronome]") {
    ScopedJuceInitialiser_GUI gui;
    Temp temp;
    test::FakeDevice fake;
    fake.channels = 0;
    fake.rate = GENERATE(44100, 48000, 96000);
    fake.outputs = GENERATE(0, 1, 2);
    Settings config;
    config.visited = true;
    REQUIRE(config.save(temp.file));
    Studio view(fake, temp.file);
    click(view, cs::navMetro);
    auto* play = find<Button>(view, cs::playSong);
    REQUIRE(play);
    if (fake.outputs == 0) {
        CHECK_FALSE(play->isEnabled());
        REQUIRE(view.keyPressed(KeyPress(KeyPress::spaceKey)));
        view.resized();
        CHECK_FALSE(find<Button>(view, cs::stopSong));
        fits(view);
        return;
    }
    click(view, cs::playSong);
    const int frames = int(std::llround(8 * fake.rate * 60. / 112.));
    AudioBuffer<float> captured(fake.outputs, frames);
    const int sizes[]{64, 127, 256, 512};
    for (int pos = 0, block = 0; pos < frames; ++block) {
        const int length = std::min(sizes[block % 4], frames - pos);
        auto out = fake.process({}, length);
        for (int ch = 0; ch < fake.outputs; ++ch)
            captured.copyFrom(ch, pos, out[ch].data(), length);
        pos += length;
    }
    for (int ch = 0; ch < fake.outputs; ++ch) {
        for (int beat = 0; beat < 8; ++beat) {
            const auto at = int(std::llround(beat * fake.rate * 60. / 112.));
            const auto next = int(std::llround((beat + 1) * fake.rate * 60. / 112.));
            CHECK(std::abs(captured.getSample(ch, at) - .16f) < 1.e-6f);
            CHECK(captured.getMagnitude(ch, at + fake.rate / 80, next - at - fake.rate / 80) == 0.f);
        }
    }
    const auto path = SystemStats::getEnvironmentVariable("ROCKER_METRONOME_WAV", {});
    if (fake.rate == 48000 && fake.outputs == 2 && path.isNotEmpty()) {
        TemporaryFile temporary{File(path)};
        std::unique_ptr<OutputStream> stream = temporary.getFile().createOutputStream();
        REQUIRE(stream);
        auto writer = WavAudioFormat().createWriterFor(stream, AudioFormatWriterOptions{}
            .withSampleRate(fake.rate).withNumChannels(2).withBitsPerSample(16));
        REQUIRE(writer);
        REQUIRE(writer->writeFromAudioSampleBuffer(captured, 0, frames));
        writer.reset();
        REQUIRE(temporary.overwriteTargetFileWithTemporary());
    }
}
