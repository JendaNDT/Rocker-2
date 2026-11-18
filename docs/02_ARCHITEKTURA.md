# Rocker 2 — architektura

**Dokument 02 z 5.** Volba technologií, stavba aplikace, cesta živého a nahraného zvuku a pravidla ukládání. Dokument je psaný tak, aby ho implementující model (Astra nebo Sol) mohl použít samostatně. Funkce a vzhled popisuje dokument 01, zkoušky dokument 03 a pořadí prací dokument 04.

Značky: **[Ověřeno]** = potvrzeno ve zdrojovém kódu nebo dokumentaci (zdroj je v kapitole 16). **[Z]** = musí potvrdit praktická zkouška na Windows (dokument 03). **[Předpoklad]** = rozumný výchozí stav, který může změnit odpověď kamaráda.

---

## 1. Doporučení v kostce

| Oblast | Volba |
|---|---|
| **Jazyk** | C++20 |
| **Rámec** | **JUCE 9** (v době návrhu verze 9.0.5) pro okno, vlastní kreslené rozhraní, zvuková zařízení, zvukové soubory, stavební bloky DSP a fonty |
| **Zvukové jádro** | **Vlastní, malé jádro** nad `juce::AudioIODeviceCallback`. Nepoužívá `AudioProcessorGraph` ani Tracktion Engine. |
| **Ovladače Windows** | **ASIO** (SDK je přímo v JUCE od verze 8.0.11, licence GPLv3) jako první volba. **WASAPI exkluzivní** a **WASAPI sdílený nízkolatenční** jako záloha. **WASAPI sdílený** pro náhradní režim bez zvukovky. |
| **MIDI a zvuky bicích** | Vstup MIDI přes JUCE (`MidiInput`), vlastní jednoduchý přehrávač vzorků bicích postavený na `juce::Synthesiser`. Údery se ukládají jako standardní soubory `.mid`. |
| **Neznámé vybavení** | Žádné pevné předpoklady. Zařízení, vstupy, frekvence, MIDI a procesor se zjišťují za běhu a rozhraní nabízí jen to, co dává smysl (dokument 01, kap. 7). |
| **Model skladby** | Neměnné snímky v C++ (každá úprava vytvoří nový snímek) a historie Zpět ze snímků. Ukládá se do JSON. |
| **Ukládání** | Složka skladby: `skladba.json`, zvuk jako 32bitové float WAV, které se nikdy nepřepisují. Atomické ukládání, deník běžícího nahrávání a historie verzí. |
| **Build a testy** | CMake, MSVC (Windows **x64 i ARM64**), Clang (macOS pro vývoj), testy Catch2. CI na GitHub Actions: `windows-2025` a `macos`. |
| **Balení** | Instalátor Inno Setup a přenosný ZIP. Ve v1 nepodepsané (Windows SmartScreen se jednou zeptá). |
| **Licence** | Veřejný repozitář s licencí **AGPLv3** (JUCE pod AGPLv3, ASIO SDK pod GPLv3). Alternativa s uzavřeným kódem je v kapitole 14. |

**Hlavní důvody volby:**

1. Kamarád potřebuje hrát přes aparát do sluchátek s malým zpožděním. **Na Windows to spolehlivě umí jen nativní aplikace s ASIO.** JUCE má ASIO i všechny režimy WASAPI vestavěné.
2. Jenda vyvíjí z Macu a **JUCE aplikace se sestaví a spustí i na Macu**. Rozhraní, logiku a testy lze dělat na Macu a Windows sestavení dělá CI.
3. JUCE už obsahuje přesně ty díly, které jinak představují největší riziko: zápis WAV, který přežije pád (`flush`), čtení souborů mapováním do paměti, konvoluci pro reprobox, převzorkování pro zkreslení, vkládání fontů a ukázkový program pro měření zpoždění.
4. Vlastní jádro je malé (desítky stop, žádné pluginy ve v1). Díky tomu zůstává pochopitelné a testovatelné, a pravidla jako *spojené díly* nebo *záchrana nahrávky po pádu* jsou plně pod naší kontrolou.

---

## 2. Porovnání reálných možností

| | **A. JUCE + vlastní jádro** (doporučeno) | **B. JUCE + Tracktion Engine** | **C. Rust (cpal) + Tauri (web UI)** | **D. C#/.NET + NAudio + WPF** | **E. Webová aplikace (Web Audio), jako starý Rocker** |
|---|---|---|---|---|---|
| **ASIO / nízké zpoždění** | Ano, ASIO SDK přímo v JUCE [Ověřeno] | Ano (přes JUCE) | ASIO jako volitelná funkce, vyžaduje LLVM/Clang a SDK při sestavení [Ověřeno] | Ano (NAudio AsioOut) | **Ne.** Prohlížeč nepoužívá ASIO. Starý Rocker sám odhaduje 15–35 ms na Windows ([ARCHITECTURE.md, kap. 3.5][r1-latency]). |
| **Vývoj z Macu** | Sestaví se a běží i na Macu | Totéž | Totéž | **Ne.** WPF i ASIO jsou jen pro Windows a na Macu nejde nic spustit. | Ano |
| **Hotové stavební bloky** | Zvukové soubory (WAV/RF64, FLAC, Ogg, Opus, MP3 čtení), DSP (konvoluce, převzorkování, brána, dozvuk, panoráma), Zpět, fonty | **Nejvíc:** časová osa, klipy, pokusy, nahrávání s kompenzací, render | Málo: WAV (hound), DSP a konvoluci je potřeba dopsat | Střední | Střední, ale omezené prohlížečem |
| **Riziko pro AI implementaci** | Střední. JUCE je velmi rozšířené a dobře zdokumentované. | Vyšší. Velké API s řídkou dokumentací a ohýbání jeho modelu na spojené díly. | Střední až vyšší. Dvě řeči (Rust + TypeScript) a víc vlastního DSP. | Střední, ale bez možnosti zkoušet na Macu | Nízké, ale nesplní zadání |
| **Licence** | AGPLv3, nebo JUCE licence (Starter zdarma do limitu příjmu) | GPLv3, nebo komerční (Tracktion) + JUCE | Volné (MIT/Apache) | Volné | Volné |
| **Verdikt** | **Doporučeno** | Záložní cesta | Zamítnuto: víc práce na stejném výsledku | Zamítnuto: nejde zkoušet na Macu | Zamítnuto: latence a vstupy |

