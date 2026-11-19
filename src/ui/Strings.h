#pragma once
#include "engine/Devices.h"
#include "song/SectionLabels.h"
namespace rocker::cs {
inline const juce::String packageSong=u8"Zabalit skladbu",openPackage=u8"Otevřít balík",cleanSong=u8"Uklidit skladbu",
    moveTrash=u8"Přesunout do koše",trashPreview=u8"Nepoužité soubory",nothingToClean=u8"Všechny nahrávky používá skladba nebo její historie.",
    transferDone=u8"Balík skladby je uložený.",transferFailed=u8"Přenos se nezdařil. Zkontroluj soubor a volné místo.",workingFiles=u8"Zpracovávám soubory…",
    diskLow=u8"Zbývá méně než 30 minut nahrávání.",diskStop=u8"Nahrávání zastaveno kvůli místu na disku. Uloženo do",diskFull=u8"Na disku je méně než 500 MB. Vyber jinou složku.",
    clockConflict=u8"Vstup a výstup mohou mít různé hodiny. Pro nahrávání vyber oba na stejné zvukovce.",clockOverride=u8"Povolit nahrávání i tak",
    syncPath=u8"Složku synchronizuje OneDrive. Pro nahrávání vyber místní složku mimo OneDrive.",libraryFolder=u8"Složka skladeb",
    remap=u8"Přiřadit vstupy",oldInput=u8"Původní vstup",newInput=u8"Nový vstup",remapLater=u8"Přiřadit později",noneAssigned=u8"Bez přiřazení",
    report=u8"Uložit zprávu",checkRig=u8"Kontrola sestavy",reportSaved=u8"Zpráva pro Jendu je uložená.",reportFailed=u8"Zprávu se nepodařilo uložit.",
    reportCPU=u8"Procesor",reportMidi=u8"Moduly MIDI",reportPorts=u8"Vstupy",reportOutput=u8"Výstup",reportObserved=u8"Zkoušky na skutečné sestavě";

inline const juce::String midiDrums = u8"Bicí (MIDI)", midiModule = u8"Modul MIDI",
                          padLearn = u8"Naučit pady", padPrompt = u8"Udeř do padu",
                          midiMode = u8"Co nahrávat", midiOnly = u8"Údery MIDI",
                          midiBoth = u8"Zvuk modulu i MIDI", midiDisabled = u8"Jen zvukový vstup",
                          kitPreset = u8"Zvuk sady", midiCompare = u8"Porovnat MIDI a zvuk",
                          compareHelp =
                              u8"Nahraj společně stereo a MIDI se 16 oddělenými údery do virblu.",
                          midiOffset = u8"Průměrný posun MIDI", gmMap = u8"Obnovit General MIDI",
                          openHat = u8"Otevřená hi-hat", closedHat = u8"Zavřená hi-hat",
                          lowTom = u8"Hluboký tom", highTom = u8"Vysoký tom", crash = u8"Činel crash",
                          ride = u8"Činel ride", midiNoDevice = u8"Připoj modul USB nebo MIDI.",
                          midiReady = u8"Údery se ukládají jako MIDI.", midiListen = u8"Poslech MIDI",
                          moduleListen = u8"Poslech modulu";
inline const juce::StringArray drumSounds{juce::String(u8"Studiová sada"), juce::String(u8"Krátká sada")};
inline const juce::String kitPort = u8"Vstup zvukovky", kitStrip = u8"Část soupravy";
inline const juce::String insertKit = u8"Přidat nastavené bicí";
inline const juce::String
    drumTest = u8"Zahraj na bicí",
    hitPrompt = u8"Udeř postupně do každého bubnu tak silně, jak budeš hrát.",
    finishHits = u8"Vyhodnotit údery", drumAgain = u8"Změřit znovu",
    noHits = u8"Žádný úder. Zkontroluj kabel, vstup a gain na zvukovce.",
    hitHot = u8"Stáhni gain na zvukovce pod −6 dBFS a zkus to znovu.",
    hitGood = u8"Úder má dostatečnou rezervu.", oneMic = u8"Jeden mikrofon",
    stereoKit = u8"Stereo modul L/P", multiKit = u8"Sada bicích", kitGain = u8"Bicí celkem",
    micAdvice = u8"Dej mikrofon asi 1–1,5 m před soupravu do výšky hlavy. Odposlech nech vypnutý.",
    stereoAdvice = u8"Zapoj výstupy L a R modulu do zvukovky. Sluchátka připoj ke zvukovce.",
    portAdvice = u8"Na víc mikrofonů potřebuješ víc vstupů. Můžeš začít jedním mikrofonem.",
    kitApply = u8"Použít pro nové skladby", kickName = u8"Kopák", snareName = u8"Virbl",
    tomName = u8"Tom", ohLeft = u8"Overhead L", ohRight = u8"Overhead P",
    kitWide = u8"Overheady vlevo a vpravo", kitCentre = u8"Overheady uprostřed", pedal = u8"Nožní spínač",
    pedalLearn = u8"Stiskni tlačítko pro Nahrát. Esc přiřazení zruší.",
    pedalBound = u8"Přiřazeno pro Nahrát", pedalClear = u8"Zrušit přiřazení",
    farOffer = u8"Zapnout Pohled zdálky při nahrávání bicích?", farReady = u8"Připraveno",
    farSaved = u8"Uloženo", farTarget = u8"Cílový takt", farFree = u8"Bez cíle",
    signalPeak = u8"Nejsilnější úder", kitRole = u8"Název stopy",
    kitHint = u8"Vstupy jsou seřazené podle zvukovky. Uprav názvy podle mikrofonů.";
inline const juce::String divider=u8" · ", rangeDash=u8" – ", starMark=u8"★ ";
inline const juce::String
    totalIdeas = u8"celkem",
    ideaTitle = u8"Nápad ", insurance = u8"Pojistka", insuranceOn = u8"Pojistka · 2:00",
    insuranceOff = u8"Pojistka vypnutá", keepHistory = u8"Ulož, co jsem hrál",
    emptyHistory = u8"Zatím není co uložit. Zahraj několik tónů.", ideaRecording = u8"NAHRÁVÁM NÁPAD",
    savingIdea = u8"Ukládám nápad…", ideaSaved = u8"Uloženo jako ", ideaStar = u8"★ Povedené",
    starredOnly = u8"Jen ★", allIdeas = u8"Všechny", note = u8"Poznámka", saveNote = u8"Uložit poznámku",
    searchNote = u8"Hledat v poznámkách", filterIdeas = u8"Vyhledat", nextIdeas = u8"Starší",
    prevIdeas = u8"Novější", useIdea = u8"Udělat z toho skladbu", tapIdea = u8"Doťukat tempo",
    tapHint = u8"Ťukni osmkrát mezerníkem do rytmu, začni na první době.",
    freeTempo = u8"Na volně hraný nápad mřížka sedí přibližně. Finální verzi nahraj s klikem.",
    badTaps = u8"Zkus osm pravidelných dob při 30 až 300 BPM.", addBacking = u8"Přidat podklad",
    backing = u8"Podklad", atCursor = u8"Na ukazatel", atBeginning = u8"Na začátek skladby",
    importing = u8"Převádím podklad…",
    importFailed =
        u8"Soubor se nepodařilo přečíst. Vyber podporovaný zvukový soubor s jedním nebo dvěma kanály.",
    badIdeaInput = u8"V Zapojení vyber zvukovku a vstup kytary.",
    previewRate = u8"Náhled se nepodařilo přehrát.", keepRecording = u8"Zastavit a uložit",
    chooseIdeaInput = u8"Vstup pro nápady", ideaClick = u8"Klik pro nápad",
    noMatches = u8"Tomuto výběru neodpovídá žádný nápad.";
inline const juce::String
    mixMode = u8"3 Míchej",
    sendMix = u8"Poslat mix…", gainLabel = u8"Hlasitost", panLabel = u8"Poloha L / P",
    master = u8"Celkem", soloStatus = u8"SÓLO · ", noSolo = u8"žádné", silentGain = u8"−∞",
    centre = u8"střed", leftShort = "L", rightShort = "P", mixPeak = u8"Špička",
    inputOver = u8"PŘEBUZENÝ VSTUP", mixOver = u8"PŘEBUZENÝ MIX", noOver = u8"Bez přebuzení",
    trackLabel = u8"Stopa", wholeSong = u8"Celá skladba", rangeOnly = u8"Vybrané takty",
    formatLabel = u8"Formát", rangeLabel = u8"Rozsah", wave24 = "WAV · 24 bit / 48 kHz",
    wave16 = "WAV · 16 bit / 44,1 kHz", mp3 = "MP3 · 320 kb/s",
    missingEncoder = u8"V tomto systému chybí kodér MP3. Obě možnosti WAV jsou dostupné.",
    soloNotice = u8"Je zapnuté sólo. V souboru uslyšíš jen vybrané stopy.",
    makeFile = u8"Vytvořit soubor", exportSolo = u8"Exportovat se sólem",
    exporting = u8"Vytvářím soubor…", playMix = u8"Přehrát výsledný soubor", reduceMix = u8"Ubrat o ",
    retryMix = u8" dB a opakovat export", mixFolder = u8"Otevřít složku", mixSuffix = u8" – mix ",
    exportFailed = u8"Soubor se nepodařilo vytvořit. Zkontroluj zdrojové nahrávky a místo na disku.",
    noPlayer = u8"Soubor otevři v přehrávači ze složky exportu.", checkClick = u8"Hledat klik v souboru",
    clickFound = u8"Na dobách je slyšet signál odpovídající kliku.",
    clickAbsent = u8"Opakující se klik se v souboru nenašel.",
    clickFailed = u8"Soubor WAV se nepodařilo přečíst.", clickBusy = u8"Hledám klik podle tempa skladby…",
    prevTracks = u8"Předchozí stopy", nextTracks = u8"Další stopy",
    mixTrackHint = u8"Vyber stopu pro S / M.";
inline const juce::String sections = u8"Díly", sectionSplit = u8"Rozdělit díl zde",
                          sectionRepeat = u8"Zopakovat díl", sectionDetach = u8"Osamostatnit",
                          sectionRestore = u8"Obnovit celou délku", sectionResize = u8"Délka výskytu",
                          sectionName = u8"Název dílu", sectionColour = u8"Barva pásky",
                          sectionRename = u8"Uložit název", sectionMove = u8"Přesunout díl",
                          sharedRepair = u8"Opravit oba", ownRepair = u8"Jen tento výskyt",
                          repeatWarning = u8"Oprava platí pro všechny výskyty.", repeatsAt = u8"se hraje",
                          repairAll = u8"Opravit všechny", totalBars = u8"taktů", outOf = u8" z ",
                          sectionCancel = u8"Zrušit";

inline const juce::String repair = u8"P · Oprav takt", repairState = u8"OPRAVA", attemptLabel = u8"Pokus",
                          takeGroup = u8"Pokusy", keepAttempt = u8"Ponechat tenhle",
                          discardAttempt = u8"Zahodit pokus", nextAttempt = u8"Další pokus",
                          originalTake = u8"Původní zahrání", restoredTake = u8"Vrátit z koše",
                          savedAttempt = u8"Vybraný pokus je uložený.",
                          discardedAttempt = u8"Pokus je v koši skladby.", preRoll = u8"Předstih",
                          postRoll = u8"Dohra", pickRepair = u8"Vyber stopu a takt, který chceš opravit.";
inline const juce::String editMode = u8"2 Stříhej", playMode = u8"1 Hraj", fileMenu = u8"Soubor",
                          versions = u8"Vrátit skladbu do stavu z…", versionTime = u8"Uložená verze",
                          applyVersion = u8"Obnovit tuto verzi",
                          noVersions = u8"Zatím tu není uložená verze.",
                          versionError = u8"Verzi se všemi nahrávkami se nepodařilo načíst.",
                          undo = u8"Zpět", redo = u8"Znovu", selectionTitle = u8"Výběr",
                          rangeHint = u8"Klikni na číslo taktu. Shift + klik přidá další takty.",
                          split = u8"Rozstřihnout", copy = u8"Kopírovat", paste = u8"Vložit",
                          erase = u8"Smazat", repeat = u8"Zopakovat", move = u8"Přesunout",
                          trim = u8"Okraj klipu", edited = u8"Úprava skladby",
                          copied = u8"Výběr je zkopírovaný.", magnifier = u8"Lupa ×4",
                          beforeBars = u8"Předchozí takty", afterBars = u8"Další takty",
                          zoomIn = u8"Přiblížit", zoomOut = u8"Oddálit",
                          editHint = u8"Táhni výběr po mřížce. Alt uvolní přichycení.",
                          noAudio = u8"Na tomto místě není vybraný zvuk.",
                          versionRestored = u8"Obnovení verze", newTake = u8"Nahrávání";
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
    echo = u8"Ozvěna",
    room = u8"Dozvuk", eq = u8"Korekce", speaker = u8"Reprobox", builtInCab = u8"Vestavěná IR",
    savePreset = u8"Uložit jako předvolbu", presetTitle = u8"Název předvolby",
    storePreset = u8"Uložit předvolbu", nameTaken = u8"Zvol nový název předvolby.",
    bypass = u8"Bez aparátu", useAmp = u8"Zapnout aparát", loadTrial = u8"Zkouška výkonu",
    ampCount = u8"Aparáty", trialRunning = u8"Zkouška výkonu běží · 16 stop, 10 minut.",
    trialFinished = u8"Deset minut dokončeno.", trialStopped = u8"Zkouška přerušena.",
    maxLoad = u8"Nejvyšší zátěž", xruns = u8"Výpadky", unreported = u8"ovladač nehlásí",
    livePage = u8"Hraj", hear = u8"Co slyším", ampPage = u8"Zvuk kytary", diagnostics = u8"Diagnostika",
    source = u8"Zdroj kytary", directIn = u8"Vstup kytary", ampIn = u8"Vstup komba",
    listen = u8"Odposlech", preset = u8"Předvolba", headphones = u8"Ve sluchátkách",
    monitorOff = u8"Živý odposlech je vypnutý.", drySignal = u8"Čistý vstup",
    wetSignal = u8"Kytara přes aparát",
    directHelp = u8"Pro hraní přes aparát nastav DIRECT / MONITOR na zvukovce na PLAYBACK nebo USB.",
    comboHelp = u8"Kombo slyšíš z místnosti nebo přes přímý odposlech zvukovky.",
    bluetooth = u8"Bluetooth může přidat přes 100 ms. Pro hraní použij sluchátka s kabelem.",
    outputLevel = u8"Úroveň výstupu", delay = u8"Zpoždění ve sluchátkách",
    estimated = u8"Odhad ovladače", measured = u8"Změřeno kabelem",
    unknown = u8"Ovladač údaj neposkytl.", good = u8"dobré", usable = u8"použitelné", slow = u8"ruší",
    noise = u8"Změř ticho", quiet = u8"Dvě sekundy nehraj. Brána změří šum vybraného vstupu.",
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
inline const juce::String& latencyRating(double milliseconds) {
    return milliseconds <= 10 ? good : (milliseconds <= 15 ? usable : slow);
}
inline const juce::String
    createSong = u8"Vytvořit skladbu",
    chooseSong = u8"Skladba", titleSong = u8"Název skladby", untitled = u8"Nová skladba",
    tempo = "Tempo (BPM)", meter = u8"Metrum", tap = u8"Naťukat tempo", apply = u8"Potvrdit",
    newTrack = u8"Přidat stopu", trackEdit = u8"Nastavit stopu", trackName = u8"Název stopy",
    trackKind = u8"Druh stopy", channels = u8"Kanály", noSong = u8"Zatím tu žádná skladba není.",
    recordSong = u8"Nahrávat", playSong = u8"Přehrát", stopSong = u8"Zastavit", atStart = u8"Na začátek",
    distantView = u8"Zdálky", normal = u8"Zpět k časové ose", bars = u8"Takty", verse = verseName,
    arm = u8"Připravit", disarm = u8"Připraveno", mute = u8"Ztlumit", solo = u8"Sólo",
    songClick = u8"Klik", countIn = u8"Odpočet", enabled = u8"Zapnuto", disabled = u8"Vypnuto",
    missingInputs = u8"Vyber vstup u každé připravené stopy.",
    rateMismatch = u8"V Zapojení nastav frekvenci skladby.", stateStop = u8"Zastaveno",
    statePlay = u8"Přehrávání", stateRecord = u8"NAHRÁVÁNÍ", bar = u8"Takt", toRecord = u8"Co nahrávám",
    noneArmed = u8"Žádná připravená stopa", projectSetup = u8"Tempo a metrum", savedSong = u8"Uloženo",
    badTempo = u8"Zadej tempo od 30 do 300 BPM a název skladby.", restore = u8"Obnovit",
    rescueTitle = u8"Obnovení skladby",
    rescueTake = u8"Na disku je nedokončená nahrávka. Přidat čitelný zvuk na původní místo?",
    rescueVersion = u8"Popis skladby nelze přečíst. Otevřít poslední čitelnou verzi?",
    badProject = u8"Skladbu se nepodařilo otevřít.",
    partialProject = u8"Některý klip nebo zvukový soubor se nepodařilo načíst.",
    recovered = u8"Nahrávka je obnovená.", dropout = u8"Výpadek zápisu",
    editTempoWarning = u8"Skladba obsahuje zvuk. Potvrzením přesuneš mřížku taktů.",
    syncTitle = u8"Zkouška souhry", syncRunning = u8"Zkouška souhry běží · 10 minut",
    syncBad = u8"Impulsy se nevrátily dostatečně silně.", syncFirst = u8"Začátek", syncLast = u8"Konec",
    syncPass = u8"Souhra vyhovuje.", syncFail = u8"Zkontroluj kalibraci a zapojení.",
    headphoneTracks = u8"Přehrávání stop", noDeviceSong = u8"Nejdřív vyber zvukovku v Zapojení.",
    useProject = u8"Zpět do skladby", inputMono = u8"Jeden vstup", inputPair = u8"Dva sousední vstupy",
    prepared = u8"Stopy připravené k nahrávání", audioTracks = u8"Stopy",
    availableSpace = u8"Volno na disku";
inline const juce::StringArray trackKinds{juce::String(u8"Bicí"), juce::String("Kytara"),
                                          juce::String(u8"Jiná")};
} // namespace rocker::cs
