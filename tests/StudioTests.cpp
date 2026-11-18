#include "FakeDevice.h"
#include "ui/Studio.h"
#include "ui/Strings.h"
#include <catch2/catch_test_macros.hpp>
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
        for (auto& name : {cs::song, cs::record, cs::resume, cs::library, cs::tuner}) {
            click(view, name);
            fits(view);
        }
    }
    for (int key : {'t', 'T', 'r', 'n', 'u'})
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