**Proč ne B, i když umí nejvíc hotového:** Tracktion Engine je výborný a ověřený (stojí na něm DAW Waveform), ale je to velký rámec s vlastním modelem skladby. Rocker 2 potřebuje několik věcí jinak: spojené výskyty dílů, záchranu nahrávky s deníkem a export, který je z konstrukce shodný s poslechem. Ty bychom museli do cizího modelu vklínit. Vlastní jádro pro 16 stop bez pluginů je menší práce, než se zdá, a hlavně ho implementující model celé pochopí. **Pokud etapa E2 (dokument 04) ukáže, že vlastní jádro neplní zkoušky M-02 a A-05, přechod na B je možný**, protože stojí na stejném JUCE a rozhraní i DSP zůstanou.

---

## 3. Ověřená fakta a co musí potvrdit zkouška

| # | Tvrzení | Stav | Zdroj (kap. 16) |
|---|---|---|---|
| F1 | Aktuální JUCE je 9.0.5. JUCE 9 má stejné ceny a licenční podmínky jako JUCE 8. Moduly JUCE jsou pod AGPLv3, nebo pod JUCE licencí. | [Ověřeno] | S1, S2, S3, S4 |
| F2 | JUCE licence Starter je zdarma do 20 000 USD ročního příjmu. | Z fóra JUCE, oficiální stránku juce.com nebylo z této relace možné otevřít. **Ověřit ručně na juce.com.** | S5 |
| F3 | JUCE 8.0.11 přidalo ASIO SDK přímo do zdrojů. Licence SDK: *Steinberg ASIO License* **nebo GPLv3**. | [Ověřeno] | S2, S6, S7 |
| F4 | JUCE nabízí WASAPI v režimech `shared`, `exclusive` a `sharedLowLatency` (přes `IAudioClient3`). | [Ověřeno] ve zdrojovém kódu | S8 |
| F5 | Sdílený nízkolatenční režim WASAPI funguje jen s ovladači, které ho podporují (WaveRT). Úspěch pod 10 ms není zaručen. | Komunitní zdroje. [Z] zkouška M-01 | S9 |
| F6 | `AudioFormatWriter::flush()` u WAV přepíše hlavičku, takže soubor zůstane čitelný i po pádu. `ThreadedWriter::setFlushInterval()` volá flush automaticky. Nad 4 GB se soubor sám přepne na RF64. | [Ověřeno] ve zdrojovém kódu | S10, S11 |
| F7 | `MemoryMappedAudioFormatReader` má `mapSectionOfFile()` a `touchSample()` pro načtení stránek do paměti předem. | [Ověřeno] | S12 |
| F8 | `TemporaryFile::overwriteTargetFileWithTemporary()` na Windows nahrazuje soubor funkcí `ReplaceFile`. | [Ověřeno] ve zdrojovém kódu. [Z] zkouška A-09 | S12 |
| F9 | `AudioIODevice` hlásí vstupní a výstupní latenci ovladače (`getInputLatencyInSamples`, `getOutputLatencyInSamples`) a počet výpadků (`getXRunCount`). | [Ověřeno]. Přesnost údaje ovladače [Z], zkouška M-01 | S12 |
| F10 | JUCE `MP3AudioFormat` umí MP3 jen **číst** (je zapnutý od 9.0.2). Kodér MP3 v JUCE není. Opus umí číst i zapisovat od 9.0.3. | [Ověřeno] | S2, S13 |
| F11 | Windows mají vestavěný kodér MP3 (Media Foundation MFT) od Windows 8, jen pro desktopové aplikace. | [Ověřeno] přes Microsoft Learn (ve výsledcích vyhledávání). Edice Windows „N“ bez Media Feature Pack ho nemusí mít [Z]. | S14 |
| F12 | JUCE obsahuje `dsp::Convolution`, `dsp::Oversampling`, `dsp::NoiseGate`, `dsp::Reverb`, `dsp::Panner`, `dsp::Limiter`, `ValueTree`, `UndoManager` a příklad `AudioLatencyDemo`. | [Ověřeno] ve stromu repozitáře | S12 |
| F13 | `Typeface::createSystemTypefaceFor(data, size)` načte font z dat vložených v programu. | [Ověřeno] | S12 |
| F14 | JUCE 9.0.1 zvýšilo minimální verzi Windows na 1607. | [Ověřeno] | S2 |
| F15 | Tracktion Engine je pod GPLv3, nebo komerční licencí. | [Ověřeno] | S15 |
| F16 | cpal (Rust) podporuje WASAPI a volitelně ASIO; ASIO vyžaduje LLVM/Clang a SDK. | [Ověřeno] | S16 |
| F17 | NeuralAmpModelerCore (pro rozšíření NAM) má licenci MIT. | [Ověřeno] | S17 |
| F18 | Fonty Inter a JetBrains Mono mají licenci SIL OFL 1.1. libsamplerate má licenci BSD-2-Clause. | [Ověřeno] | S18, S19, S20 |
| F19 | Standardní runnery GitHub Actions jsou pro veřejné repozitáře zdarma; `windows-2025` má pro veřejné repozitáře 4 vCPU a 16 GB RAM. | [Ověřeno] přes dokumentaci GitHub (ve výsledcích vyhledávání) | S21 |
| F20 | Azure Artifact Signing pro jednotlivce (veřejná důvěra) je jen pro USA a Kanadu. Ani podpis nedává okamžitou důvěru SmartScreenu. | [Ověřeno] přes Microsoft Learn (ve výsledcích vyhledávání) | S22 |
| F21 | OneDrive „Known Folder Move“ přesouvá Plochu, Dokumenty a Obrázky. | [Ověřeno] přes Microsoft Learn (ve výsledcích vyhledávání) | S23 |
| F22 | `SetThreadExecutionState(ES_CONTINUOUS \| ES_SYSTEM_REQUIRED)` brání uspání, dokud se stav nezruší. | [Ověřeno] přes Microsoft Learn (ve výsledcích vyhledávání) | S24 |
| F23 | JUCE podporuje sestavení pro Windows na ARM (Arm64 a Arm64EC). | [Ověřeno] v `BREAKING_CHANGES.md` a `juce_TargetPlatform.h` | S12 |
| F24 | JUCE obsahuje vstup MIDI (`juce_MidiDevices.h`), `Synthesiser` a `SamplerSound` (`juce_Sampler.h`) pro přehrávání vzorků. | [Ověřeno] ve stromu repozitáře | S12 |

> **Omezení průzkumu:** domény `juce.com`, `forum.juce.com` a `learn.microsoft.com` byly v síťovém nastavení této relace blokované. Tvrzení z nich mám z výsledků vyhledávání a ze zdrojového kódu JUCE na GitHubu. U F2 a F11 doporučuji jedno ruční ověření.

**Co musí potvrdit praktická zkouška na Windows** (podrobně v dokumentu 03):

