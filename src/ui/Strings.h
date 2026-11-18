#pragma once
#include "engine/Devices.h"
namespace rocker::cs {
inline constexpr auto app="Rocker 2", brand="ROCKER", version="2", setup="Zapojení a zvukovka",
    record="Nahrát nápad", song="Nová skladba", resume="Pokračovat", ideas="Nápady", tuner="Ladička",
    emptySongs="Zatím tu žádná skladba není.", emptyIdeas="Tady najdeš svoje uložené nápady.",
    library="Otevřít knihovnu", home="Úvodní obrazovka", device="Zvukovka", disconnected="Bez připojení",
    connect="Připoj zvukovku nebo vyber mikrofon notebooku.", unplugged="Zapojení se změnilo. Zkontroluj zařízení a vstupy.",
    failed="Zařízení se nepodařilo otevřít. Zkus jiný ovladač nebo zařízení.",
    writeError="Nastavení se nepodařilo uložit. Zkontroluj volné místo.",
    skip="Přeskočit", back="Zpět", next="Další krok", finish="Dokončit zapojení", refresh="Vyhledat zařízení",
    driver="Ovladač", inputPort="Vstupní zařízení", outputPort="Výstupní zařízení", bothPort="Zařízení",
    rate="Vzorkovací frekvence", buffer="Velikost bufferu", noPort="Žádné zařízení",
    bufferHelp="Menší buffer zkrátí zpoždění. Při praskání zvol větší.",
    fallback="Mikrofon notebooku je pro nápady. Pro hraní přes aparát připoj zvukovku.",
    laptop="Používám mikrofon notebooku", card="Používám externí zvukovku",
    play="Pustit zkušební tón", again="Přehrát znovu", question="Slyšíš ho v obou sluchátkách?",
    noOutput="Připoj výstup pro sluchátka nebo tento krok přeskoč.",
    stereo="Zapoj sluchátka. Uslyšíš sekundu vlevo, sekundu vpravo a sekundu v obou.",
    mono="Zařízení má jediný výstup. Uslyšíš tři sekundy tónu v jednom kanálu.",
    toneInfo="440 Hz   /   −18 dBFS", inputHelp="Brnkni na kytaru nebo udeř do bubnu. Pojmenuj vstup, který se rozsvítí.",
    clipHelp="Při přebuzení uber GAIN na zvukovce. Kliknutím smažeš značku.",
    input="Vstup", alias="Název vstupu", db=" dBFS", silence="−∞ dBFS", clip="PŘEBUZENO",
    nameHint="Kytara, bicí…", noInputs="Žádný vstup. Vyber vstupní zařízení v prvním kroku.",
    steps="Zapojení", emptyDash="—", saved="Zapojení je uložené.", fallbackChoice="Náhradní režim pro mikrofon notebooku",
    permission="Povol přístup k mikrofonu v nastavení soukromí a znovu vyhledej zařízení.";
inline constexpr const char* titles[]={"Ovladač a zařízení","Sluchátka","Vstupy"};
inline constexpr const char* drivers[]={"ASIO","Windows Audio exkluzivní","Windows Audio s nízkou latencí",
    "Windows Audio sdílený","CoreAudio","ALSA"};
inline constexpr const char* answers[]={"Ano","Jen v jednom","Ne"};
inline constexpr const char* advice[]={"Sluchátka jsou zapojená správně. Pokračuj ke vstupům.",
    "Zasuň stereo jack až na doraz a zkontroluj levý i pravý výstup. Pak tón zopakuj.",
    "Zapoj sluchátka do zvolené zvukovky, zvedni hlasitost sluchátek a zkus tón znovu."};
inline constexpr const char* hints[]={"Ovladač výrobce má obvykle nejmenší zpoždění.",
    "Rocker 2 používá výstup sám. Ostatní aplikace mohou být během hraní potichu.",
    "Výstup můžeš sdílet. Tento režim zkracuje zpoždění u podporovaných zařízení.",
    "Výstup můžeš sdílet s dalšími aplikacemi. Při hraní může být slyšet zpoždění.",
    "Vyber vstup a sluchátka připojená ke stejné zvukovce.","Vyber zařízení pro vstup a poslech."};
inline juce::String coming(int stage) { return "Připravujeme (etapa E" + juce::String(stage) + ")"; }
inline juce::String bufferSize(int n) { return juce::String(n) + " vzorků"; }
inline juce::String frequency(double hz) { return juce::String(hz/1000, std::fmod(hz,1000)==0 ? 0 : 1).replaceCharacter('.', ',') + " kHz"; }
inline juce::String channel(int n) { return juce::String(input) + " " + juce::String(n+1); }
}
