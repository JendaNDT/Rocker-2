# Předání E8 · od riffu ke skladbě

## 1. Hotovo

Na úvodu stiskni **R**, zahraj riff a ulož jej mezerníkem. **U** uloží poslední dvě minuty právě hrané kytary. V knihovně označ nápad hvězdou, přidej poznámku, doťukej osm dob a vytvoř z něj skladbu. V režimu Hraj můžeš přetáhnout zvukový podklad. [Běhy a balíky pro E8](https://github.com/JendaNDT/Rocker-2/actions/workflows/ci.yml?query=branch%3Aetapa-8) · [Knihovna nápadů](zkousky/E8/napady.png)

## 2. Zbývá

Navazuje E9 s akustickými bicími, stereo modulem, skupinou mikrofonů a ovládáním od soupravy. Použitelnost formátu M4A záleží na systémovém kodeku ve Windows.

## 3. Ruční zkoušky

H-11 a H-12, poslech předstihu a doťukání tempa s nástrojem **čekají na ruční zkoušku po vydání v1**. [Návod pro Jendu](../README.md#jak-zachytit-riff)

## 4. Rozhodnutí

Pojistka sleduje zvolený kytarový vstup. Doťukaný riff se umístí na mřížku s celým začátkem. Podklady se při importu uloží do samostatného WAV ve skladbě. [Rozhodnutí E8](ROZHODNUTI.md#e8--nápad-a-jeho-tempo)

## 5. Prompt pro etapu E9

### Podklady

Jsi hlavní vývojář Rockeru 2. Na GitHubu zkontroluj větve, rozpracované změny, PR, testy, balíky a vydání. Přečti dokumenty 01–05, STAV, ROZHODNUTI a předání na nejnovější ověřené větvi. Rozsah určuje 04 / E9, nastavení bicích 01 / 4.4 a ovládání od soupravy 01 / kap. 5.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** Pracuj jen v Rocker-2. C++20, JUCE 9.0.5, vlastní kreslení přes tokeny, české texty pohromadě, kód a komentáře anglicky. Zachovej offline provoz, AGPLv3, OFL a přibalené licence. Ve zvukovém vlákně dodrž 02 / 4.3. Zachovej starší skladby, bezpečné ukládání, původní zvuk, pokusy, díly, export a nápady.

### Úkol

Přidej výzvu **Zahraj na bicí**, rozpoznání skutečně použitých vstupů a radu pro sestavu s málo vstupy. Umožni společnou nahrávku, opravu, střih, hlasitost a polohy více mikrofonů v **Sadě bicích**. Podporuj stereo modul, vyzkoušej nejsilnější úder. Dokonči Pohled zdálky s dobami, postupem a potvrzením uložení. Umožni ovládání numerickou klávesnicí a přiřazení nožního spínače.

### Ověření

Spusť část A-24 pro skutečné počty dostupných vstupů a celé předchozí automatické zkoušky. Integraci vyzkoušej na simulovaných sestavách, pořiď snímky a balíky pro Windows x64, Windows ARM64 a Mac. H-05 až H-08 a hraní ze skutečné soupravy čekají na ruční zkoušku po vydání.

### Předání

Každá etapa má vlastní větev etapa-N, malé commity a český PR. Přidej přímé odkazy na ověřený běh, balíky a snímky. Aktualizuj STAV, ROZHODNUTI a PREDANI v dosavadním pořadí včetně promptu pro E10. Pokračuj podle 04 bez čekání na hraní s Jendou.
