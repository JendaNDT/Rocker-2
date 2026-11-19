# Předání E7 · mix a export

## 1. Hotovo

Ve skladbě stiskni **3 Míchej**. Při přehrávání nastav hlasitost a polohu stop, sleduj levý a pravý kanál u Celkem. **Ctrl+E** otevře výběr formátu a rozsahu. Po exportu můžeš výsledek rovnou přehrát. Odkazy na běh a jeho balíky pro všechny tři systémy jsou v návrhu E7. [Výsledky a sestavení](https://github.com/JendaNDT/Rocker-2/pull/8) · [Mix](zkousky/E7/mix.png) · [Poslat mix](zkousky/E7/export.png)

## 2. Zbývá

Navazuje E8 s rychlým uložením nápadu a importem podkladu. Doručení MP3 závisí na dostupnosti systémového kodéru ve Windows. Výsledek zkoušky A-12 je samostatně v běhu pro každý typ procesoru.

## 3. Ruční zkoušky

M-03, M-13, výkon na skutečném notebooku a poslech kytary, bicích a exportu v telefonu **čekají na ruční zkoušku po vydání v1**. [Postup pro Jendu](../README.md#jak-poslat-mix)

## 4. Rozhodnutí

Převod starších mono stop zachová hlasitost. Celá skladba má dvě sekundy na dozvuk. Výběr dodrží hranice zvolených taktů. Každý export se uloží zvlášť. [Rozhodnutí E7](ROZHODNUTI.md#e7--hlasitost-a-soubor-pro-posluchače)

## 5. Prompt pro etapu E8

### Podklady

Jsi hlavní vývojář Rockeru 2. Ověř aktuální větve, rozpracované změny, PR, skutečné výsledky testů a vydání. Přečti dokumenty 01–05, STAV, ROZHODNUTI a poslední předání na ověřené větvi. Rozsah E8 určuje dokument 04, práci hráče dokument 01 / 2.1 a kapitola o Nápadech.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** Pracuj jen v Rocker-2. C++20 a JUCE 9.0.5, vlastní kreslení a designové tokeny. České texty soustřeď na jedno místo. Kód a komentáře anglicky. Aplikace funguje offline. Zachovej licence AGPLv3 a OFL, čitelnost starších skladeb, původní WAV, mix a export. Dodrž pravidla zvukového vlákna podle 02 / 4.3.

### Úkol

Zprovozni **Nahrát nápad** s dvousekundovým předstihem, **Pojistku** a klávesu **U – Ulož, co jsem hrál**. Nápady se mají dát označit hvězdou, opatřit poznámkou, vyfiltrovat a poslechnout. Přidej **Udělat z toho skladbu**, **Doťukat tempo** a úplný úvod. Importuj WAV, AIFF, FLAC, OGG, Opus a MP3. Pro M4A využij dostupné kodeky Windows. Zachovej rychlost i výšku při změně vzorkovací frekvence.

### Ověření

Spusť A-18, A-19, zkoušky ovládání a regresní sadu včetně mixu a exportu. Ověř cestu od riffu přes hvězdu až ke skladbě s bicími. Ulož sestavení a snímky na Windows x64, Windows ARM64 a Macu. H-11 a H-12 čekají na ruční zkoušku po vydání.

### Předání

Pracuj ve větvi etapa-8 po malých commitech. Otevři český PR s výsledky, balíky a snímky. Aktualizuj STAV, ROZHODNUTI a toto předání v pořadí Hotovo, Zbývá, Ruční zkoušky, Rozhodnutí, Prompt pro E9. Pokračuj podle 04 bez čekání na ruční zkoušky.