- skutečné zpoždění tam a zpět s kamarádovou zvukovkou (ASIO vs. WASAPI) a jestli při hře ruší (M-01, H-01),
- že nová stopa sedí k bicím na začátku i na konci 10minutové skladby (M-02),
- že ovladač hlásí latenci pravdivě, nebo jakou korekci potřebuje (M-01),
- výkon notebooku: počet stop s aparátem bez výpadků zvuku (M-05),
- záchrana nahrávky po násilném ukončení i při skutečném výpadku proudu (A-08, M-07),
- dostupnost kodéru MP3 na kamarádově Windows (A-12),
- čitelnost Pohledu zdálky ze 3 m (H-05).

---

## 4. Jak na sebe části aplikace navazují

### 4.1 Vlákna a moduly

```
┌──────────────────────── VLÁKNO ROZHRANÍ (message thread) ─────────────────────────┐
│ ui/        Stavový pás · Hlavy stop · Díly a pravítko · Spodní deska · Zdálky      │
│              ▲ čte stav jádra 30×/s (atomické hodnoty)    │ uživatel udělá úpravu  │
│              │                                            ▼                        │
│ model/     Skladba = neměnný snímek ──► Příkaz (např. ZkrátitVýskyt) ──► nový snímek│
│            Historie Zpět/Znovu = zásobník snímků                                   │
│              │ po každém novém snímku: sestav PlánPřehrávání                       │
│ storage/   Ukladač (odešle snímek na pozadí) · Verze · Balíky · Záchrana          │
└──────────────┬─────────────────────────────────────────────────────────────────────┘
               │ výměna ukazatele na hotový plán (bez zámku), starý plán se uvolní v UI
┌──────────────▼─────────────────── ZVUKOVÉ VLÁKNO (reálný čas) ─────────────────────┐
│ engine/    vstupy ─► měřiče ─► Pojistka (kruhová paměť) ─► fronta nahrávání        │
│            vstupy ─► odposlech (aparát stopy) ─► sběrnice ODPOSLECH ─┐             │
│            plán ─► čtení klipů ─► aparát / hlasitost / poloha ─► sběrnice MIX ─┼─► výstup
│            klik a odpočet ─► sběrnice KLIK (jen sluchátka) ────────────────────┘             │
└──────────────┬─────────────────────────────────────────────────────────────────────┘
               │ fronta bez zámku (předem alokovaná, asi 10 s všech vstupů)
┌──────────────▼──────────── DISKOVÉ VLÁKNO ─────────────────────────────────────────┐
│ Zapisovač WAV (ThreadedWriter, flush 1×/s) · deník Záchrany · hlídání místa         │
└────────────────────────────────────────────────────────────────────────────────────┘
  PRACOVNÍ VLÁKNO: náhledy průběhů · import a převod frekvence · export (offline render)
```

### 4.2 Adresáře zdrojového kódu

| Modul | Odpovědnost |
|---|---|
| `app/` | Start aplikace, hlavní okno, klávesové zkratky (5.10 v dokumentu 01), přepínání režimů, nastavení počítače |
| `ui/` | Designové tokeny (barvy, písma, rozměry z kap. 5.2 dokumentu 01), vlastní `LookAndFeel`, komponenty: `StavovyPas`, `HlavaStopy`, `PasDilu`, `Pravitko`, `PlochaStop`, `Lupa`, `SpodniDeska` (Co slyším, Pokusy, Zvuk kytary, Celkem), `PohledZdalky`, `UvodniObrazovka`, `KnihovnaNapadu`, `Pruvodce` (Zapojení), `Ladicka` |
| `model/` | Datové typy skladby (kap. 5), čisté funkce úprav (`rozdelitDil`, `zopakovatDil`, `zkratitVyskyt`, `osamostatnit`, `vytvoritPokus`…), historie, validace a migrace JSON |
| `engine/` | `ZvukovaZarizeni` (výběr ovladače, frekvence, bufferu, latence), `RozpoznaniSestavy` (počet vstupů, MIDI zařízení, který vstup ožil, Kontrola sestavy), `MidiVstup`, `Jadro` (zvukové zpětné volání), `PlanPrehravani`, `Nahravac`, `Pojistka`, `Metronom`, `Merice`, `Renderer` (export) |
| `dsp/` | `Aparat` (brána, zkreslení s převzorkováním, korekce, reprobox, ozvěna, dozvuk), `SadaBici` (přehrávač vzorků pro MIDI bicí), `Ladicka` (detekce výšky YIN/MPM), `Klik`, `Panorama`, `Prolnuti` |
| `storage/` | `UkladacSkladby` (atomický zápis), `Verze`, `Zachrana` (deník), `Balik` (zip `.rocker2`), `Napady`, `Nahledy` (průběhy) |
| `platform/` | Specifika Windows: zákaz spánku, volné místo, rozpoznání OneDrive, cesty, úroveň oprávnění |
| `tests/` | Catch2: model, jádro s falešným zařízením, render, záchrana (podproces se násilně ukončí), snímky obrazovek rozhraní |

### 4.3 Pravidla zvukového vlákna (závazná)

Zvukové vlákno **nikdy nečeká**. Konkrétně v něm nesmí být:

- zámky (mutex), alokace paměti, práce se soubory, logování ani volání rozhraní.

Komunikace probíhá jen dvěma způsoby:

- **atomické hodnoty** (jezdce, stav transportu, měřiče),
- **předem alokované fronty bez zámku** (nahrávání, události jako „přebuzení v taktu 37“ nebo „výpadek zvuku“).

Ověřuje se třemi cestami: kontrolním seznamem při revizi kódu, v ladicím sestavení hlídáním doby zpětného volání a počtu výpadků, a nástrojem ThreadSanitizer při testech na Macu.

---

## 5. Datový model skladby

### 5.1 Pojmy

