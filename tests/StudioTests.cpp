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
        File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("rocker-test", "", true);
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
        for (auto& name : {cs::record, cs::resume, cs::library}) {
            click(view, name);
            fits(view);
        }
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
    for (int key : {'t', 'T', 'r', 'u', 'n'})
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
