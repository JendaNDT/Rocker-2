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
inline juce::String coming(int stage) {
    return juce::String(u8"Připravujeme (etapa E") + juce::String(stage) + ")";
}
inline juce::String bufferSize(int n) { return juce::String(n) + juce::String(u8" vzorků"); }
inline juce::String frequency(double hz) {
    return juce::String(hz / 1000, std::fmod(hz, 1000) == 0 ? 0 : 1).replaceCharacter('.', ',') + " kHz";
}
inline juce::String channel(int n) { return juce::String(input) + " " + juce::String(n + 1); }
} // namespace rocker::cs