| Pojem v rozhraní | Typ v kódu | Obsah |
|---|---|---|
| Skladba | `Song` | název, `schemaVersion`, vzorkovací frekvence, tempo (BPM), metrum, ladění, komorní A, stopy, díly, pořadí dílů, zvukové soubory |
| Stopa | `Track` | id, název, druh (`kytara`, `bici`, `sadaBici`, `biciMidi`, `podklad`, `jine`), zdroj kytary (`primo`, `komboMikrofon`, `komboLinka`, `oboji`), u MIDI bicích zvolená vestavěná sada, barva, kanály (1/2), přiřazení vstupu podle názvu, režim odposlechu (`seZvukem`, `cisty`, `vypnuto`), mix (`gainDb`, `pan`, `mute`, `solo`), `amp` (kopie nastavení aparátu + název předvolby + příznak upraveno), `groupId` u sady bicích |
| Díl | `Section` | id, název, barva, délka v taktech, **klipy** a **skupiny pokusů** s polohou vztaženou k začátku dílu |
| Výskyt dílu | `SectionInstance` | id, `sectionId`, `cropStartBars`, `lengthBars` (zkrácení výskytu) |
| Pořadí | `arrangement` | seznam výskytů za sebou |
| Klip | `Clip` | id, `trackId`, `assetId`, `sourceStart` (vzorky v souboru), `length`, `position` (vzorky od začátku dílu), `gainDb`, `fadeInMs`, `fadeOutMs` |
| Pokusy | `TakeGroup` | id, `trackIds` (1 stopa nebo celá sada bicích), rozsah v dílu, `takes[]` (každý pokus = sada klipů), `activeTakeId` |
| Zvukový soubor | `AudioAsset` | id, relativní cesta, kanály, frekvence, délka, vznik (`nahravka`, `import`, `napad`), čas vzniku |
| Záznam úderů | `MidiAsset` | id, relativní cesta k souboru `.mid`, délka. Klip MIDI bicích ukazuje na něj stejně jako zvukový klip na `AudioAsset`, takže pokusy, díly a střih fungují stejně. |

### 5.2 Pravidla modelu

- **Polohy jsou ve vzorcích** (`int64`) při frekvenci skladby. Takty se z nich počítají z tempa. Začátek taktu `n` = `round(n × dobyVTaktu × vzorkuNaDobu)`. Počítá se vždy z indexu, nikdy přičítáním, aby se chyba zaokrouhlení nesčítala.
- **Tempo je ve v1 jedno na skladbu.** Po prvním nahraném zvuku se mění jen po potvrzení (dokument 01, 4.5).
- **Časová osa = výskyty dílů za sebou.** Začátek výskytu se počítá z délek předchozích výskytů v taktech. Zkrácení jednoho výskytu proto posune všechno za ním a nikde nevznikne díra.
- **Spojené výskyty** sdílejí `sectionId`. *Osamostatnit* vytvoří nový `Section` s kopiemi klipů. Ty ukazují na **tytéž** zvukové soubory, takže na disku nic nepřibude.
- **Nahrávka, která přeteče hranu dílu**, se rozdělí na klipy v obou dílech, které ukazují na stejný soubor s různým `sourceStart`.
- **Pokus má přednost** před základními klipy v rozsahu skupiny pokusů. Na hranách se automaticky prolne 10 ms.
- **Úprava = čistá funkce** `Song → Song`. Historie Zpět drží předchozí snímky; protože se nekopíruje zvuk, je to levné.
- **Načítání je tolerantní:** neznámá pole se ignorují, poškozený klip se vyřadí s hlášením, starší `schemaVersion` se převede migrací. Při poškozeném `skladba.json` se nabídne poslední dobrá verze z `Verze/`.

### 5.3 Ukázka `skladba.json` (zkráceno)

```json
{
  "schemaVersion": 1,
  "nazev": "Noční riff",
  "sampleRate": 48000,
  "tempoBpm": 112.0,
  "metrum": { "dobyVTaktu": 4, "jednotka": 4 },
  "ladeni": { "id": "drop-d", "komorniA": 440.0 },
  "stopy": [
    { "id": "t1", "nazev": "Bicí", "druh": "bici", "kanaly": 2,
      "vstup": { "nazvy": ["In 3", "In 4"] }, "odposlech": "vypnuto",
      "mix": { "gainDb": -2.0, "pan": 0.0, "mute": false, "solo": false } },
    { "id": "t2", "nazev": "Kytara", "druh": "kytara", "kanaly": 1,
      "vstup": { "nazvy": ["In 1"] }, "odposlech": "seZvukem",
      "mix": { "gainDb": -4.5, "pan": -0.25, "mute": false, "solo": false },
      "amp": { "predvolba": "Crunch", "upraveno": true,
               "brana": -62, "zkresleni": 0.48, "typ": "distortion",
               "basy": 0.1, "stredy": 0.2, "vysky": 0.1, "jas": 0.0,
               "reprobox": "moderni-412", "ozvena": 0.0, "dozvuk": 0.12, "hlasitost": 0.5 } }
  ],
  "dily": [
    { "id": "s1", "nazev": "Sloka", "barva": "sloka", "delkaTaktu": 8,
      "klipy": [ { "id": "c1", "trackId": "t1", "assetId": "a1",
                   "sourceStart": 336, "length": 822857, "position": 0,
                   "fadeInMs": 0, "fadeOutMs": 10 } ],
      "pokusy": [] },
    { "id": "s2", "nazev": "Refrén", "barva": "refren", "delkaTaktu": 8, "klipy": [], "pokusy": [] }
  ],
  "poradi": [
    { "id": "i1", "sectionId": "s1", "cropStartBars": 0, "lengthBars": 8 },
    { "id": "i2", "sectionId": "s2", "cropStartBars": 0, "lengthBars": 8 },
    { "id": "i3", "sectionId": "s2", "cropStartBars": 0, "lengthBars": 6 }
  ],
  "zvuky": [
    { "id": "a1", "soubor": "Zvuk/bici-p1-20261118-2102.wav", "kanaly": 2,
      "sampleRate": 48000, "delka": 912000, "vznik": "nahravka" }
  ]
}
```

Výskyt `i3` je **druhý refrén zkrácený na 6 taktů**. Výskyt `i2` je beze změny. Vrácení znamená nastavit `lengthBars` zpět na 8 (nebo Zpět v historii).

### 5.4 Co není ve skladbě, ale v nastavení počítače

Soubor `%APPDATA%\Rocker 2\nastaveni.json` obsahuje: ovladač a zařízení, rozpoznanou sestavu, přiřazení padů MIDI (Naučit pady) pro každý modul, preferovaný buffer, kalibraci zpoždění pro každou kombinaci (zařízení, frekvence, buffer), mapování názvů vstupů, klávesy nožního spínače, umístění složky Skladby a zapnutí Pojistky. Díky tomu se skladba přenese na jiný počítač bez cizích nastavení zvukovky.

---

## 6. Kudy prochází živý zvuk

### 6.1 Jeden blok zvukového zpětného volání (např. 128 vzorků)

