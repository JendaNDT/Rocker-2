#pragma once
#include "engine/Devices.h"
namespace rocker::cs {
inline const juce::String
    app = "Rocker 2",
    brand = "ROCKER", version = "2", setup = u8"Zapojení a zvukovka", record = u8"Nahrát nápad",
    song = u8"Nová skladba", resume = u8"Pokračovat", ideas = u8"Nápady", tuner = u8"Ladička",
    emptySongs = u8"Zatím tu žádná skladba není.", emptyIdeas = u8"Tady najdeš svoje uložené nápady.",
    library = u8"Otevřít knihovnu", home = u8"Úvodní obrazovka", device = "Zvukovka",
    disconnected = u8"Bez připojení", connect = u8"Připoj zvukovku nebo vyber mikrofon notebooku.",
    unplugged = u8"Zapojení se změnilo. Zkontroluj zařízení a vstupy.",
    failed = u8"Zařízení se nepodařilo otevřít. Zkus jiný ovladač nebo zařízení.",
    writeError = u8"Nastavení se nepodařilo uložit. Zkontroluj volné místo.", skip = u8"Přeskočit",
    back = u8"Zpět", next = u8"Další krok", finish = u8"Dokončit zapojení",
    refresh = u8"Vyhledat zařízení", driver = u8"Ovladač", inputPort = u8"Vstupní zařízení",
    outputPort = u8"Výstupní zařízení", bothPort = u8"Zařízení", rate = u8"Vzorkovací frekvence",
    buffer = "Velikost bufferu", noPort = u8"Žádné zařízení",
    bufferHelp = u8"Menší buffer zkrátí zpoždění. Při praskání zvol větší.",
    fallback = u8"Mikrofon notebooku je pro nápady. Pro hraní přes aparát připoj zvukovku.",
    laptop = u8"Používám mikrofon notebooku", card = u8"Používám externí zvukovku",
    play = u8"Pustit zkušební tón", again = u8"Přehrát znovu",
    question = u8"Slyšíš ho v obou sluchátkách?",
    noOutput = u8"Připoj výstup pro sluchátka nebo tento krok přeskoč.",
    stereo = u8"Zapoj sluchátka. Uslyšíš sekundu vlevo, sekundu vpravo a sekundu v obou.",
    mono = u8"Zařízení má jediný výstup. Uslyšíš tři sekundy tónu v jednom kanálu.",
    toneInfo = u8"440 Hz   /   −18 dBFS",
    inputHelp = u8"Brnkni na kytaru nebo udeř do bubnu. Pojmenuj vstup, který se rozsvítí.",
    clipHelp = u8"Při přebuzení uber GAIN na zvukovce. Kliknutím smažeš značku.", input = "Vstup",
    alias = u8"Název vstupu", db = " dBFS", silence = u8"−∞ dBFS", clip = u8"PŘEBUZENO",
    nameHint = u8"Kytara, bicí…", noInputs = u8"Žádný vstup. Vyber vstupní zařízení v prvním kroku.",
    steps = u8"Zapojení", emptyDash = u8"—", saved = u8"Zapojení je uložené.",
    fallbackChoice = u8"Náhradní režim pro mikrofon notebooku",
    permission = u8"Povol přístup k mikrofonu v nastavení soukromí a znovu vyhledej zařízení.";
inline const juce::String titles[] = {u8"Ovladač a zařízení", u8"Sluchátka", "Vstupy"};
inline const juce::String drivers[] = {"ASIO",
                                       u8"Windows Audio exkluzivní",
                                       u8"Windows Audio s nízkou latencí",
                                       u8"Windows Audio sdílený",
                                       "CoreAudio",
                                       "ALSA"};
inline const juce::String answers[] = {"Ano", "Jen v jednom", "Ne"};
inline const juce::String advice[] = {
    u8"Sluchátka jsou zapojená správně. Pokračuj ke vstupům.",
    u8"Zasuň stereo jack až na doraz a zkontroluj levý i pravý výstup. Pak tón zopakuj.",
    u8"Zapoj sluchátka do zvolené zvukovky, zvedni hlasitost sluchátek a zkus tón znovu."};
inline const juce::String hints[] = {
    u8"Ovladač výrobce má obvykle nejmenší zpoždění.",
    u8"Rocker 2 používá výstup sám. Ostatní aplikace mohou být během hraní potichu.",
    u8"Výstup můžeš sdílet. Tento režim zkracuje zpoždění u podporovaných zařízení.",
    u8"Výstup můžeš sdílet s dalšími aplikacemi. Při hraní může být slyšet zpoždění.",
    u8"Vyber vstup a sluchátka připojená ke stejné zvukovce.",
    u8"Vyber zařízení pro vstup a poslech."};
inline const juce::String findGuitar = u8"Najít vstup kytary", guitar = "Kytara",
                          pluck = u8"Teď brnkni jen na kytaru.",
                          ambiguous =
                              u8"Ozvalo se víc vstupů. Podle měřičů napiš název k tomu správnému.",
                          identified = u8"Signál kytary má ",
                          existing = u8". Jméno můžeš upravit v jeho řádku.";
inline juce::String coming(int stage) {
    return juce::String(u8"Připravujeme (etapa E") + juce::String(stage) + ")";
}
inline juce::String bufferSize(int n) { return juce::String(n) + juce::String(u8" vzorků"); }
inline juce::String frequency(double hz) {
    return juce::String(hz / 1000, std::fmod(hz, 1000) == 0 ? 0 : 1).replaceCharacter('.', ',') + " kHz";
}
inline juce::String channel(int n) { return juce::String(input) + " " + juce::String(n + 1); }
inline const juce::StringArray tuningNames = {"Standard",   "Drop D", "Drop C", "Drop B", "Eb standard",
                                              "D standard", "Open G", "Open D", "DADGAD"};
inline const juce::StringArray pitchNames = {"C",  "C#", "D",  "D#", "E",  "F",
                                             "F#", "G",  "G#", "A",  "A#", "B"};
inline const juce::StringArray presetNames = {juce::String(u8"Čistý"), juce::String("Crunch"),
                                              juce::String("Lead"), juce::String("Metal")};
inline const juce::StringArray sources = {
    juce::String(u8"Přímo do zvukovky"), juce::String(u8"Kombo s mikrofonem"),
    juce::String(u8"Linka z komba / multiefektu"), juce::String(u8"Čistá kytara i kombo")};
inline const juce::StringArray monitors = {juce::String("Vypnuto"), juce::String(u8"Čistě"),
                                           juce::String("Se zvukem")};
inline const juce::StringArray shapes = {juce::String(u8"Měkké"), juce::String(u8"Nesymetrické"),
                                         juce::String(u8"Tvrdé")};
inline const juce::String
    livePage = u8"Hraj",
    hear = u8"Co slyším", ampPage = u8"Zvuk kytary", diagnostics = u8"Diagnostika",
    source = u8"Zdroj kytary", directIn = u8"Vstup kytary", ampIn = u8"Vstup komba",
    listen = u8"Odposlech", preset = u8"Předvolba", headphones = u8"Ve sluchátkách",
    monitorOff = u8"Živý odposlech je vypnutý.", drySignal = u8"Čistý vstup",
    wetSignal = u8"Kytara přes aparát",
    directHelp = u8"Pro hraní přes aparát nastav DIRECT / MONITOR na zvukovce na PLAYBACK nebo USB.",
    comboHelp = u8"Kombo slyšíš z místnosti nebo přes přímý odposlech zvukovky.",
    bluetooth = u8"Bluetooth může přidat přes 100 ms. Pro hraní použij sluchátka s kabelem.",
    outputLevel = u8"Úroveň výstupu", delay = u8"Zpoždění ve sluchátkách",
    estimated = u8"Odhad ovladače", measured = u8"Změřeno kabelem",
    unknown = u8"Ovladač údaj neposkytl.", good = u8"dobré", usable = u8"použitelné",
    slow = u8"může rušit", noise = u8"Změř ticho",
    quiet = u8"Dvě sekundy nehraj. Brána změří šum vybraného vstupu.",
    gateSet = u8"Práh brány je nastaven.", gate = u8"Brána", drive = u8"Zkreslení",
    shape = u8"Druh zkreslení", bass = "Basy", middle = u8"Středy", treble = u8"Výšky", presence = "Jas",
    volume = "Hlasitost", cabinet = u8"Reprobox · vestavěná impulsní odezva", tuning = u8"Ladění",
    string = u8"Struna", automatic = u8"Podle tónu", ref = u8"Referenční tón",
    custom = u8"Vlastní ladění", saveTuning = u8"Uložit ladění", tuningName = u8"Název ladění",
    notesHint = u8"Struny od nejhlubší, např. D2 A2 D3 G3 B3 E4 (B = H).",
    badTuning = u8"Zadej vlastní název a 1 až 12 tónů s oktávou v rozsahu F1 až D6.",
    waiting = u8"Brnkni na jednu strunu.", inTune = u8"Naladěno", flat = u8"Přitáhni", sharp = u8"Povol",
    tunedStrings = u8"Naladěné struny", rig = u8"Kontrola sestavy", processor = u8"Procesor",
    load = u8"Zátěž zvukového vlákna", measure = u8"Změřit zpoždění",
    measuring = u8"Měřím návrat signálu…", delta = u8"Korekce", samples = u8" vz.",
    cableHelp =
        u8"Stáhni výstup na minimum. Propoj levý linkový výstup se vstupem. Vypni DIRECT MONITOR.",
    cableGain = u8"Opatrně přidej výstup. Po měření odpoj kabel, teprve potom zapni odposlech.",
    noReturn = u8"Signál se nevrátil dostatečně čistě. Zkontroluj kabel, vstup a hlasitost a zkus "
               u8"měření znovu.",
    needIO = u8"Vyber vstup i výstup v Zapojení.",
    notMeasured = u8"Pro toto zapojení zatím chybí měření.", io = u8"Vstupy / výstupy",
    directOption = u8"Při rušivém zpoždění vypni odposlech zde a použij DIRECT na zvukovce.",
    dbUnit = " dBFS", latencyUnit = " ms", percentUnit = " %", centUnit = u8" centů",
    customSaved = u8"Ladění je uložené.", soundOn = u8"Zapnout odposlech",
    soundOff = u8"Vypnout odposlech", ampDelay = u8"včetně aparátu", userNotes = u8"Tóny strun",
    openSetup = u8"Zařízení", listeningTo = u8"Poslechový vstup";
} // namespace rocker::cs
