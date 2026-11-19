#include "storage/ProjectFiles.h"
#include "FakeDevice.h"
#include "ui/Studio.h"
#include "ui/Strings.h"
#include <catch2/catch_test_macros.hpp>
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
        click(view, cs::resume);
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
    const int x0 = head->getX() - style::grid + style::trackHead;
    const int width = view.getWidth() - style::margin - x0;
    auto x = [&](double bar) { return float(x0 + bar * width / 16); };
    const int axisY = head->getY() - style::grid - style::gap - style::button / 2;
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
    int start = head->getX() - style::grid + style::trackHead;
    int width = view.getWidth() - style::margin - start;
    int axisY = head->getY() - style::grid - style::gap - style::button / 2;
    int tapeY = axisY - style::gap - style::button;
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
    CHECK(studio.currentPage() == Page::device);
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