1. **Vstupy** ze zvukovky (N kanálů, float).
2. **Měřiče vstupů:** špička vzorku v dBFS a **přebuzení vstupu**, pokud se vzorek dotkne −0,1 dBFS nebo víc. Událost jde do fronty pro rozhraní.
3. **Pojistka:** aktivní vstupy se zapíšou do kruhové paměti (2 min + 5 s rezerva, strop 256 MB).
4. **Nahrávání:** pokud běží, vstupy nahrávaných stop jdou do fronty nahrávání se značkou polohy na časové ose.
5. **Odposlech:** u nahrávaných stop (a u vybrané stopy se vstupem, když se nenahrává) jde vstup do **aparátu té stopy** (je-li odposlech *Se zvukem*), pak přes hlasitost a polohu stopy na sběrnici **ODPOSLECH**.
6. **Přehrávání:** pro každou stopu se z plánu přečtou klipy pro tento blok (soubory mapované do paměti), u kytary projdou aparátem stopy, pak hlasitostí a polohou. Výsledek jde na sběrnici **MIX**.
7. **Klik a odpočet:** generuje je metronom přesně na vzorek uvnitř bloku a jdou **jen na sběrnici KLIK**.
8. **MIDI bicí:** údery z modulu se podle časové značky převedou na přesný vzorek v bloku. Živě je zahraje vestavěná sada na sběrnici ODPOSLECH, při nahrávání jdou do fronty nahrávání a nahrané údery se při přehrávání zahrají na sběrnici MIX.
9. **Výstup do sluchátek** = MIX + ODPOSLECH + KLIK. Měřič **Celkem** měří jen MIX, tedy přesně to, co půjde do exportu.

**Oprava taktu a aparát:** při opravě je jedna instance aparátu stopy. Mimo opravovaný úsek dostává přehrávaný klip, uvnitř úseku živý vstup, a přepíná se na hraně úseku s prolnutím. Kytarista proto slyší plynulý zvuk a dozvuk zkreslení se neutne. [Předpoklad: [Z] zkouška H-04]

### 6.2 Rozpočet zpoždění (orientační, při 48 kHz)

| Nastavení | Vstup + výstup bufferu | Převodníky a USB (typicky) | Celkem tam a zpět | Hodnocení v Rockeru 2 |
|---|---|---|---|---|
| ASIO, buffer 64 | 2,7 ms | 2–4 ms | **asi 5–7 ms** | dobré |
| ASIO, buffer 128 | 5,3 ms | 2–4 ms | **asi 8–10 ms** | dobré |
| WASAPI exkluzivní, nejmenší perioda zařízení (často 3–10 ms) | 6–20 ms | 2–4 ms | **asi 8–25 ms** | dobré až ruší, podle zařízení [Z] |
| WASAPI sdílený nízkolatenční (je-li podporován) | 5–10 ms | 2–4 ms | **asi 8–15 ms** | použitelné [Z] |
| WASAPI sdílený běžný | 20–40 ms | | **30 ms a víc** | ruší, jen pro poslech |

Hranice hodnocení v rozhraní: **dobré ≤ 10 ms, použitelné 10–15 ms, ruší > 15 ms**. Hodnoty jsou orientační a definitivně je nastaví zkouška H-01 s kamarádem.

**Aparát nesmí přidat znatelné zpoždění.** Převzorkování zkreslení se dělá polyfázovými IIR filtry s minimální latencí (několik vzorků). Vlastní latenci aparátu jádro zná a **při přehrávání ji vyrovnává** (kytarové stopy se čtou o tuto latenci dřív), aby kytara s aparátem nezaostávala za bicími.

---

## 7. Kudy prochází nahraný zvuk

```
zvukovka ─► zvukové vlákno ─► fronta bez zámku ─► diskové vlákno ─► WAV (flush 1×/s)
                │                                          │
                └─ poloha začátku na časové ose            └─ deník Zachrana/<id>.json
                                                                  │
                     po zastavení: příkaz „PřidatPokus/Klip“ ◄────┘ (pak se deník smaže)
```

1. **Stisk R:** jádro určí přesný vzorek začátku na časové ose. Diskové vlákno založí soubor `Zvuk/<stopa>-p<pokus>-<datum-čas>.wav` (32bit float, kanály podle stopy) a **ještě před prvním vzorkem** zapíše deník `Zachrana/<id>.json` s údaji `trackId`, cílový díl nebo pokus, poloha, frekvence, kanály a použitá kompenzace.
2. **Během nahrávání:** `ThreadedWriter` zapisuje a každou sekundu (`setFlushInterval(sampleRate)`) volá `flush()`, který přepíše hlavičku WAV. Soubor je tak kdykoli čitelný nejméně do poslední sekundy. [Ověřeno F6]
3. **Přetečení fronty** (disk se zasekne déle než asi 10 s): nastane událost „výpadek zápisu v taktu X“. Na časové ose se objeví značka a rozhraní to ohlásí. Nic se neztratí potichu.
4. **Kompenzace zpoždění:** vzorek `k` nahrávky odpovídá poloze `začátek + k − L` na časové ose, kde
   `L = vstupníLatence + výstupníLatence (údaj ovladače) + kalibračníKorekce (změřená kabelem)`.
   Soubor zůstává celý a klip dostane `sourceStart = L`. Kalibrace se ukládá pro každou kombinaci (zařízení, frekvence, buffer) v nastavení počítače.
5. **Oprava taktu:** nahrává se celý průchod (předstih až dohra) a klip pokusu zpřístupní jen opravovaný úsek s prolnutím. Okraje jde později roztáhnout.
6. **Stop:** zapisovač dokončí soubor, model přidá zvukový soubor a klip nebo pokus (krok jde vrátit), skladba se uloží a teprve potom se deník smaže.
7. **MIDI bicí:** údery jdou stejnou frontou. Diskové vlákno je každou sekundu připisuje do textového deníku v `Zachrana/`, takže po pádu se ztratí nejvýš poslední sekunda. Po zastavení vznikne soubor `.mid`. Kompenzace je `výstupníLatence + korekceMIDI`: hráč reaguje na zpožděný poslech, ale údery MIDI nemají vstupní zvukovou latenci. `korekceMIDI` se změří zkouškou M-14, pokud modul posílá zvuk i MIDI zároveň, jinak je 0.
8. **Nápad a Pojistka:** stejná cesta, jen cílem je knihovna Nápady. *Ulož, co jsem hrál* zkopíruje z kruhové paměti poslední 2 minuty na diskovém vlákně. Kruhová paměť má rezervu, aby se čtená část nepřepisovala.

---

## 8. Přehrávání a export

### 8.1 Plán přehrávání

`PlanPrehravani` je neměnný, předem připravený seznam úseků pro každou stopu:

- otevřený čtečka souboru, `sourceStart`, začátek na časové ose, délka, zesílení a prolnutí,
- k tomu kopie nastavení mixu a aparátu.

Vzniká na vlákně rozhraní **z výskytů → dílů → klipů → aktivních pokusů** a předává se zvukovému vláknu výměnou ukazatele. Starý plán se uvolní zpátky na vlákně rozhraní.

