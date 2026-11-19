# Rocker 2 — plán oprav a vylepšení po v1.0.0

**Dokument 06.** Podklad pro implementující model (Codex). Vznikl z revize kódu verze 1.0.0 (commit `4237242`). Navazuje na dokumenty 01–05 a na pravidla pro etapy v [04](04_ETAPY_VYVOJE.md#pravidla-pro-všechny-etapy).

Čísla řádků platí pro commit `4237242`. Před úpravou místo vždy najdi znovu podle názvu funkce.

## Jak s plánem pracovat

1. Pracuj po etapách v uvedeném pořadí: **E12 → E13 → E14 a další**. Každá etapa má vlastní větev (`etapa-12` …), malé commity a pull request s českým popisem.
2. Na začátku každé etapy napiš nejdřív **zkoušku, která chybu zachytí** (musí selhat), a teprve potom oprav kód. Ve výsledku PR uveď, že zkouška před opravou selhala a po opravě prošla.
3. Platí všechny závazné podmínky z dokumentu 05: zvukové zpětné volání bez zámků, alokací, souborů a logování. Barvy, písma a rozměry jen přes tokeny v `src/ui/Tokens.h`. České texty jen v `src/ui/Strings.h`. Kód, názvy a komentáře anglicky.
4. Etapa je hotová, až projde CI na Windows x64, Windows ARM64 i Macu, včetně snímků obrazovky (A-17).
5. Aktualizuj `docs/STAV.md`, `docs/PREDANI.md`, `docs/ROZHODNUTI.md` a README. Každou volbu, která není v tomto plánu jednoznačně daná, zapiš do ROZHODNUTI.md s důvodem a alternativou a řekni o ní Jendovi.
6. Nové automatické zkoušky číslujeme od **A-25** dál.

## Přehled

| Etapa | Obsah | Velikost |
|---|---|---|
| **E12** | Opravy chyb O-1 až O-5 | S–M |
| **E13** | Rychlá vylepšení V-1 až V-5 | M |
| **E14** | Export stop zvlášť | S–M |
| **E15** | Zvuk stopy Jiná (zpěv, baskytara) | M |
| **E16** | Vodicí rytmus místo kliku | M |
| **E17** | Hlasitější export (omezovač, LUFS) | M |
| **E18** | Nahrávání dokola | L |

Na Jendův pokyn z 19. 11. 2026 pokračujeme v pořadí E12 až E18. Poslech, hraní celé písničky (H-09) a další ruční zkoušky přijdou po dokončení E18. Každou etapu uzavírají její automatické zkoušky. [Rozhodnutí](ROZHODNUTI.md#e12--složka-nápadů-historie-a-připojení)

---

## E12 — opravy chyb

### O-1 · Nápady nefungují po změně Složky skladeb

**Příznak:** Po změně **Složky skladeb** (Diagnostika) se nahraný riff neobjeví v knihovně Nápady a zobrazí se „Nastavení se nepodařilo uložit“. Zvuk se zapíše, ale ladění, tempo a první doba se ztratí. Náhled nápadu se nepřehraje („Náhled se nepodařilo přehrát.“). Ověřeno zkouškou: ve výchozí složce riff v knihovně je, s vlastní složkou chybí.

**Příčina:**
- `src/ui/Studio.cpp:224–227`: objekt `ideas` vzniká se složkou `libraryRoot.getSiblingFile("Napady")` ještě **před** přepsáním `libraryRoot` hodnotou `settings.libraryPath`.
- `src/ui/IdeaPage.cpp:106` (`stopIdea`) a `:147` (`playIdea`) počítají složku znovu jako `libraryRoot.getSiblingFile("Napady")`. Po změně složky skladeb ukazují jinam než `ideas`.
- `src/ui/FilePage.cpp:84` (`chooseLibrary`) mění `libraryRoot` za běhu, takže chyba nastane i bez restartu.

**Rozhodnutí:** Nápady zůstávají v pevné složce `Rocker 2/Napady` vedle nastavení. Složka skladeb je neovlivní a stávající nápady se nemusí přesouvat. Zapiš to do ROZHODNUTI.md.

**Postup:**
1. Do `Ideas` (`src/storage/Ideas.h`) přidej `const juce::File& root() const { return folder; }`.
2. V `stopIdea` a `playIdea` nahraď `libraryRoot.getSiblingFile("Napady")` výrazem `ideas->root()`.
3. Ověř hledáním v kódu, že `getSiblingFile("Napady")` zbývá jen v konstruktoru `Studio`.

**Zkouška A-25** (v `tests/StudioTests.cpp`, podle vzoru „A-17 E8 an idea becomes…“):
- `Settings.libraryPath` míří do vlastní složky, například `temp/Hudba/Moje skladby`.
- Nahraj riff klávesami **R** a **mezerník**. Tlačítko `cs::ideaStar` se musí objevit.
- `Ideas(temp/Napady).all().size() == 1` a uložené ladění odpovídá vybranému.
- Mezerník spustí náhled: objeví se tlačítko `cs::stopSong`.
- Vytvoř z nápadu skladbu. Musí vzniknout ve **vlastní** složce skladeb.

### O-2 · Useknutá hodnota hlasitosti v režimu Míchej

**Příznak:** Jezdce hlasitosti ukazují „−3,0 d“ a „0,0 dB“. Ve skutečnosti jde o useknutý text „−3,0 dBFS“. Vidět je to na snímku `18-mix` v obou velikostech.

**Příčina:**
- `Fader::paint` (`src/ui/LivePages.cpp:29–36`) kreslí hodnotu do pevné šířky `s::faderReadout = 54` bez kontroly délky.
- `mixSlider` (`LivePages.cpp:388`) přidává jednotku `cs::dbUnit = " dBFS"`. U zesílení stopy je **dBFS špatně**. Jde o relativní změnu, správně je „dB“.
- `Studio::layoutErrors()` (`Studio.cpp:1106`) jezdce `Fader` nekontroluje, proto A-17 chybu nezachytila.

**Postup:**
1. Do `Strings.h` přidej `gainUnit = " dB"` a použij ho v `mixSlider`. `dbUnit` ponech pro měřiče, kde dBFS správně je.
2. Do `Fader` přidej `int readoutWidth() const`. Změří nejširší možný text: pro zesílení „−95,9 dB“ a `cs::silentGain`, pro polohu „L 100“ a `cs::centre`, pro ostatní jezdce mez rozsahu s jednotkou. Šířku použij místo `s::faderReadout` a odečti ji od pruhu.
3. Přidej `bool Fader::fits() const`, který porovná nejširší text s vyhrazenou šířkou. V `layoutErrors()` přidej větev pro `Fader`, stejně jako u `ComboBox`.
4. Zkontroluj, že se řádek stopy v Míchej vejde při 1280 × 720. Případně zúž pruh, ne text.

**Zkouška:** Rozšiř A-17 v režimu Míchej. Nastav jezdce na −3, −12,5, −95,9 a polohu L 100 a pokaždé zavolej `fits(view)`. Bez opravy musí zkouška selhat. Snímek `18-mix` musí ukazovat „−3,0 dB“.

### O-3 · Jedna hláška pro 23 různých chyb

**Příznak:** `cs::writeError` („Nastavení se nepodařilo uložit. Zkontroluj volné místo.“) se zobrazuje i při chybě nahrávky, skladby, nápadu nebo importu.

**Postup:** Do `Strings.h` přidej samostatné hlášky a nahraď použití podle tabulky. Formulace ponech krátké a ověř, že se vejdou do stavového řádku (A-17).

| Nová hláška (návrh textu) | Místa použití |
|---|---|
| `settingsError` (stávající text) | `Studio.cpp:273` (`persist`), `LivePages.cpp:520` (předvolba) |
| `songSaveError` „Skladbu se nepodařilo uložit. Zkontroluj volné místo a přístup ke složce.“ | `ProjectPage.cpp:70, 80, 369, 406, 541`, `FilePage.cpp:56`, `EditPage.cpp:205` |
| `takeSaveError` „Nahrávku se nepodařilo dokončit. Při dalším otevření skladby ji Rocker zkusí obnovit.“ | `ProjectPage.cpp:345, 431` |
| `armError` „Nahrávání se nepodařilo spustit. Zkontroluj volné místo a složku skladby.“ | `ProjectPage.cpp:289` (mimo `diskFull`) |
| `ideaSaveError` „Nápad se nepodařilo uložit.“ | `IdeaPage.cpp:71, 222, 255, 404, 419` |
| `importError` „Podklad se nepodařilo přidat do skladby.“ | `IdeaPage.cpp:513, 526` |
| `cleanupError` „Úklid se nepodařilo dokončit. Soubory zůstaly na disku.“ | `FilePage.cpp:33` |
| `diagnosticError` „Zkoušku se nepodařilo připravit.“ | `ProjectPage.cpp:1140, 1178` |

Před použitím ověř, že text hlášky odpovídá skutečnosti. Například `takeSaveError` smí slibovat obnovu jen tehdy, když deník ve složce `Zachrana` opravdu zůstal. Jinak text uprav.

**Zkouška:** V O-1 zkontroluj, že se při chybě nápadu neobjeví text nastavení. Pokud stavový text není dostupný z testu, přidej `Studio::statusText()` jen pro čtení. Hledáním ověř, že `cs::writeError` zůstal jen u nastavení, případně ho přejmenuj na `settingsError`.

### O-4 · Uložené verze vytlačují stejné kopie

**Příznak:** `projectTick` (`ProjectPage.cpp:404–408`) volá `song.archive` každých 5 minut i bez změny. `Song::archive` (`src/song/Song.cpp`, konec souboru) drží 50 verzí. Po zhruba 4 hodinách nečinnosti zůstanou jen stejné kopie a starší užitečné verze zmizí. Totéž platí pro opakované otevírání a zavírání skladby (`finishProject`).

**Postup:**
1. V `Song::archive` najdi nejnovější soubor ve `Verze` (stejné řazení jako v `latestVersion`). Pokud se jeho obsah shoduje s `JSON::toString(json())`, nic nezapisuj a vrať `true`.
2. Pořadí vlastností v `DynamicObject` je dané pořadím vkládání, takže porovnání textu je stabilní. Pro jistotu porovnávej znovu načtený `Song`, tedy `Song::parse(...) == *this`, ne bajty.
3. `restoreVersion` (`EditPage.cpp:192`) dál ukládá současný stav před obnovením. Díky shodě se nezdvojí.

**Zkouška A-26** (v `tests/SongTests.cpp`):
- Dvakrát `archive` beze změny vytvoří 1 soubor.
- Po změně tempa vzniknou 2 soubory.
- 60 různých změn ponechá 50 nejnovějších.
- Stará verze po `trashAudio` (upravený katalog `zvuky`) se při další archivaci s jiným obsahem nesmaže omylem.

### O-5 · Frekvence skladby a zvukovky se liší

**Příznak:** `startProject` (`ProjectPage.cpp:236`) ukáže „V Zapojení nastav frekvenci skladby.“ a neřekne kterou. Po přenosu balíku na jiný počítač hráč neví, co nastavit.

**Postup:**
1. Z `cs::rateMismatch` udělej funkci `rateMismatch(int song, int device)` s textem „Skladba je ve 48 kHz, zvukovka běží na 44,1 kHz.“ Použij existující `cs::frequency`.
2. Na stránce skladby, pokud `link.online && link.current.rate != song.grid.rate`:
   - když `link.caps.rates` frekvenci skladby obsahuje, ukaž tlačítko **„Přepnout zvukovku na 48 kHz“**. To zavolá `choose()` s `link.current` a novou frekvencí, uloží nastavení a smaže kalibraci jen u nového klíče `calibrationKey`.
   - jinak ukaž „Tvoje zvukovka 48 kHz nenabízí.“ Převod celé skladby na jinou frekvenci patří do pozdější etapy. Zapiš to do ROZHODNUTI.md.
3. Hlášku ukaž hned při `openProject`, ne až po stisku Přehrát.

**Zkouška:** Rozšiř `tests/FakeDevice.h` o seznam podporovaných frekvencí (dnes `caps.rates = {rate}`). Do testu A-24 přidej scénář: skladba 48 kHz, zvukovka 44,1 kHz s nabídkou 44,1 a 48. Tlačítko je vidět, po kliknutí je `link.current.rate == 48000` a mezerník spustí přehrávání.

**Podmínka dokončení E12:** A-25, A-26 a rozšířené A-17 a A-24 procházejí. Snímky `18-mix` a `21-napady` jsou v PR. Kontrolní seznam pro Jendu: změnit Složku skladeb, nahrát riff, přehrát ho a projít Míchej.

---

## E13 — rychlá vylepšení

### V-1 · Mazání nápadů

**Proč:** Knihovna Nápady jen roste. Dvouminutový nápad z Pojistky má asi 23 MB a smazat ho nejde.

**Postup:**
1. Do `Ideas` přidej `bool remove(const juce::String& id, const std::function<bool(const juce::File&)>& trash)`.
2. **Pořadí je důležité.** `Ideas::recover()` při spuštění znovu přidá každý WAV, který v katalogu chybí. Proto nejdřív přesuň WAV do systémového koše (`File::moveToTrash`). Až když se to povede, ulož katalog bez položky. Když se koš nepovede, položku ponech a vrať `false`.
3. V knihovně u vybraného nápadu přidej tlačítko **„Smazat nápad“** s potvrzením ve dvou krocích, stejně jako `confirmClean` u úklidu skladby: „Přesunout do koše? Ano / Ne“.
4. Tlačítko je neaktivní během nahrávání, ukládání a náhledu.

**Zkouška A-27:** Smazání odstraní položku i soubor (funkce `trash` je v testu podvržená). Nový `Ideas(...)` s `recover()` nápad znovu nepřidá. Selhání koše ponechá položku i soubor.

### V-2 · Načítání M4A/AAC i na Macu

**Proč:** Na Macu JUCE registruje `CoreAudioFormat` v `registerBasicFormats()`, který M4A a AAC přečte. Rocker to ale vypíná.

**Postup:**
1. V `supportsImport` (`src/engine/Import.cpp:7`) změň `#if JUCE_WINDOWS` na `#if JUCE_WINDOWS || JUCE_MAC`.
2. V `importAudio` volej `decodeSystemAudio` jen ve Windows (`#if JUCE_WINDOWS`). Na Macu přečte soubor rovnou `AudioFormatManager`.
3. Uprav README (sekce „Jak zachytit riff“, krok 5).

**Zkouška:** Přidej malý zkušební soubor `tests/fixtures/a19-tone.m4a`. Vytvoř ho ze stejného tónu jako `a19-tone.mp3`, na Macu příkazem `afconvert -f m4af -d aac`. Do A-19 přidej kontrolu délky a výšky tónu pro Windows i Mac. U AAC počítej s posunem kodéru (priming 2112 vzorků). Toleranci délky zdůvodni v ROZHODNUTI.md.

### V-3 · Export pro posluchače i na Macu

**Proč:** MP3 jde jen ve Windows přes Media Foundation (`src/engine/Mp3.cpp`). macOS kodér MP3 nemá, ale má AAC.

**Postup:**
1. Do `Delivery` (`src/engine/Export.h`) přidej `m4a`: „M4A (AAC) · 256 kb/s“, 44,1 kHz stereo.
2. Na Macu zakóduj PCM z `convert()` přes AudioToolbox (`ExtAudioFileCreateWithURL`, `kAudioFileM4AType`, `kAudioFormatMPEG4AAC`). V CMake přidej `-framework AudioToolbox` do větve `APPLE`.
3. Délku a priming zapiš tak, aby přehrávače přehrály přesnou délku. Ověř to dekódováním přes `CoreAudioFormat`, obdobně jako `tagMp3Length` u MP3.
4. Ve Windows lze AAC přidat přes Media Foundation (`MFAudioFormat_AAC`). Je to volitelné, rozhodnutí zapiš.
5. V nabídce exportu ukaž jen formáty dostupné na daném systému.

**Zkouška A-28** (obdoba A-12): zakóduj dvousekundový výběr, dekóduj ho a porovnej délku, začátek a konec. Na Macu se zkouška nesmí přeskočit.

### V-4 · Verze pro Macy s procesorem Intel

**Postup:**
1. V `.github/workflows/ci.yml` sestav balík pro Mac s `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` v samostatné složce sestavení (`build-universal`).
2. Kontrolu se sanitizéry (A-15, `ROCKER_SANITIZERS=ON`) ponech v dnešní složce jen pro arm64. Sanitizéry s univerzálním sestavením spolehlivě nefungují.
3. Za sestavení přidej kontrolu `lipo -archs "Rocker 2.app/Contents/MacOS/Rocker 2"`. Musí vypsat `x86_64 arm64`, jinak krok selže.
4. Volitelně spusť `StudioTests` z univerzálního sestavení přes Rosettu (`arch -x86_64`), pokud ji běžec GitHubu nabízí. Výsledek zapiš do STAV.md.
5. Uprav tabulku stažení v README: „Mac (čip Apple i Intel)“.

### V-5 · Srozumitelné tlačítko pro umístění podkladu

**Proč:** Na stránce skladby je vedle sebe „Na začátek“ (přesun ukazatele) a „Na začátek skladby“ (`ProjectPage.cpp:667`, přepínač místa pro **Přidat podklad**). Vypadají stejně, ale dělají něco jiného.

**Postup:**
1. Texty přepínače změň na „Podklad: od začátku“ a „Podklad: na ukazatel“ (`cs::atBeginning`, `cs::atCursor`).
2. Přepínač umísti hned vedle **Přidat podklad**, aby bylo vidět, že k sobě patří.
3. Uprav README a testy, které hledají tlačítka podle jména (E8).

**Podmínka dokončení E13:** A-27, A-28 a rozšířená A-19 procházejí na všech systémech. Balík pro Mac obsahuje obě architektury. Kontrolní seznam pro Jendu: smazat nápad, na Macu přidat M4A a vyexportovat M4A, spustit na Macu s Intelem (pokud je k dispozici).

---

## E14 — export stop zvlášť

**Cíl:** Jedním příkazem uložit každou stopu jako samostatný WAV, se stejnou délkou a začátkem, aby šly složit v jiném studiu.

**Návrh:**
- V exportu přidej volbu **„Stopy zvlášť“**. Vznikne složka `Exporty/<Skladba> – stopy <datum>` se souborem pro každou stopu, například `01 Bicí.wav` a `02 Kytara.wav`.
- Akustická sada bicích (stopy se stejným `group`) se uloží jako jedna stereo stopa.
- Každá stopa obsahuje svůj zvuk aparátu, hlasitost a polohu, ale **ne** celkovou hlasitost. Ztlumené stopy se přeskočí. Sólo se ignoruje a dialog na to upozorní.
- Všechny soubory začínají na 0 a končí stejně (konec skladby plus dozvuk), stejně jako u celé skladby.
- Implementace: `renderSong` dostane volitelnou masku stop, nebo použij kopii `Song` s ostatními stopami ztlumenými a `masterDb = 0`.

**Zkouška A-29:** Součet vykreslených stop v plovoucí čárce (před ditherem) se rovná vykreslení celé skladby s celkovou hlasitostí 0 dB. Tolerance je stejná jako v A-05. Všechny soubory mají stejnou délku.

---

## E15 — zvuk stopy Jiná (zpěv, baskytara)

**Cíl:** Stopa typu `jine` má dnes čistý zvuk bez úprav. Dostane jednoduchý řetězec efektů s předvolbami **Bez úprav**, **Zpěv** a **Baskytara**.

**Návrh:**
- Nový procesor `src/dsp/Voice.{h,cpp}`: třípásmový ekvalizér, kompresor a dozvuk. Dozvuk může sdílet implementaci místnosti z `Amp`. Parametry se balí do jednoho 64bitového slova jako `AmpParameters`, aby se daly předat do zvukového vlákna bez zámku (vzor `AmpCommand`).
- Nastavení se uloží do skladby jako nové pole stopy `zvuk`. Starší skladby bez pole se otevřou s **Bez úprav**. `schemaVersion` zůstává 1, pole je volitelné.
- Pevné zpoždění procesoru se kompenzuje stejně jako u aparátu (`delaySamples`).
- Spodní deska „Zvuk stopy“ se otevře i pro stopu Jiná, s vlastními jezdci.

**Zkouška A-30** (obdoba A-05 a A-20): bitově shodný výstup při blocích 64, 128, 512 a 1024. Změna předvolby za běhu a návrat. Uložení a znovuotevření. Původní WAV zůstane nezměněný.

---

## E16 — vodicí rytmus místo kliku

**Cíl:** Kytarista nahraje kytaru dřív než bicí a místo suchého kliku slyší jednoduchý rytmus (dokument 01, kap. 6, bod 2).

**Návrh:**
- Volba u kliku: **Klik / Rytmus**. Uloží se do `ViewState`.
- Rytmus: kopák na 1 a 3, virbl na 2 a 4, hi-hat v osminách. U jiného metra kopák na první době, virbl na poslední, hi-hat na každé době.
- Zvuk vzniká v `Performance::process` stejně jako klik, z jednoúderových vzorků `DrumPlayer` (sada 0). Vzorky se připraví mimo zvukové vlákno při vytvoření `Performance`.
- Rytmus jde **jen do sluchátek**. Nikdy do nahrávky ani exportu.

**Zkouška A-31** (obdoba A-06): export s klikem, s rytmem i bez obojího je bitově shodný. Každý úder rytmu začíná na přesném vzorku podle `BeatGrid` po dobu 10 minut.

---

## E17 — hlasitější export

**Cíl:** Volba exportu **„Hlasitější (pro telefon)“** srovná hlasitost na −14 LUFS a omezí špičky na −1 dBTP. V Míchej se ukáže změřená hlasitost.

**Návrh:**
- Měření hlasitosti podle ITU-R BS.1770-4 a EBU R128: K-filtr, bloky 400 ms, absolutní a relativní práh.
- Dvě fáze bez zvukového vlákna: změřit, nastavit zesílení, pak omezovač s předstihem (look-ahead) a odhadem skutečné špičky čtyřnásobným převzorkováním.
- Výchozí export zůstává beze změny. Hlasitější varianta je volitelná a v názvu souboru má „hlasitejsi“.

**Zkouška A-32:** Zkušební signály EBU Tech 3341 (alespoň případy 1–5) změří s tolerancí ±0,1 LU. Po omezovači skutečná špička nepřekročí −1 dBTP a výsledek je do 0,5 LU od cíle.

---

## E18 — nahrávání dokola

**Cíl:** Vybrané takty hrají dokola a každý průchod se uloží jako nový pokus ve stejném úseku, jako u opravy taktu klávesou P (dokument 01, kap. 6, bod 1). Nejlepší pokus se vybírá klávesou Tab.

**Návrh:**
- V režimu Stříhej je s vybranými takty k dispozici volba **„Nahrávat dokola“**, zatím bez klávesové zkratky. Zkratku navrhni a zapiš do ROZHODNUTI.md.
- Přehrávání se v `Performance` vrací na začátek výběru. Hodiny (`clock`) běží dál lineárně a na pozici ve skladbě se převádějí. Klik i odpočet se drží mřížky.
- Nahrává se do **jednoho souvislého WAV**. Po zastavení se rozdělí na průchody: každý průchod se přidá přes `Song::addAttempt` se stejným `from`/`to`, s posunem `offset = index × délka + kompenzace`.
- Deník v `Zachrana` dostane `smyckaOd`, `smyckaDo` a délku průchodu. `recoverTake` po pádu obnoví všechny celé průchody a nedokončený označí jako zahozený (`discarded`).
- Předstih a dohra fungují jako u opravy: na hranách průchodu se použije stejné prolínání jako v A-21.

**Zkouška A-33:** Tři průchody s číslovanými vzorky dají tři pokusy s přesnou polohou každého vzorku. Ukončení procesu ve třetím průchodu (obdoba A-08) obnoví dva celé pokusy a třetí jako zahozený. Volba pokusu za běhu (Tab) funguje jako v A-21.

**Ruční zkouška H-14 (po vydání):** Hráč nahraje riff dokola pětkrát a vybere nejlepší průchod. Zapíše, jestli přechod na začátek smyčky slyší a jestli ho ovládání nezdržuje.

---

## Prompt pro Codex

Zkopíruj do Codexu na začátku každé etapy a změň číslo etapy:

> Jsi hlavní vývojář aplikace Rocker 2 v tomto repozitáři. Přečti `docs/05_STARTOVACI_PROMPT.md` (závazné podmínky), `docs/04_ETAPY_VYVOJE.md` (pravidla etap), `docs/PREDANI.md` a celý `docs/06_PLAN_OPRAV_A_VYLEPSENI.md`. Proveď etapu **E12** podle dokumentu 06. Pracuj ve větvi `etapa-12` po malých commitech. U každé opravy nejdřív napiš zkoušku, která chybu zachytí, ověř, že selže, a teprve potom oprav kód. Odchylky od plánu zapiš do `docs/ROZHODNUTI.md`. Aktualizuj `docs/STAV.md`, `docs/PREDANI.md` a README. Otevři pull request s českým popisem, výsledky CI pro Windows x64, Windows ARM64 i Mac a snímky obrazovek. Jendovi napiš česky krátký kontrolní seznam, co má vyzkoušet. Co nejde ověřit automaticky, označ „čeká na ruční zkoušku“.