Pohyb jezdce nevytváří nový plán. Hodnota jde přes atomickou proměnnou a v jádru se vyhlazuje po vzorcích.

### 8.2 Čtení souborů

32bitové float WAV se čtou přes `MemoryMappedAudioFormatReader` bez dekódování. Pracovní vlákno volá `touchSample()` na oblast před ukazatelem přehrávání, aby zvukové vlákno nečekalo na disk. Pro 16 stop × 10 minut je to asi 1,8 GB souborů, které spravuje operační systém. [Z] zkouška M-05 na kamarádově notebooku.

### 8.3 Export

- `Renderer` vytvoří **oddělenou instanci téhož jádra** a volá **stejnou funkci** pro zpracování bloku. Vstupy jsou ticho, sběrnice KLIK i ODPOSLECH jsou vypnuté a zpracovává se od začátku do konce plus 2 s dozvuku, rychleji než v reálném čase.
- **Shoda s poslechem** je zajištěná konstrukcí:
  - stejný kód,
  - deterministický dozvuk (pevné semínko),
  - vyhlazování parametrů nezávislé na velikosti bloku,
  - IR reproboxu načtené před začátkem renderu.

  Ověřuje se testem A-05: render s blokem 64, 128, 512 a 1024 se nesmí lišit o víc než −120 dBFS.
- **Klik nikdy není v exportu**, protože sběrnice KLIK do renderu vůbec nevstupuje. Test A-06 to hlídá.
- **Formáty:**
  - WAV 24 bit / 48 kHz,
  - WAV 16 bit / 44,1 kHz s ditherem TPDF (pro starší přehrávače),
  - **MP3 320 kb/s přes Media Foundation** (render do dočasného WAV, převod na 44,1 kHz, kódování).

  Kde kodér MP3 chybí (macOS vývojová verze, Windows „N“), nabídka MP3 je šedá s vysvětlením a WAV jde vždy. [Z] A-12
- **Kontrola přebuzení:** export počítá špičku vzorku. Překročí-li 0 dBFS, ohlásí místo („takt 37“) a nabídne „Ubrat celkovou hlasitost o X dB a exportovat znovu“.

---

## 9. Časování a souhra stop

| Otázka | Řešení |
|---|---|
| **Co jsou hodiny?** | Počítadlo vzorků zvukového zpětného volání. Žádné nástěnné hodiny, žádné časovače. |
| **Proč klik nezačne ujíždět?** | Klik se generuje přímo v bloku na přesném vzorku z indexu doby (kap. 5.2). |
| **Proč nahrávka sedí na bicí?** | Kompenzace `L` (kap. 7, bod 4) a kalibrace kabelem. Ověřuje se zkouškou M-02. |
| **Proč nesedí jinak na konci dlouhé skladby?** | Vstup i výstup běží na **jednom zařízení se společnými hodinami**, takže se čas nerozjíždí. Pokud uživatel ve WASAPI zvolí dvě různá zařízení, Rocker 2 nahrávání přes stopy zablokuje s vysvětlením (lze vědomě obejít). ASIO pracuje vždy s jedním zařízením. |
| **Jiná frekvence zařízení?** | Skladba má pevnou frekvenci (výchozí 48 kHz). Rocker 2 zkusí přepnout zařízení na frekvenci skladby; když to nejde, ohlásí to. Převzorkování v reálném čase ve v1 není. Importy se převádějí předem kvalitním převodem (libsamplerate). |
| **Proč sedí MIDI údery?** | Časová značka každé zprávy MIDI se převede na vzorek a odečte se výstupní latence (kap. 7, bod 7). Ověřuje se testem A-22 a zkouškou M-14. |
| **Výpadky zvuku?** | `getXRunCount()` a měření doby zpětného volání. Během nahrávání se výpadek označí na časové ose a ukáže v Diagnostice. |

---

## 10. Co se ukládá a kdy

| Událost | Co se zapíše | Kam | Kdy |
|---|---|---|---|
| **Nahrávání běží** | zvuk (vzorky) | `Zvuk/*.wav` | průběžně, hlavička se obnovuje 1×/s |
| | deník nahrávání | `Zachrana/<id>.json` | před prvním vzorkem |
| **Nahrávání skončí** | nový zvukový soubor a klip nebo pokus | `skladba.json` | hned, potom se smaže deník |
| **Střih, přesun, zkrácení, díly, volba pokusu** | nový snímek skladby | `skladba.json` | 1–2 s po poslední úpravě (atomicky) |
| **Změna aparátu nebo mixu** | parametry stopy | `skladba.json` | po puštění jezdce + 1–2 s, ne při každém pixelu tažení |
| **Změna pohledu** (přiblížení, ukazatel, režim, výběr) | stav pohledu | `zobrazeni.json` | 2 s po změně a při zavření. Nevytváří krok Zpět ani verzi. |
| **Každých 5 minut práce** | kopie `skladba.json` | `Verze/skladba-<datum-čas>.json` | posledních 50 verzí, starší se mažou |
| **Zavření skladby nebo programu** | dokončí zápisy a uloží verzi | vše výše | před zavřením okna |
| **Pád programu** | nic nového. Při startu se najdou deníky v `Zachrana/` a nabídne se obnovení. | — | při dalším startu |
| **Export** | soubor mixu | `Exporty/` (nebo vybrané místo) | na pokyn |
| **Zabalit skladbu** | `skladba.json`, `zobrazeni.json`, `Zvuk/` a `Nahledy/` (volitelně) jako zip `.rocker2` | vybrané místo | na pokyn |

**Atomický zápis:** zápis jde do dočasného souboru, potom `TemporaryFile::overwriteTargetFileWithTemporary()` (na Windows `ReplaceFile`). Rozepsaný soubor tak nikdy nepřepíše dobrý. [Ověřeno F8, [Z] A-09]

---

## 11. Složka skladby na disku

```
C:\Users\<jméno>\Rocker 2\            ← výchozí umístění mimo OneDrive (lze změnit)
├─ Skladby\
│  └─ Noční riff\
│     ├─ skladba.json        popis skladby
│     ├─ zobrazeni.json      kde jsem skončil
│     ├─ Zvuk\               nahrávky a importy, 32bit float WAV; nikdy se nepřepisují
│     ├─ Nahledy\            průběhy pro kreslení (lze smazat, vzniknou znovu)
│     ├─ Verze\              posledních 50 stavů skladby
│     ├─ Zachrana\           deníky právě běžících nahrávání
│     ├─ Kos\                zahozené pokusy (do „Uklidit skladbu“)
│     └─ Exporty\            hotové mixy
└─ Napady\
   ├─ napady.json            seznam, ★, poznámky, tempo, ladění
   └─ Zvuk\
%APPDATA%\Rocker 2\nastaveni.json       nastavení tohoto počítače
%LOCALAPPDATA%\Rocker 2\logs\           záznamy pro Diagnostiku
```

- Vnitřní názvy složek jsou bez diakritiky (kvůli zip balíkům a nástrojům). Název skladby diakritiku mít smí („Noční riff“). [Z] A-10
- **Spotřeba místa:** 32bit float, 48 kHz, mono = 11,5 MB za minutu. Sada 8 mikrofonů zabere 10 minut × 8 × 11,5 ≈ 0,9 GB.

---

## 12. Odolnost

| Situace | Chování |
|---|---|
| **Pád programu** | Zvuk do poslední sekundy je na disku (F6). Při startu se najdou deníky a nabídne se *Vložit zpět do skladby* nebo *Uložit jako nápad*. Pokud hlavička nesedí (pád přímo při zápisu hlavičky), délka se dopočítá z velikosti souboru. Obslužná rutina pádu (`SystemStats::setApplicationCrashHandler`) zapíše log. |
| **Výpadek proudu** | Jako pád. Windows ale může mít poslední sekundy v mezipaměti disku. Diskové vlákno proto po každém flush volá i `FlushFileBuffers` (nejvýš 1×/s). [Z] M-07 |
| **Dochází místo** | Diskové vlákno hlídá volné místo každé 2 s. Pod 30 minut nahrávání varuje, pod 500 MB nahrávání řízeně ukončí. |
| **Zvukovka se odpojí** | `AudioDeviceManager` ohlásí změnu. Transport se zastaví, zapisovač dokončí soubor a stav se ohlásí. Po návratu zařízení stejného názvu se samo znovu otevře. |
| **Spánek** | Během nahrávání a přehrávání `SetThreadExecutionState(ES_CONTINUOUS \| ES_SYSTEM_REQUIRED)`, v Pohledu zdálky navíc `ES_DISPLAY_REQUIRED`. Po zastavení se stav zruší. |
| **OneDrive** | Pokud cesta ke složce leží pod `%OneDrive%`, `%OneDriveConsumer%` nebo `%OneDriveCommercial%`, Rocker 2 doporučí jiné umístění. |
| **Antivirus** | Kontrola nových WAV antivirem může zpomalit zápis. Fronta pokryje asi 10 s. [Z] M-06 |
| **Poškozený soubor skladby** | Tolerantní načtení (kap. 5.2). Když selže, nabídne se poslední dobrá verze. |
| **Bluetooth výstup** | Rozpozná se podle názvu nebo typu zařízení a zobrazí se varování (dokument 01, 4.1). |

---

## 13. Vývoj z Macu, sestavení a balení

### 13.1 Repozitář

Nový repozitář `rocker2`, oddělený od starého Rockeru. Doporučuji veřejný repozitář s licencí AGPLv3 (kap. 14).

```
rocker2/
├─ CMakeLists.txt            JUCE 9.0.5 přes FetchContent (pevně na tag), Catch2 v3
├─ src/ app ui model engine dsp storage platform
├─ assets/ fonts (Inter, JetBrains Mono + licence OFL) · ir (reproboxy) · icons
├─ tests/
├─ docs/ 01–05 (tyto dokumenty) · STAV.md · PREDANI.md · ROZHODNUTI.md
├─ installer/ rocker2.iss (Inno Setup)
└─ .github/workflows/ ci.yml
```

### 13.2 Sestavení

- **macOS (vývoj):** CMake + Xcode nebo Ninja, CoreAudio. Slouží k vývoji rozhraní, logiky a testů. Jenda si ho může pustit se svou zvukovkou. Není to produkt pro kamaráda.
- **Windows (produkt):** GitHub Actions `windows-2025`, generátor Visual Studio, x64, statická běhová knihovna (`MSVC_RUNTIME_LIBRARY MultiThreaded`), aby nebylo potřeba instalovat VC++ redistributable. Dále `JUCE_ASIO=1`, testy a výstupy **ZIP a instalátor**, které se uloží jako artefakt a při značce verze jako GitHub Release. Sestavuje se **pokaždé x64 i ARM64** (MSVC umí ARM64 sestavit i na x64 runneru) a instalátor vybere správnou verzi podle procesoru. [Z] E0
- **Testy bez zvukovky:** CI nemá zvukovou kartu, proto jádro umí běžet s **falešným zařízením**, které posílá bloky s umělým vstupem (sinus, impulsy, ticho) a zaznamenává výstup.
- **Snímky obrazovek rozhraní:** test vykreslí hlavní obrazovky (`Component::createComponentSnapshot`) při 1280×720 a 1920×1080 se zvětšením 150 % a uloží PNG jako artefakt CI. **Jenda tak kontroluje vzhled z Macu bez Windows.**
- **Windows ve virtuálním stroji na Macu** (Windows na ARM): stačí ke kontrole instalátoru a rozhraní, ne k měření zpoždění.

### 13.3 Předání kamarádovi

1. Stáhne si instalátor `Rocker2-Setup-<verze>.exe` z GitHub Releases (nebo mu ho Jenda pošle).
2. SmartScreen u nepodepsané aplikace ukáže „Systém Windows ochránil váš počítač“. Klikne na **Další informace → Přesto spustit**. Podepsání je mimo v1: Azure Artifact Signing pro jednotlivce je jen pro USA a Kanadu (F20) a OV certifikát stojí peníze.
3. Při prvním spuštění projde Zapojení a pak **Nápověda → Diagnostika → Kontrola sestavy**. Výsledek (co funguje a co ne) pošle Jendovi.
4. Při potížích použije **Nápověda → Diagnostika → Uložit zprávu**. Vznikne zip s logy a nastavením (bez zvuku), který pošle Jendovi.

---

## 14. Licence

**Doporučená varianta: otevřený kód pod AGPLv3.**

- JUCE se použije pod AGPLv3. ASIO SDK pod GPLv3. Licence GNU AGPLv3 (§13) výslovně dovoluje kombinaci s dílem pod GPLv3.
- Repozitář je veřejný, což navíc znamená bezplatné Windows runnery v CI (F19).
- Fonty Inter a JetBrains Mono (OFL 1.1) se smí vložit do aplikace; licence se přiloží.
- Další knihovny: libsamplerate (BSD-2), Catch2 (BSL-1.0).
- Kodér MP3 je součást Windows.
- Pozdější vzorky bicích musí být vlastní nebo CC0.

**Alternativa s uzavřeným kódem:** JUCE Starter (zdarma do limitu ročního příjmu, F2) a **proprietární licence ASIO** (smlouva se Steinbergem), protože GPLv3 varianta ASIO uzavřený kód nepřipouští.

> Nejde o právní radu. Před případným prodejem nebo veřejným šířením je dobré podmínky znovu přečíst.

---

## 15. Rizika a jak je brzy odhalíme

| Riziko | Dopad | Opatření | Kdy ověříme |
|---|---|---|---|
| Zvukovka nemá ASIO nebo má špatný ovladač | Zpoždění aparátu ruší | WASAPI exkluzivní a nízkolatenční sdílený. Volba „Přímý odposlech zvukovky + aparát jen při přehrávání“ (kytara se nahraje čistě, zvuk se nastaví potom). | E1, H-01 |
| Souběhové chyby ve vlastním jádře | Lupance, pád | Pravidla 4.3, ThreadSanitizer na Macu, testy jádra s falešným zařízením | E2 |
| Ovladač hlásí latenci nepřesně | Kytara se posune proti bicím | Kalibrace kabelem a zkouška M-02 | E2 |
| Zvuk aparátu nepřesvědčí | Kamarád nebude chtít nahrávat přes Rocker 2 | Ladění zvuku s kytarou (H-03), lepší IR reproboxů, NAM jako rozšíření | E3 |
| Slabý notebook | Výpadky zvuku | Měření M-05, zmrazení zvuku (v1.1), větší buffer při mixu | E3, E7 |
| Chybí kodér MP3 | Nejde poslat MP3 | WAV vždy, vysvětlující hláška | E7, A-12 |
| Rozhraní se při AI vývoji rozpadne do nesourodých stylů | Matoucí ovládání | Designové tokeny z dokumentu 01 v jednom souboru a snímky obrazovek v každé etapě | každá etapa |
| Antivirus nebo OneDrive zdrží zápis | Výpadek nahrávky | Fronta 10 s, varování u OneDrive, zkouška M-06 | E2, E11 |
| Notebook s ARM | Jiné sestavení, chybí ASIO | Sestavení ARM64 pokaždé, instalátor vybere verzi sám, bez ASIO režim Windows | E0 |
| Vybavení, které nemáme k vyzkoušení | Neodhalená chyba u cizí zvukovky nebo modulu | Simulace sestav (A-24) a Kontrola sestavy u kamaráda (M-15) | E0, E11 |
| Licence vzorků vestavěné sady bicích | Nelze šířit | Jen vzorky s licencí CC0, nebo vlastní nahrávka (kamarádovy akustické bicí) | E10 |
| Modul bicích s neobvyklými čísly not MIDI | Pady hrají špatný zvuk | Výchozí rozložení podle General MIDI a funkce Naučit pady | E10 |

---

## 16. Zdroje

| Kód | Zdroj |
|---|---|
| S1 | JUCE Releases (9.0.5): https://github.com/juce-framework/JUCE/releases |
| S2 | JUCE `CHANGE_LIST.md` (9.0.1 Windows 1607, 9.0.2 MP3 čtení, 9.0.3 Opus, 8.0.11 ASIO SDK GPLv3): https://github.com/juce-framework/JUCE/blob/master/CHANGE_LIST.md |
| S3 | JUCE `LICENSE.md` (AGPLv3 / JUCE licence): https://github.com/juce-framework/JUCE/blob/master/LICENSE.md |
| S4 | JUCE fórum, „JUCE 9: No pricing or EULA changes“: https://forum.juce.com/t/juce-9-no-pricing-or-eula-changes/69000 |
| S5 | JUCE fórum, limity příjmů tarifů: https://forum.juce.com/t/revenue-limits-for-juce-tiers/61058 a https://forum.juce.com/t/do-i-need-the-starter-or-indie-licence/69021 (oficiálně: https://juce.com/get-juce/) |
| S6 | Licence ASIO SDK v JUCE: https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_devices/native/asio/LICENSE.txt |
| S7 | Steinberg, tisková zpráva 15. 10. 2025 (ASIO pod GPLv3): https://ocl-steinberg-live.steinberg.net/_storage/asset/808575/storage/master/Press%20Release%20-%202025-10-15%20-%20OBS%20Partnership-%20EN.pdf |
| S8 | JUCE `juce_AudioIODeviceType.h` a `juce_WASAPI_windows.cpp`: https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_devices/native/juce_WASAPI_windows.cpp |
| S9 | JUCE issue č. 560 (WASAPI low latency): https://github.com/juce-framework/JUCE/issues/560 · Microsoft: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient3-initializesharedaudiostream |
| S10 | JUCE `juce_AudioFormatWriter.h` (flush, setFlushInterval): https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_formats/format/juce_AudioFormatWriter.h |
| S11 | JUCE `juce_WavAudioFormat.cpp` (flush → writeHeader, RF64): https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_formats/codecs/juce_WavAudioFormat.cpp |
| S12 | Strom repozitáře JUCE: `juce_MemoryMappedAudioFormatReader.h`, `juce_TemporaryFile.h`, `juce_Files_windows.cpp`, `juce_AudioIODevice.h`, `juce_Typeface.h`, `juce_dsp/…`, `examples/Audio/AudioLatencyDemo.h`: https://github.com/juce-framework/JUCE |
| S13 | JUCE `juce_MP3AudioFormat.h` („doesn't currently provide an encoder“): https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_formats/codecs/juce_MP3AudioFormat.h |
| S14 | Microsoft, MP3 Audio Encoder: https://learn.microsoft.com/en-us/windows/win32/medfound/mp3-audio-encoder |
| S15 | Tracktion Engine: https://github.com/Tracktion/tracktion_engine/ |
| S16 | cpal: https://github.com/RustAudio/cpal |
| S17 | NeuralAmpModelerCore: https://github.com/sdatkinson/NeuralAmpModelerCore |
| S18 | Inter: https://github.com/rsms/inter |
| S19 | JetBrains Mono: https://github.com/JetBrains/JetBrainsMono |
| S20 | libsamplerate: https://github.com/libsndfile/libsamplerate |
| S21 | GitHub-hosted runners: https://docs.github.com/en/actions/reference/runners/github-hosted-runners |
| S22 | Microsoft, možnosti podepisování kódu: https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/code-signing-options · Artifact Signing quickstart: https://learn.microsoft.com/azure/artifact-signing/quickstart |
| S23 | Microsoft, Known Folder Move: https://learn.microsoft.com/sharepoint/redirect-known-folders |
| S24 | Microsoft, SetThreadExecutionState: https://learn.microsoft.com/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate |

[r1-latency]: https://github.com/JendaNDT/rocker/blob/77df8e37e318e01d0d135db3a8640cf046d64fa5/ARCHITECTURE.md
